#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

constexpr auto MaxConnection = 10;

/* Инициализирует сокет для сервера или клиента, в зависимости от наличия или отсутствия адреса */
int InitSocket(const char* serverAddr, int listenPort);

