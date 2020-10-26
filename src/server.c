// Data & Communication Networks
// Project 1 - Socket Programming
// Peter Fabinski (pnf9945)
// TigerS - server

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "common.h"
#include "server.h"

#define MAX_USERS 128

int main(void) {

  int err;

  // get the addrinfo for listening on the local machine
  struct addrinfo *hostinfo;

  // hints tell getaddrinfo what kind of address we want
  struct addrinfo hints = {0};
  hints.ai_flags = AI_PASSIVE;      // we want to listen
  hints.ai_family = AF_INET;        // IPv4
  hints.ai_socktype = SOCK_STREAM;  // TCP
  hints.ai_protocol = IPPROTO_TCP;  // TCP

  // call getaddrinfo
  err = getaddrinfo(NULL, STR(FTP_PORT), &hints, &hostinfo);
  if (err) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
    return -1;
  }

  // get a socket for listening with the first provided address
  int listenfd = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);
  if (listenfd == -1) {
    fprintf(stderr, "socket: %s\n", strerror(errno));
    return -1;
  }

  // bind to the address
  err = bind(listenfd, hostinfo->ai_addr, hostinfo->ai_addrlen);
  if (err) {
    fprintf(stderr, "bind: %s\n", strerror(errno));
    return -1;
  }

  // listen for connections
  err = listen(listenfd, 128);
  if (err) {
    fprintf(stderr, "listen: %s\n", strerror(errno));
    return -1;
  }

  // open the user database
  FILE *users = fopen("users.txt", "r");
  if (!users) {
    fprintf(stderr, "fopen: %s\n", strerror(errno));
    fprintf(stdout, "Failed to open user database.\n");
    return -1;
  }

  printf("Now accepting connections.\n");
  // accept connections and handle the work for each one
  for (;;) {
    int connfd = accept(listenfd, NULL, NULL); // don't care about their address
    printf("Accepted connection.\n");

    // create a thread for this connection
    pthread_t thread;
    err = pthread_create(&thread, NULL, handle_client, (void *) (intptr_t) connfd);
    if (err) {
      fprintf(stderr, "pthread_create: %s\n", strerror(err));
    }
    // let it go off on its own
    pthread_detach(thread);
  }

  printf("Quitting\n");
  return 0;
}

void *handle_client(void *arg) {
  int connfd = (intptr_t) arg;
  // receive the initial request from the client
  struct ftp_auth_request req = {0};
  ssize_t received = recv(connfd, &req, sizeof(req), MSG_WAITALL);
  if (received == 0) {
    fprintf(stderr, "Connection closed during authentication request.\n");
    return (void *)-1;
  } else if (received == -1) {
    fprintf(stderr, "recv: %s\n", strerror(errno));
    return (void *)-1;
  } else if ((size_t) received < sizeof(req)) {
    fprintf(stderr, "Not enough data received for authentication request.\n");
    return (void *)-1;
  }

  req.type = ntohl(req.type);
  req.username_len = ntohl(req.username_len);
  req.password_len = ntohl(req.password_len);

  char *username = malloc(req.username_len + 1);
  if (username == NULL) {
    fprintf(stderr, "Out of memory.\n");
  }
  username[req.username_len] = '\0';

  received = recv(connfd, username, req.username_len, MSG_WAITALL);
  if (received == 0) {
    fprintf(stderr, "Connection closed during receive of username.\n");
    return (void *)-1;
  } else if (received == -1) {
    fprintf(stderr, "recv: %s\n", strerror(errno));
    return (void *)-1;
  } else if ((size_t) received < req.username_len) {
    fprintf(stderr, "Not enough data received for username.\n");
    return (void *)-1;
  }

  char *password = malloc(req.password_len + 1);
  if (password == NULL) {
    fprintf(stderr, "Out of memory.\n");
  }
  password[req.password_len] = '\0';

  received = recv(connfd, password, req.password_len, MSG_WAITALL);
  if (received == 0) {
    fprintf(stderr, "Connection closed during receive of password.\n");
    return (void *)-1;
  } else if (received == -1) {
    fprintf(stderr, "recv: %s\n", strerror(errno));
    return (void *)-1;
  } else if ((size_t) received < req.password_len) {
    fprintf(stderr, "Not enough data received for password.\n");
    return (void *)-1;
  }

  printf("username: %s, password: %s\n", username, password);

  return 0;
}

