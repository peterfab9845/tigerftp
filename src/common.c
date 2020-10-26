#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
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

int send_close(int sockfd) {
  struct ftp_file_request req = {0};
  req.type = htonl(END);
  req.filename_len = 0;

  int err = send_all(sockfd, &req, sizeof(req));
  if (err == -1) {
    return -1;
  }
  return 0;
}


// close a connection by socket file descriptor
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

