#include "protocol.h"

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <thread>

void PrintMatrix(const std::vector<std::vector<int>>& matrix)
{
    int n = (int)matrix.size();

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::cout << matrix[i][j] << " ";
        }
        std::cout << "\n";
    }
}

void MirrorWorker(std::vector<std::vector<int>>& a, int startRow, int endRow)
{
    int n = (int)a.size();

    for (int i = startRow; i < endRow; i++) {
        for (int j = 0; j < n / 2; j++) {
            a[i][j] = a[i][n - 1 - j];
        }
    }
}

void MirrorRightToLeftParallel(std::vector<std::vector<int>>& a, int threadCount)
{
    int n = (int)a.size();
    if (n == 0) return;

    if (threadCount < 1) threadCount = 1;
    if (threadCount > n) threadCount = n;

    std::vector<std::thread> threads;

    int base = n / threadCount;
    int extra = n % threadCount;

    int start = 0;

    for (int t = 0; t < threadCount; t++) {
        int rows = base + (t < extra ? 1 : 0);
        int end = start + rows;

        threads.push_back(std::thread(MirrorWorker, std::ref(a), start, end));

        start = end;
    }

    for (int i = 0; i < (int)threads.size(); i++) {
        threads[i].join();
    }
}

int main()
{
    char buff[1024];

    std::cout << "TCP SERVER STARTING...\n";

    if (WSAStartup(0x0202, (WSADATA*)&buff[0])) {
        std::cout << "WSAStartup error\n";
        return -1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == INVALID_SOCKET) {
        std::cout << "Socket error\n";
        WSACleanup();
        return -1;
    }

    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(SERVER_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&local_addr, sizeof(local_addr))) {
        std::cout << "Bind error\n";
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    if (listen(server_socket, 5)) {
        std::cout << "Listen error\n";
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server started on port " << SERVER_PORT << "\n";
    std::cout << "Waiting for client...\n";

    sockaddr_in client_addr;
    int client_addr_size = sizeof(client_addr);

    SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_size);

    std::cout << "Client connected\n";

    int command;
    recv(client_socket, (char*)&command, sizeof(command), 0);

    if (command == CMD_SEND_DATA)
    {
        int n, threadCount;

        recv(client_socket, (char*)&n, sizeof(n), 0);
        recv(client_socket, (char*)&threadCount, sizeof(threadCount), 0);

        std::vector<std::vector<int>> matrix(n, std::vector<int>(n));

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                recv(client_socket, (char*)&matrix[i][j], sizeof(int), 0);
            }
        }

        std::cout << "\nMatrix BEFORE:\n";
        PrintMatrix(matrix);

        MirrorRightToLeftParallel(matrix, threadCount);

        std::cout << "\nMatrix AFTER:\n";
        PrintMatrix(matrix);

        int response = RESP_OK;
        send(client_socket, (char*)&response, sizeof(response), 0);

        // надсилаємо результат назад клієнту
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                send(client_socket, (char*)&matrix[i][j], sizeof(int), 0);
            }
        }

        std::cout << "\nResult matrix sent to client\n";
    }

    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}