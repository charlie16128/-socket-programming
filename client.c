#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// 建立 TCP IPv4 socket
int createTCPIpv4Socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket無法建立");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

// 建立 IPv4 地址結構
struct sockaddr_in *createIPv4Address(const char *ip, int port) {
    struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (ip[0] == '\0') {
        addr->sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
            perror("無效的ip地址");
            free(addr);
            exit(EXIT_FAILURE);
        }
    }
    return addr;
}

// 新線程函數，用於接收伺服器訊息
void *listenAndPrint(void *arg) {
    int socketFD = (int)(intptr_t)arg;
    char buffer[1024];

    while (true) {
        ssize_t amountReceived = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
        if (amountReceived > 0) {
            buffer[amountReceived] = '\0';
            printf("伺服器回應: %s\n", buffer);
        } else if (amountReceived == 0) {
            printf("伺服器已關閉連接。\n");
            break;
        } else {
            perror("接收訊息失敗");
            break;
        }
    }

    close(socketFD);
    pthread_exit(NULL);
}

// 用戶輸入並傳送訊息到伺服器
void readConsoleEntriesAndSendToServer(int socketFD) {
    char *name = NULL;
    size_t nameSize = 0;
    printf("請輸入名稱: ");
    ssize_t nameCount = getline(&name, &nameSize, stdin);
    name[nameCount - 1] = '\0'; // 移除換行符

    char *line = NULL;
    size_t lineSize = 0;
    printf("輸入訊息 (輸入 'exit' 關閉):\n");

    char buffer[1024];

    while (true) {
        printf("> ");
        ssize_t charCount = getline(&line, &lineSize, stdin);
        line[charCount - 1] = '\0'; // 移除換行符

        if (strcmp(line, "exit") == 0)
            break;

        snprintf(buffer, sizeof(buffer), "%s: %s", name, line);

        ssize_t amountWasSent = send(socketFD, buffer, strlen(buffer), 0);
        if (amountWasSent <= 0) {
            perror("訊息傳送失敗");
            break;
        }
    }

    free(name);
    free(line);
}

// 創建線程以接收伺服器訊息
void startListeningAndPrintMessagesOnNewThread(int socketFD) {
    pthread_t id;
    if (pthread_create(&id, NULL, listenAndPrint, (void *)(intptr_t)socketFD) != 0) {
        perror("無法創建新線程");
        close(socketFD);
        exit(EXIT_FAILURE);
    }
    pthread_detach(id); // 分離線程，無需顯式回收
}

int main() {
    int socketFD = createTCPIpv4Socket();
    struct sockaddr_in *address = createIPv4Address("127.0.0.1", 2000);

    // 連接伺服器
    if (connect(socketFD, (struct sockaddr *)address, sizeof(*address)) != 0) {
        perror("連接伺服器失敗");
        free(address);
        close(socketFD);
        exit(EXIT_FAILURE);
    }
    printf("成功連接伺服器！\n");

    free(address);

    // 開始接收伺服器訊息
    startListeningAndPrintMessagesOnNewThread(socketFD);

    // 讀取用戶輸入並傳送
    readConsoleEntriesAndSendToServer(socketFD);

    close(socketFD);

    return 0;
}
