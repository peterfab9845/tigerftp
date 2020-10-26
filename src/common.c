#include <sys/socket.h>
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
      return -1;
    }
    sent += n;
    len -= n;
  }
  return sent;
}

