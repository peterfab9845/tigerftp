// Data & Communication Networks
// Project 1 - Socket Programming
// Peter Fabinski (pnf9945)
// TigerS - server

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

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

  freeaddrinfo(hostinfo);

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
    printf("Connection opened.\n");

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
  struct ftp_auth_request auth_req = {0};
  ssize_t received = recv(connfd, &auth_req, sizeof(auth_req), MSG_WAITALL);
  if (received == 0) {
    fprintf(stderr, "Connection closed.\n");
    close_conn(connfd);
    return (void *)-1;
  } else if (received == -1) {
    fprintf(stderr, "recv: %s\n", strerror(errno));
    close_conn(connfd);
    return (void *)-1;
  } else if ((size_t) received < sizeof(auth_req)) {
    fprintf(stderr, "Not enough data received for authentication request.\n");
    close_conn(connfd);
    return (void *)-1;
  }

  auth_req.type = ntohl(auth_req.type);

  // check request type
  if (auth_req.type != AUTH_REQ) {
    fprintf(stderr, "Sequence error: expected AUTH_REQ\n");
    err = close_conn(connfd);
    if (err) {
      fprintf(stderr, "Error closing connection.\n");
    }
  }

  auth_req.username_len = ntohl(auth_req.username_len);
  auth_req.password_len = ntohl(auth_req.password_len);

  // make space for and receive the username
  char *username = malloc(auth_req.username_len + 1);
  if (username == NULL) {
    fprintf(stderr, "Out of memory.\n");
  }
  username[auth_req.username_len] = '\0';

  // make space for and receive the password
  received = recv(connfd, username, auth_req.username_len, MSG_WAITALL);
  if (received == 0) {
    fprintf(stderr, "Connection closed during receive of username.\n");
    close_conn(connfd);
    return (void *)-1;
  } else if (received == -1) {
    fprintf(stderr, "recv: %s\n", strerror(errno));
    close_conn(connfd);
    return (void *)-1;
  } else if ((size_t) received < auth_req.username_len) {
    fprintf(stderr, "Not enough data received for username.\n");
    close_conn(connfd);
    return (void *)-1;
  }

  char *password = malloc(auth_req.password_len + 1);
  if (password == NULL) {
    fprintf(stderr, "Out of memory.\n");
  }
  password[auth_req.password_len] = '\0';

  received = recv(connfd, password, auth_req.password_len, MSG_WAITALL);
  if (received == 0) {
    fprintf(stderr, "Connection closed during receive of password.\n");
    close_conn(connfd);
    return (void *)-1;
  } else if (received == -1) {
    fprintf(stderr, "recv: %s\n", strerror(errno));
    close_conn(connfd);
    return (void *)-1;
  } else if ((size_t) received < auth_req.password_len) {
    fprintf(stderr, "Not enough data received for password.\n");
    close_conn(connfd);
    return (void *)-1;
  }

  int auth_result = check_auth(username, password);
  if (auth_result == 1) {
    // good password, send the acknowledge with success
    struct ftp_auth_response resp = {0};
    resp.type = htonl(AUTH_RESP);
    resp.result = htonl(SUCCESS);

    printf("Successful login by: %s\n", username);

    int err = send_all(connfd, &resp, sizeof(resp));
    if (err == -1) {
      fprintf(stderr, "Error sending auth response.\n");
    }
  } else if (auth_result == 0) {
    // bad password, deny and close
    printf("Bad password provided by: %s\n", username);
    deny_auth(connfd);
    return (void *) 0;
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
  free(username);
  free(password);

  // process user requests
  for (;;) {
    struct ftp_file_request file_req = {0};

    received = recv(connfd, &file_req, sizeof(file_req), MSG_WAITALL);
    if (received == 0) {
      fprintf(stderr, "Connection closed.\n");
      close_conn(connfd);
      return (void *)-1;
    } else if (received == -1) {
      fprintf(stderr, "recv: %s\n", strerror(errno));
      close_conn(connfd);
      return (void *)-1;
    } else if ((size_t) received < sizeof(file_req)) {
      fprintf(stderr, "Not enough data received for file request.\n");
      close_conn(connfd);
      return (void *)-1;
    }

    file_req.type = ntohl(file_req.type);
    // if it is an END request, nothing more to read. Close connection.
    if (file_req.type == END) {
      err = close_conn(connfd);
      if (err) {
        fprintf(stderr, "Error closing connection.\n");
        return (void *) -1;
      }
      printf("Connection closed.\n");
      return (void *) 0;
    } else if (file_req.type != GET && file_req.type != PUT) {
      // unknown request type
      fprintf(stderr, "Sequence error: expected GET, PUT, or END\n");
      err = close_conn(connfd);
      if (err) {
        fprintf(stderr, "Error closing connection.\n");
      }
    }

    // otherwise, it's a GET or PUT. Get the filename.
    file_req.filename_len = ntohl(file_req.filename_len);
    // and size, if needed
    file_req.filesize = ntohl(file_req.filesize);

    char *filename = malloc(file_req.filename_len + 1);
    filename[file_req.filename_len] = '\0';

    received = recv(connfd, filename, file_req.filename_len, MSG_WAITALL);
    if (received == 0) {
      fprintf(stderr, "Connection closed.\n");
      close_conn(connfd);
      return (void *)-1;
    } else if (received == -1) {
      fprintf(stderr, "recv: %s\n", strerror(errno));
      close_conn(connfd);
      return (void *)-1;
    } else if ((size_t) received < file_req.filename_len) {
      fprintf(stderr, "Not enough data received for filename.\n");
      close_conn(connfd);
      return (void *)-1;
    }

    // set up a buffer for file operations
    char buf[512];

    // *********** GET REQUEST

    if (file_req.type == GET) {
      // send the file to the client
      printf("GET %s\n", filename);

      FILE *file = fopen(filename, "r");
      if (!file) {
        fprintf(stderr, "Failed to open requested file for reading.\n");
        if (send_fail(connfd, GET)) {
          return (void *)-1;
        }
        continue;
      }
      // we have a good FILE handle - file exists
      // determine the size and send to client

      struct stat stats;
      err = stat(filename, &stats);
      if (err) {
        fprintf(stderr, "stat: %s\n", strerror(errno));
        // tell the client there was a problem
        if (send_fail(connfd, GET)) {
          return (void *)-1;
        }
        continue;
      }
      off_t filesize = stats.st_size;
      // send file size in a successful response
      struct ftp_file_response resp = {0};
      resp.type = htonl(GET);
      resp.result = htonl(SUCCESS);
      resp.filesize = htonl(filesize);

      err = send_all(connfd, &resp, sizeof(resp));
      if (err == -1) {
        fprintf(stderr, "Error sending filesize.\n");
        close_conn(connfd);
        printf("Connection closed.\n");
        return (void *)-1;
      } 

      // send the file
      for (;;) {
        size_t num_read = fread(buf, 1, sizeof(buf), file);
        if (num_read != 0) {
          err = send_all(connfd, buf, num_read);
          if (err == -1) {
            fprintf(stderr, "Error sending file data.\n");
            close_conn(connfd);
            printf("Connection closed.\n");
            return (void *)-1;
          } 
        } else {
          if (ferror(file)) {
            fprintf(stderr, "fread: %s\n", strerror(errno));
            close_conn(connfd);
            printf("Connection closed.\n");
            return (void *)-1;
          } else {
            // read 0 and no ferror, so we're finished
            break;
          }
        }
      }
      // done sending file
      err = fclose(file);
      if (err) {
        fprintf(stderr, "fclose: %s\n", strerror(errno));
      }
    // *********** PUT REQUEST

    } else if (file_req.type == PUT) {
      // receive a file from the client
      printf("PUT %s\n", filename);

      // first send a response to the request
      struct ftp_file_response resp = {0};
      resp.type = htonl(PUT);
      resp.result = htonl(SUCCESS);
      resp.filesize = htonl(file_req.filesize); // not needed here, but why not include

      err = send_all(connfd, &resp, sizeof(resp));
      if (err == -1) {
        fprintf(stderr, "Error sending PUT response.\n");
        close_conn(connfd);
        printf("Connection closed.\n");
        return (void *)-1;
      } 

      // create new file for writing
      FILE *file = fopen(filename, "w");
      if (!file) {
        fprintf(stderr, "Failed to open requested file for writing.\n");
        close_conn(connfd);
        printf("Connection closed.\n");
        return (void *)-1;
      }

      char buf[512];

      size_t num_received = 0;
      int to_receive;
      while (num_received < file_req.filesize) {
        // determine how much to receive
        if (file_req.filesize - num_received >= sizeof(buf)) {
          to_receive = sizeof(buf);
        } else {
          to_receive = file_req.filesize - num_received;
        }
        // receive and write to buffer
        ssize_t received = recv(connfd, buf, to_receive, 0);
        if (received == 0) {
          fprintf(stderr, "Connection closed.\n");
          return (void *)-1;
        } else if (received == -1) {
          fprintf(stderr, "recv: %s\n", strerror(errno));
          close_conn(connfd);
          printf("Connection closed.\n");
          return (void *)-1;
        }
        // save the written part of buffer to file
        fwrite(buf, 1, received, file);
        if (ferror(file)) {
          fprintf(stderr, "fwrite: %s\n", strerror(errno));
          close_conn(connfd);
          printf("Connection closed.\n");
          return (void *)-1;
        }
        num_received += received;
      }

      // close the file
      err = fclose(file);
      if (err) {
        fprintf(stderr, "fclose: %s\n", strerror(errno));
        return (void *)-1;
      }
    }
    free(filename);
  }
  return (void *) -1;
}

int send_fail(int connfd, enum ftp_req_type type) {
  struct ftp_file_response resp = {0};
  resp.type = htonl(type);
  resp.result = htonl(FAILURE);
  resp.filesize = 0;

  int err = send_all(connfd, &resp, sizeof(resp));
  if (err == -1) {
    fprintf(stderr, "Error sending command response.\n");
    close_conn(connfd);
    printf("Connection closed.\n");
    return -1;
  }
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
  printf("Connection closed.\n");
}

