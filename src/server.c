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

  // set SO_REUSEPORT so we can restart the server immediately (not necessary for clean exits)
  int optval = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

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
  int err;

  int connfd = (intptr_t) arg;
  // receive the initial request from the client
  struct ftp_auth_request req = {0};
  ssize_t received = recv(connfd, &req, sizeof(req), MSG_WAITALL);
  if (received == 0) {
    fprintf(stderr, "Connection closed.\n");
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

  // check request type
  if (req.type != AUTH_REQ) {
    fprintf(stderr, "Sequence error: expected AUTH_REQ\n");
    err = close_conn(connfd);
    if (err) {
      fprintf(stderr, "Error closing connection.\n");
    }
  }

  // make space for and receive the username
  char *username = malloc(req.username_len + 1);
  if (username == NULL) {
    fprintf(stderr, "Out of memory.\n");
  }
  username[req.username_len] = '\0';

  // make space for and receive the password
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

  int auth_result = check_auth(username, password);
  if (auth_result == 1) {
    // good password, send the acknowledge with success
    struct ftp_auth_response resp = {0};
    resp.type = htonl(AUTH_RESP);
    resp.result = htonl(SUCCESS);

    int err = send_all(connfd, &resp, sizeof(resp));
    if (err == -1) {
      fprintf(stderr, "Error sending auth response.\n");
    }
  } else if (auth_result == 0) {
    // bad password, deny and close
    deny_auth(connfd);
  } else {
    // error
    struct ftp_auth_response resp = {0};
    resp.type = htonl(AUTH_RESP);
    resp.result = htonl(UNKNOWN);

    int err = send_all(connfd, &resp, sizeof(resp));
    if (err == -1) {
      fprintf(stderr, "Error sending auth response.\n");
    }
  }

  // process user requests


  return 0;
}

// check if a username and password are in the database
// return: 0 if no, 1 if yes, -1 if error
int check_auth(char *username, char *password) {
  // open the user database
  FILE *users = fopen("users.txt", "r");
  if (!users) {
    fprintf(stderr, "fopen: %s\n", strerror(errno));
    fprintf(stdout, "Failed to open user database.\n");
    return 0;
  }

  // check username and password
  char line[256];
  for (;;) {
    if (fgets(line, 256, users) == NULL) {
      if (ferror(users)) {
        fprintf(stderr, "Error getting users line: %s\n", strerror(errno));
        return -1;
      } else {
        // reached EOF with no match. Deny auth
        return 0;
      }
    }
    // check the line
    static char *strtok_state;
    char *token;
    token = strtok_r(line, " \r\n", &strtok_state);
    if (strcmp(token, username) != 0) {
      continue;
    }
    // username exists, check password
    token = strtok_r(NULL, " \r\n", &strtok_state);
    if (strcmp(token, password) != 0) {
      // incorrect password
      continue;
    }
    // both were correct, return positively
    return 1;
  }

  // shouldn't get here
  return -1;
}

// send bad-auth response and close connection
void deny_auth(int connfd) {
  struct ftp_auth_response resp = {0};
  resp.type = htonl(AUTH_RESP);
  resp.result = htonl(FAILURE);

  int err = send_all(connfd, &resp, sizeof(resp));
  if (err == -1) {
    fprintf(stderr, "Error sending auth response.\n");
  }
  err = close_conn(connfd);
  if (err) {
    fprintf(stderr, "Error closing connection.\n");
  }
}

