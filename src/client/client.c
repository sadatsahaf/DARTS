#include "../../include/common.h"
#include "../../include/utils.h"


int sock_fd;
char username[NAME_LEN];
volatile int running = 1;


void* receive_handler(void* arg) {
    Packet pkt;
    while(running) {
        int bytes = recv_packet(sock_fd, &pkt);
        if(bytes <= 0) {
            printf("\nServer disconnected.\n");
            running = 0;
            exit(0);
        }

        if(pkt.type == MSG_PRIVATE) {
            printf("\033[0;33m[PM from %s]: %s\033[0m\n", pkt.source, pkt.data);
        } else if (pkt.type == MSG_ERROR) {
            printf("\033[0;31m[ERROR]: %s\033[0m\n", pkt.data);
        } else if (pkt.type == MSG_ACK) {
             printf("\033[0;32m[INFO]: %s\033[0m\n", pkt.data);
        } else {
            printf("[%s]: %s\n", pkt.source, pkt.data);
            fflush(stdout);
        }
        printf("> "); 
        fflush(stdout);
    }
    return NULL;
}
#include <stdio.h>

void print_new_ascii_art() {
    printf("                   ##########               \n");
    printf("               #####        #####   ###     \n");
    printf("            ####               ######       \n");
    printf("           ###            ##########        \n");
    printf("         ###          #######  #### ###     \n");
    printf("        ###      #########  ######   ###    \n");
    printf("        #   ############  ####### #   ###   \n");
    printf("       ##############   ########    ##  ##   \n");
    printf("     ############    #########       # ##   \n");
    printf("        ######    ###########        ####   \n");
    printf("       ##       ############ ######  ####   \n");
    printf("       ###    #############          ####   \n");
    printf("        ##   ##  ########           ####    \n");
    printf("         ##  ####  #####            ###     \n");
    printf("           # #        #  ##########  #      \n");
    printf("          ####                   ######     \n");
    printf("         ###  ####            ####          \n");
    printf("        ##       ############               \n");
    printf("                                            \n");
    printf("                                            \n");
}
void logo(){

    printf(" ::::::::::.  .:::::::::.  ::::::::::.   ::::::::::::    .:::::::::::\n");
    printf(" ::::   ::::  ::::   ::::  ::::   :::::      ::::        ::::\n");
    printf(" ::::   ::::  ::::   ::::  ::::   :::::      ::::        ::::\n");
    printf(" ::::   ::::  :::::::::::  ::::::::::'       ::::        ':::::::::.\n");
    printf(" ::::   ::::  ::::   ::::  ::::   ::::.      ::::               :::::\n");
    printf(" ::::   ::::  ::::   ::::  ::::    :::::     ::::               :::::\n");
    printf(" ::::::::::'  ::::   ::::  ::::     ::::     ::::        ::::::::::'\n");
}



void print_help() {
    printf("\n--- DARTS COMMANDS ---\n");
    printf("/all <msg>   : Broadcast message\n");
    printf("/pm <uptr> <msg> : Private message\n");
    printf("/users       : List online users\n");
    printf("/quit        : Exit\n");
    printf("----------------------\n");
}


int main(int argc, char const *argv[]) {
    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        return 1;
    }
#endif

    if(argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    struct sockaddr_in serv_addr;
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) die("Socket creation error");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0) die("Invalid address");

    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) die("Connection Failed");

    // Login
    printf("Enter Username: ");
    scanf("%s", username);
    getchar(); // Consume newline

    Packet login;
    login.type = MSG_LOGIN;
    strcpy(login.source, username);
    send_packet(sock_fd, &login);

    // Start Receiver Thread
    pthread_t recv_thread;
    if(pthread_create(&recv_thread, NULL, receive_handler, NULL) != 0) die("Thread failed");
    print_new_ascii_art();
    logo();
    print_help();
    printf("> ");

    char buffer[BUFFER_SIZE];
    while(running && fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline

        Packet pkt;
        strcpy(pkt.source, username);
        //for quiting
        if(strncmp(buffer, "/quit", 5) == 0) {
            pkt.type = MSG_EXIT;
            send_packet(sock_fd, &pkt);
            running = 0;
            break;
        } 
        //for broadcasting
        else if (strncmp(buffer, "/users", 6) == 0) {
            pkt.type = MSG_LIST;
            send_packet(sock_fd, &pkt);
        }
        //for personal message
        else if (strncmp(buffer, "/pm", 3) == 0) {
            pkt.type = MSG_PRIVATE;
            strtok(buffer, " ");
            char *target = strtok(NULL, " ");
            char *msg = strtok(NULL, "");
            
            if(target && msg) {
                while (*msg == ' ') msg++;
                strcpy(pkt.target, target);
                snprintf(pkt.data, BUFFER_SIZE, "%s", msg);
                send_packet(sock_fd, &pkt);
            } else {
                printf("Usage: /pm <user> <msg>\n");
            }
        }
        // for all the users
        else if (strncmp(buffer, "/all", 4) == 0) {
            pkt.type = MSG_BROADCAST;
            strtok(buffer, " ");
            char *msg = strtok(NULL, "");
            if(msg) {
                strcpy(pkt.data, msg);
                while (*msg == ' ') msg++;
                snprintf(pkt.data, BUFFER_SIZE, "%s", msg);
                send_packet(sock_fd, &pkt);
            } else {
                printf("Usage: /all <msg>\n>");
            }
        }
        else {
            pkt.type = MSG_BROADCAST;
            strcpy(pkt.data, buffer);
            send_packet(sock_fd, &pkt);
        }
        
        // Sleep slightly to let the receiver thread print incoming first if race happens
        usleep(10000); 
        printf("> ");
    }

    close(sock_fd);
    return 0;
}