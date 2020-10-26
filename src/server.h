#ifndef SERVER_H
#define SERVER_H

void *handle_client(void *arg);
int check_auth(char *username, char *password);
void deny_auth(int connfd);

#endif

