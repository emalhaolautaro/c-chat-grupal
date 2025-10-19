#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 10000
#define BUFFER_SIZE 1024
#define NAME_SIZE 32

SOCKET sock;

DWORD WINAPI recvThread(LPVOID lpParam) {
    char buffer[BUFFER_SIZE];
    int valread;
    while (1) {
        valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (valread <= 0) {
            printf("Servidor desconectado.\n");
            exit(0);
        }
        buffer[valread] = '\0';
        printf("%s\n", buffer);
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    struct sockaddr_in server_addr;
    char name[NAME_SIZE];
    char buffer[BUFFER_SIZE];

    WSAStartup(MAKEWORD(2,2), &wsaData);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Error al crear socket: %d\n", WSAGetLastError());
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Error al conectar al servidor: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Ingrese su nombre: ");
    fgets(name, NAME_SIZE, stdin);
    name[strcspn(name, "\n")] = '\0';
    send(sock, name, strlen(name), 0);

    // Crear hilo para recibir mensajes
    HANDLE hThread = CreateThread(NULL, 0, recvThread, NULL, 0, NULL);

    // Bucle principal de envío de mensajes
    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strcmp(buffer, "exit") == 0) break;
        send(sock, buffer, strlen(buffer), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}