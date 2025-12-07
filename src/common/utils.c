#include "../../include/utils.h"
#include <time.h>

// Windows inet_pton wrapper
#ifdef _WIN32
int inet_pton(int af, const char *src, void *dst) {
    struct sockaddr_storage ss;
    int size = sizeof(ss);
    char src_copy[INET6_ADDRSTRLEN + 1];
    strncpy(src_copy, src, INET6_ADDRSTRLEN);
    src_copy[INET6_ADDRSTRLEN] = '\0';
    
    if (WSAStringToAddressA(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
        switch(af) {
            case AF_INET:
                *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
                return 1;
            case AF_INET6:
                *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}
#endif

void die(const char *msg) {
    perror(msg);
    #ifdef _WIN32
        printf("Winsock Error: %d\n", WSAGetLastError());
    #endif
    exit(1);
}

void log_msg(const char *msg) {
    time_t now;
    time(&now);
    char *date = ctime(&now);
    date[strlen(date) - 1] = '\0';
    printf("[%s] LOG: %s\n", date, msg);
    
    // Write to log file
    FILE *logfile = fopen("darts_chat.log", "a");
    if (logfile) {
        fprintf(logfile, "[%s] %s\n", date, msg);
        fclose(logfile);
    }
}

void send_packet(int fd, Packet *pkt) {
    // Replaced write() with send()
    if(send(fd, (char*)pkt, sizeof(Packet), 0) < 0) {
        // perror("Send failed");
    }
}

int recv_packet(int fd, Packet *pkt) {
    // Replaced read() with recv()
    int bytes = recv(fd, (char*)pkt, sizeof(Packet), 0);
    return bytes;
}