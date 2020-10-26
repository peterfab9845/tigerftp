#ifndef SERVER_H
#define SERVER_H

#include "common.h"

void *handle_client(void *arg);
int send_fail(int connfd, enum ftp_req_type type);
int check_auth(char *username, char *password);
void deny_auth(int connfd);

#endif

