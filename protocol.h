#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define SERVER_PORT 1111
#define SERVER_ADDR "127.0.0.1"

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

#pragma comment(lib, "ws2_32.lib")