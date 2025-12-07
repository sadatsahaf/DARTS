#include "../../include/common.h"
#include "../../include/ds.h"
#include "../../include/utils.h"

// Globals
Node* client_list = NULL;
HashTable user_map;
fd_set readfds;
int max_sd;

void handle_client_message(int fd);
void broadcast_message(Packet* pkt, int sender_fd);
void disconnect_client(int fd);

int main() {
    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        return 1;
    }
#endif

    int server_fd, new_socket;
    struct sockaddr_in address;
    #ifdef _WIN32
    char opt = 1;
    #else
    int opt = 1;
    #endif
    int addrlen = sizeof(address);

    // Init Data Structures
    hash_init(&user_map);

    // Create Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) die("socket failed");
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) die("setsockopt");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        printf("WSA Error: %d\n", WSAGetLastError());
        die("bind failed");
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        printf("WSA Error: %d\n", WSAGetLastError());
        die("listen");
    }

    log_msg("DARTS Server started on port 8888...");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add child sockets to set
        Node* curr = client_list;
        while(curr) {
            int sd = curr->socket_fd;
            if(sd > 0) FD_SET(sd, &readfds);
            if(sd > max_sd) max_sd = sd;
            curr = curr->next;
        }

        // Wait for activity
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) printf("select error");

        // Incoming connection on master socket
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            
            list_add(&client_list, new_socket);
            printf("New connection, socket fd is %d\n", new_socket);
        }

        // Check IO on other sockets
        curr = client_list;
        while(curr) {
            int sd = curr->socket_fd;
            Node* next = curr->next; // Save next pointer before handle_client_message may disconnect
            if (FD_ISSET(sd, &readfds)) {
                handle_client_message(sd);
            }
            curr = next;
        }
    }
    return 0;
}

void handle_client_message(int fd) {
    Packet pkt;
    int bytes = recv_packet(fd, &pkt);

    if (bytes <= 0) {
        disconnect_client(fd);
        return;
    }

    switch(pkt.type) {
        case MSG_LOGIN: {
            // Register user in Hash Table
            hash_insert(&user_map, pkt.source, fd);
            printf("User logged in: %s (FD: %d)\n", pkt.source, fd);
            
            // Notify others
            Packet welcome;
            welcome.type = MSG_BROADCAST;
            snprintf(welcome.data, BUFFER_SIZE, "SERVER: %s has joined the chat.", pkt.source);
            strcpy(welcome.source, "SERVER");
            broadcast_message(&welcome, fd);
            break;
        }
        case MSG_BROADCAST:
            // Just forward
            broadcast_message(&pkt, fd);
            break;
        case MSG_PRIVATE: {
            int target_fd = hash_get(&user_map, pkt.target);
            if(target_fd != -1) {
                send_packet(target_fd, &pkt);
            } else {
                // Send error back to sender
                Packet err;
                err.type = MSG_ERROR;
                strcpy(err.source, "SERVER");
                sprintf(err.data, "User '%s' not found.", pkt.target);
                send_packet(fd, &err);
            }
            break;
        }
        case MSG_LIST: {
            Packet list_pkt;
            list_pkt.type = MSG_ACK;
            strcpy(list_pkt.source, "SERVER");
            strcpy(list_pkt.data, "Online users: ");
            
            for(int i=0; i<HASH_SIZE; i++) {
                HashEntry* e = user_map.buckets[i];
                while(e) {
                    strcat(list_pkt.data, e->username);
                    strcat(list_pkt.data, ", ");
                    e = e->next;
                }
            }
            send_packet(fd, &list_pkt);
            break;
        }
        case MSG_EXIT:
            disconnect_client(fd);
            break;
        default:
            break;
    }
}

void broadcast_message(Packet* pkt, int sender_fd) {
    Node* curr = client_list;
    while(curr) {
        if(curr->socket_fd != sender_fd) {
            send_packet(curr->socket_fd, pkt);
        }
        curr = curr->next;
    }
    
    // Log broadcast messages to file
    if (pkt->type == MSG_BROADCAST) {
        FILE *logfile = fopen("darts_chat.log", "a");
        if (logfile) {
            time_t now;
            time(&now);
            char *date = ctime(&now);
            char datebuf[100];
            strncpy(datebuf, date, strlen(date) - 1);
            datebuf[strlen(date) - 1] = '\0';
            fprintf(logfile, "[%s] %s: %s\n", datebuf, pkt->source, pkt->data);
            fclose(logfile);
        }
    }
}

void disconnect_client(int fd) {
    char* name = hash_get_user_by_fd(&user_map, fd);
    if(name) {
        printf("User disconnected: %s\n", name);
        hash_remove(&user_map, name);
    } else {
        printf("Socket %d disconnected\n", fd);
    }
    close(fd);
    list_remove(&client_list, fd);
}
