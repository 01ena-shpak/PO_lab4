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

void HandleClient(SOCKET client_socket)
{
    std::cout << "\nClient connected in thread\n";

    std::vector<std::vector<int>> matrix;
    int n = 0;
    int threadCount = 1;

    bool hasData = false;
    bool isDone = false;
    bool isRunning = false;

    while (true)
    {
        int command;
        int bytes_recv = recv(client_socket, (char*)&command, sizeof(command), 0);

        if (bytes_recv <= 0) {
            std::cout << "Client disconnected unexpectedly\n";
            break;
        }

        if (command == CMD_SEND_DATA)
        {
            recv(client_socket, (char*)&n, sizeof(n), 0);
            recv(client_socket, (char*)&threadCount, sizeof(threadCount), 0);

            matrix = std::vector<std::vector<int>>(n, std::vector<int>(n));

            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    recv(client_socket, (char*)&matrix[i][j], sizeof(int), 0);
                }
            }

            hasData = true;
            isDone = false;
            isRunning = false;

            std::cout << "\nCMD_SEND_DATA received\n";
            std::cout << "Matrix size N = " << n << "\n";
            std::cout << "Thread count = " << threadCount << "\n";

            std::cout << "\nMatrix received:\n";
            PrintMatrix(matrix);

            int response = RESP_OK;
            send(client_socket, (char*)&response, sizeof(response), 0);
        }
        else if (command == CMD_GET_STATUS)
        {
            int response;

            if (!hasData) {
                response = RESP_NO_DATA;
            }
            else if (isRunning) {
                response = RESP_IN_PROGRESS;
            }
            else if (isDone) {
                response = RESP_DONE;
            }
            else {
                response = RESP_OK;
            }

            send(client_socket, (char*)&response, sizeof(response), 0);

            std::cout << "CMD_GET_STATUS received, response = " << response << "\n";
        }
        else if (command == CMD_START)
        {
            if (!hasData) {
                int response = RESP_NO_DATA;
                send(client_socket, (char*)&response, sizeof(response), 0);
            }
            else {
                std::cout << "\nCMD_START received\n";

                isRunning = true;

                std::cout << "\nMatrix BEFORE:\n";
                PrintMatrix(matrix);

                MirrorRightToLeftParallel(matrix, threadCount);

                std::cout << "\nMatrix AFTER:\n";
                PrintMatrix(matrix);

                isRunning = false;
                isDone = true;

                int response = RESP_DONE;
                send(client_socket, (char*)&response, sizeof(response), 0);
            }
        }
        else if (command == CMD_GET_RESULT)
        {
            if (!isDone) {
                int response = RESP_IN_PROGRESS;
                send(client_socket, (char*)&response, sizeof(response), 0);
            }
            else {
                int response = RESP_OK;
                send(client_socket, (char*)&response, sizeof(response), 0);

                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < n; j++) {
                        send(client_socket, (char*)&matrix[i][j], sizeof(int), 0);
                    }
                }

                std::cout << "\nResult matrix sent to client\n";
            }
        }
        else if (command == CMD_DISCONNECT)
        {
            std::cout << "CMD_DISCONNECT received\n";
            break;
        }
        else {
            int response = RESP_ERROR;
            send(client_socket, (char*)&response, sizeof(response), 0);
        }
    }

    closesocket(client_socket);
    std::cout << "Client thread finished\n";
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
    std::cout << "Waiting for clients...\n";

    while (true)
    {
        sockaddr_in client_addr;
        int client_addr_size = sizeof(client_addr);

        SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_size);

        if (client_socket == INVALID_SOCKET) {
            std::cout << "Accept error\n";
            continue;
        }

        std::thread clientThread(HandleClient, client_socket);
        clientThread.detach();
    }

    closesocket(server_socket);
    WSACleanup();

    return 0;
}