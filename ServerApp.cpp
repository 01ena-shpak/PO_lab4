#include "protocol.h"

#include <iostream>
#include <winsock2.h>

int main()
{
    char buff[1024];

    std::cout << "TCP SERVER STARTING...\n";

    if (WSAStartup(0x0202, (WSADATA*)&buff[0])) {
        std::cout << "WSAStartup error: " << WSAGetLastError() << "\n";
        return -1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == INVALID_SOCKET) {
        std::cout << "Socket creation error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return -1;
    }

    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(SERVER_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&local_addr, sizeof(local_addr))) {
        std::cout << "Bind error: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    if (listen(server_socket, 5)) {
        std::cout << "Listen error: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server started on port " << SERVER_PORT << "\n";
    std::cout << "Waiting for client...\n";

    sockaddr_in client_addr;
    int client_addr_size = sizeof(client_addr);

    SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_size);

    if (client_socket == INVALID_SOCKET) {
        std::cout << "Accept error: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    std::cout << "Client connected successfully!\n";

    int command;
    int bytes_recv = recv(client_socket, (char*)&command, sizeof(command), 0);

    if (bytes_recv == SOCKET_ERROR) {
        std::cout << "Recv error: " << WSAGetLastError() << "\n";
    }
    else if (bytes_recv == 0) {
        std::cout << "Client disconnected\n";
    }
    else {
        std::cout << "Received command: " << command << "\n";

        if (command == CMD_SEND_DATA) {
            int n;
            int threadCount;

            recv(client_socket, (char*)&n, sizeof(n), 0);
            recv(client_socket, (char*)&threadCount, sizeof(threadCount), 0);

            std::cout << "Command is CMD_SEND_DATA\n";
            std::cout << "Matrix size N = " << n << "\n";
            std::cout << "Thread count = " << threadCount << "\n";

            int response = RESP_OK;
            send(client_socket, (char*)&response, sizeof(response), 0);
        }
        else {
            std::cout << "Unknown command\n";

            int response = RESP_ERROR;
            send(client_socket, (char*)&response, sizeof(response), 0);
        }
    }

    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}