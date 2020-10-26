#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "common.h"

// keep calling send until finished or error
// return: -1 on error, bytes sent otherwise
// sockfd: socket file descriptor
// buf: the data to send
// len: length of the data
int send_all(int sockfd, void *buf, int len) {
  int sent = 0;
  while (len > 0) {
    int n = send(sockfd, buf + sent, len, 0);
    if (n == -1) {
      fprintf(stderr, "send: %s\n", strerror(errno));
      return -1;
    }
    sent += n;
    len -= n;
  }
  return sent;
}

// close a connection
// return: close status
// sockfd: socket file descriptor to close
int close_conn(int sockfd) {
  int err = close(sockfd);
  if (err) {
    fprintf(stderr, "close: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

