// Data & Communication Networks
// Project 1 - Socket Programming
// Peter Fabinski (pnf9945)
// TigerS - server

#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

#include "common.h"
#include "server.h"

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
  }

  // listen for connections
  err = listen(listenfd, 128);
  if (err) {
    fprintf(stderr, "listen: %s\n", strerror(errno));
  }

  // accept connections and handle the work for each one
  printf("Now accepting connections.\n");
  accept(listenfd, NULL, NULL); // don't care about their address

  printf("Quitting\n");
  return 0;
}

void *handle_client(void *arg) {
  return 0;
}

