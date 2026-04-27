#include "protocol.h"

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <cstdlib>
#include <ctime>

std::vector<std::vector<int>> GenerateMatrix(int n)
{
    std::vector<std::vector<int>> matrix(n, std::vector<int>(n));

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = std::rand() % 10;
        }
    }

    return matrix;
}

int main()
{
    std::srand((unsigned int)std::time(nullptr));

    char buff[1024];

    if (WSAStartup(0x0202, (WSADATA*)&buff[0])) {
        std::cout << "WSAStartup error: " << WSAGetLastError() << "\n";
        return -1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket == INVALID_SOCKET) {
        std::cout << "Socket creation error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return -1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    std::cout << "Connecting to server...\n";

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr))) {
        std::cout << "Connect error: " << WSAGetLastError() << "\n";
        closesocket(client_socket);
        WSACleanup();
        return -1;
    }

    std::cout << "Connected to TCP server successfully!\n";

    int n = 5;
    int threadCount = 2;

    std::vector<std::vector<int>> matrix = GenerateMatrix(n);

    std::cout << "\nOriginal matrix on client:\n";
    PrintMatrix(matrix);

    int command;
    int response;

    command = CMD_GET_STATUS;
    sendInt(client_socket, command);
    response = recvInt(client_socket);
    std::cout << "\nSTATUS before sending data: " << response << "\n";

    command = CMD_SEND_DATA;
    sendInt(client_socket, command);
    sendInt(client_socket, n);
    sendInt(client_socket, threadCount);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            sendInt(client_socket, matrix[i][j]);
        }
    }

    response = recvInt(client_socket);
    std::cout << "SEND_DATA response: " << response << "\n";

    command = CMD_GET_STATUS;
    sendInt(client_socket, command);
    response = recvInt(client_socket);
    std::cout << "STATUS after sending data: " << response << "\n";

    command = CMD_START;
    sendInt(client_socket, command);
    response = recvInt(client_socket);
    std::cout << "START response: " << response << "\n";

    command = CMD_GET_STATUS;
    sendInt(client_socket, command);
    response = recvInt(client_socket);
    std::cout << "STATUS immediately after START: " << response << "\n";

    WaitByTimer(4);

    command = CMD_GET_STATUS;
    sendInt(client_socket, command);
    response = recvInt(client_socket);
    std::cout << "STATUS after timer: " << response << "\n";

    command = CMD_GET_RESULT;
    sendInt(client_socket, command);
    response = recvInt(client_socket);
    std::cout << "GET_RESULT response: " << response << "\n";

    if (response == RESP_OK) {
        std::vector<std::vector<int>> resultMatrix(n, std::vector<int>(n));

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                resultMatrix[i][j] = recvInt(client_socket);
            }
        }

        std::cout << "\nResult matrix from server:\n";
        PrintMatrix(resultMatrix);
    }

    command = CMD_DISCONNECT;
    sendInt(client_socket, command);

    closesocket(client_socket);
    WSACleanup();

    return 0;
}
