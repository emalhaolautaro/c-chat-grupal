//
// Servidor TCP concurrente con múltiples clientes (Windows)
// Autor: Lautaro Emalhao
// Fecha: 18/10/2025
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 10000
#define BUFFER_SIZE 1024
#define NAME_SIZE 32

int main() {
    WSADATA wsaData;
    SOCKET listenSocket, new_socket;
    SOCKET client_sockets[FD_SETSIZE];   // Sockets de clientes
    char client_names[FD_SETSIZE][NAME_SIZE]; // Nombres de clientes
    struct sockaddr_in server_addr, client_addr;
    fd_set readfds;
    int addrlen, activity, i, valread, sd, max_sd;
    char buffer[BUFFER_SIZE];

    // Inicializar Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error: WSAStartup falló\n");
        return 1;
    }
    printf("Winsock inicializado.\n");

    // Crear socket de escucha
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        printf("Error al crear socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Configurar dirección y puerto
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(listenSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Error en bind: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Listen
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Error en listen: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    printf("Servidor escuchando en el puerto %d...\n", PORT);

    // Inicializar arrays de clientes
    for (i = 0; i < FD_SETSIZE; i++) {
        client_sockets[i] = 0;
        client_names[i][0] = '\0';
    }

    addrlen = sizeof(client_addr);

    // Bucle principal
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listenSocket, &readfds);
        max_sd = listenSocket;

        // Agregar sockets de clientes al conjunto
        for (i = 0; i < FD_SETSIZE; i++) {
            sd = client_sockets[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        // Esperar actividad
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && WSAGetLastError() != WSAEINTR) {
            printf("Error en select: %d\n", WSAGetLastError());
        }

        // Nueva conexión entrante
        if (FD_ISSET(listenSocket, &readfds)) {
            new_socket = accept(listenSocket, (struct sockaddr*)&client_addr, &addrlen);
            if (new_socket != INVALID_SOCKET) {
                // Recibir nombre del cliente
                char name_buffer[NAME_SIZE];
                int name_len = recv(new_socket, name_buffer, NAME_SIZE - 1, 0);
                if (name_len <= 0) {
                    closesocket(new_socket);
                    continue;
                }
                name_buffer[name_len] = '\0';

                // Guardar socket y nombre
                for (i = 0; i < FD_SETSIZE; i++) {
                    if (client_sockets[i] == 0) {
                        client_sockets[i] = new_socket;
                        strncpy(client_names[i], name_buffer, NAME_SIZE);
                        client_names[i][NAME_SIZE - 1] = '\0';
                        printf("Nueva conexión: %s desde %s:%d\n",
                               client_names[i],
                               inet_ntoa(client_addr.sin_addr),
                               ntohs(client_addr.sin_port));
                        break;
                    }
                }
            }
        }

        // Manejar mensajes de clientes
        for (i = 0; i < FD_SETSIZE; i++) {
            sd = client_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);
                if (valread <= 0) {
                    // Cliente desconectado
                    printf("%s se ha desconectado.\n", client_names[i]);
                    closesocket(sd);
                    client_sockets[i] = 0;
                    client_names[i][0] = '\0';
                } else {
                    buffer[valread] = '\0';

                    // Preparar mensaje con nombre
                    char message_to_send[BUFFER_SIZE + NAME_SIZE];
                    snprintf(message_to_send, sizeof(message_to_send), "%s: %s", client_names[i], buffer);
                    printf("%s\n", message_to_send);

                    // Enviar a todos los demás clientes
                    for (int j = 0; j < FD_SETSIZE; j++) {
                        int out_sd = client_sockets[j];
                        if (out_sd > 0 && out_sd != sd) {
                            send(out_sd, message_to_send, strlen(message_to_send), 0);
                        }
                    }

                    // Respuesta opcional al cliente que envió
                    const char* response = "Mensaje recibido por el servidor.\n";
                    send(sd, response, strlen(response), 0);
                }
            }
        }
    }

    // Nunca se alcanza, pero por completitud
    closesocket(listenSocket);
    WSACleanup();
    printf("Servidor finalizado.\n");
    return 0;
}