#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "mpack.h"
#include "protocol.h"
#include "server.h"
#include "game.h"
#include "db.h"
#include "logger.h"

#define dbconninfo "dbname=cardio user=root password=1234 host=localhost port=5433"
#define MAIN_LOG "server.log"

int compare_raw_bytes(char *b1, char *b2, int len);