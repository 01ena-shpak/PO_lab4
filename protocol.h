#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define SERVER_PORT 1111
#define SERVER_ADDR "127.0.0.1"

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <chrono>

enum Command
{
    CMD_SEND_DATA = 1,
    CMD_START = 2,
    CMD_GET_STATUS = 3,
    CMD_GET_RESULT = 4,
    CMD_DISCONNECT = 5
};

enum Response
{
    RESP_OK = 1,
    RESP_ERROR = 2,
    RESP_IN_PROGRESS = 3,
    RESP_DONE = 4,
    RESP_NO_DATA = 5
};

inline int recvInt(SOCKET s)
{
    int net;
    int result = recv(s, (char*)&net, sizeof(net), 0);

    if (result <= 0) {
        return -1;
    }

    return ntohl(net);
}

inline void sendInt(SOCKET s, int value)
{
    int net = htonl(value);
    send(s, (char*)&net, sizeof(net), 0);
}

inline void PrintMatrix(const std::vector<std::vector<int>>& matrix)
{
    int n = (int)matrix.size();

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::cout << matrix[i][j] << " ";
        }
        std::cout << "\n";
    }
}

inline void WaitByTimer(int seconds)
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    while (true) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

        int elapsed = (int)std::chrono::duration_cast<std::chrono::seconds>(now - start).count();

        if (elapsed >= seconds) {
            break;
        }
    }
}

#pragma comment(lib, "ws2_32.lib")