#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int createTCPIpv4Socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("socket無法建立");
        exit(1);
    }
    return sockfd;
}

struct sockaddr_in *createIPv4Address(const char *ip, int port) {
    struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (ip[0] == '\0') {
        addr->sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
            perror("無效的ip地址");
            exit(1);
        }
    }
    return addr;
}

void startListeningAndPrintMessagesOnNewThread(int fd);

void listenAndPrint(void *arg);

void readConsoleEntriesAndSendToServer(int socketFD) {
    char *name = NULL;
    size_t nameSize = 0;
    printf("請輸入名稱\n");
    ssize_t nameCount = getline(&name, &nameSize, stdin);
    name[nameCount - 1] = 0;

    char *line = NULL;
    size_t lineSize = 0;
    printf("輸入(exit關閉) : \n");

    char buffer[1024];

    while (true) {
        ssize_t charCount = getline(&line, &lineSize, stdin);
        line[charCount - 1] = 0;

        sprintf(buffer, "%s:%s", name, line);

        if (charCount > 0) {
            if (strcmp(line, "exit") == 0)
                break;

            ssize_t amountWasSent = send(socketFD, buffer, strlen(buffer), 0);
        }
    }
}

int main() {
    int socketFD = createTCPIpv4Socket();
    struct sockaddr_in *address = createIPv4Address("127.0.0.1", 2000);

    int result = connect(socketFD, (struct sockaddr *)address, sizeof (*address));

    if (result == 0)
        printf("成功連接伺服器\n");

    startListeningAndPrintMessagesOnNewThread(socketFD);

    readConsoleEntriesAndSendToServer(socketFD);

    close(socketFD);

    return 0;
}



void startListeningAndPrintMessagesOnNewThread(int socketFD) {
    pthread_t id;
    pthread_create(&id, NULL, listenAndPrint, (void *)(intptr_t)socketFD);
}

void listenAndPrint(void *arg) {
    int socketFD = (int)(intptr_t)arg;
    char buffer[1024];

    while (true) {
        ssize_t amountReceived = recv(socketFD, buffer, 1024, 0);

        if (amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("Response was %s\n", buffer);
        }

        if (amountReceived == 0)
            break;
    }

    close(socketFD);
}
