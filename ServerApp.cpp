#include "protocol.h"

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <thread>
#include <mutex>

std::mutex coutMutex;
int nextClientId = 1;
std::mutex clientIdMutex;

void PrintMatrixSafe(int clientId, const std::vector<std::vector<int>>& matrix)
{
    std::lock_guard<std::mutex> lock(coutMutex);

    int n = (int)matrix.size();

    std::cout << "\n[Client " << clientId << "] Matrix:\n";

    for (int i = 0; i < n; i++) {
        std::cout << "[Client " << clientId << "] ";
        for (int j = 0; j < n; j++) {
            std::cout << matrix[i][j] << " ";
        }
        std::cout << "\n";
    }
}

void PrintSafe(int clientId, const std::string& text)
{
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "[Client " << clientId << "] " << text << "\n";
}

void PrintServerSafe(const std::string& text)
{
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << text << "\n";
}

int GetNextClientId()
{
    std::lock_guard<std::mutex> lock(clientIdMutex);

    int id = nextClientId;
    nextClientId++;

    return id;
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

void HandleClient(SOCKET client_socket, int clientId)
{
    PrintSafe(clientId, "connected in thread");

    std::vector<std::vector<int>> matrix;
    int n = 0;
    int threadCount = 1;

    bool hasData = false;
    bool isDone = false;
    bool isRunning = false;

    std::mutex stateMutex;
    std::thread calculationThread;

    while (true)
    {
        int command = recvInt(client_socket);

        if (command == -1) {
            PrintSafe(clientId, "disconnected unexpectedly");
            break;
        }

        if (command == CMD_SEND_DATA)
        {
            n = recvInt(client_socket);
            threadCount = recvInt(client_socket);

            matrix = std::vector<std::vector<int>>(n, std::vector<int>(n));

            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    matrix[i][j] = recvInt(client_socket);
                }
            }

            {
                std::lock_guard<std::mutex> lock(stateMutex);
                hasData = true;
                isDone = false;
                isRunning = false;
            }

            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "\n[Client " << clientId << "] CMD_SEND_DATA received\n";
                std::cout << "[Client " << clientId << "] Matrix size N = " << n << "\n";
                std::cout << "[Client " << clientId << "] Thread count = " << threadCount << "\n";
            }

            PrintMatrixSafe(clientId, matrix);

            sendInt(client_socket, RESP_OK);
        }
        else if (command == CMD_GET_STATUS)
        {
            int response;

            {
                std::lock_guard<std::mutex> lock(stateMutex);

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
            }

            sendInt(client_socket, response);

            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "[Client " << clientId << "] CMD_GET_STATUS received, response = " << response << "\n";
            }
        }
        else if (command == CMD_START)
        {
            bool canStart;

            {
                std::lock_guard<std::mutex> lock(stateMutex);
                canStart = hasData && !isRunning;
            }

            if (!canStart) {
                sendInt(client_socket, RESP_NO_DATA);
            }
            else {
                PrintSafe(clientId, "CMD_START received");

                {
                    std::lock_guard<std::mutex> lock(stateMutex);
                    isRunning = true;
                    isDone = false;
                }

                if (calculationThread.joinable()) {
                    calculationThread.join();
                }

                calculationThread = std::thread([&matrix, threadCount, &isRunning, &isDone, &stateMutex, clientId]() {
                    WaitByTimer(3);

                    MirrorRightToLeftParallel(matrix, threadCount);

                    PrintMatrixSafe(clientId, matrix);

                    {
                        std::lock_guard<std::mutex> lock(stateMutex);
                        isRunning = false;
                        isDone = true;
                    }
                    });

                sendInt(client_socket, RESP_OK);
            }
        }
        else if (command == CMD_GET_RESULT)
        {
            bool ready;

            {
                std::lock_guard<std::mutex> lock(stateMutex);
                ready = isDone;
            }

            if (!ready) {
                sendInt(client_socket, RESP_IN_PROGRESS);
            }
            else {
                if (calculationThread.joinable()) {
                    calculationThread.join();
                }

                sendInt(client_socket, RESP_OK);

                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < n; j++) {
                        sendInt(client_socket, matrix[i][j]);
                    }
                }

                PrintSafe(clientId, "result matrix sent to client");
            }
        }
        else if (command == CMD_DISCONNECT)
        {
            PrintSafe(clientId, "CMD_DISCONNECT received");
            break;
        }
        else {
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "[Client " << clientId << "] Unknown command: " << command << "\n";
            }

            sendInt(client_socket, RESP_ERROR);
        }
    }

    if (calculationThread.joinable()) {
        calculationThread.join();
    }

    closesocket(client_socket);
    PrintSafe(clientId, "thread finished");
}

int main()
{
    char buff[1024];

    PrintServerSafe("TCP SERVER STARTING...");

    if (WSAStartup(0x0202, (WSADATA*)&buff[0])) {
        PrintServerSafe("WSAStartup error");
        return -1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == INVALID_SOCKET) {
        PrintServerSafe("Socket creation error");
        WSACleanup();
        return -1;
    }

    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(SERVER_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&local_addr, sizeof(local_addr))) {
        PrintServerSafe("Bind error");
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    if (listen(server_socket, 5)) {
        PrintServerSafe("Listen error");
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    PrintServerSafe("Server started on port 1111");
    PrintServerSafe("Waiting for clients...");

    while (true)
    {
        SOCKET client_socket = accept(server_socket, NULL, NULL);

        if (client_socket == INVALID_SOCKET) {
            PrintServerSafe("Accept error");
            continue;
        }

        int clientId = GetNextClientId();

        std::thread clientThread(HandleClient, client_socket, clientId);
        clientThread.detach();
    }

    closesocket(server_socket);
    WSACleanup();

    return 0;
}