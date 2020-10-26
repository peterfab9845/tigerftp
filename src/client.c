// Data & Communication Networks
// Project 1 - Socket Programming
// Peter Fabinski (pnf9945)
// TigerC - client

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "client.h"

#define CMDLEN 255

int main(void) {
  int line_max;

  // find max line length
  if (LINE_MAX >= CMDLEN) {
    line_max = CMDLEN;
  } else {
    long limit = sysconf(_SC_LINE_MAX);
    if (limit < 0 || limit > CMDLEN) {
      line_max = CMDLEN;
    } else {
      line_max = (int)limit;
    }
  }

  // alloc memory for the line
  char *line = malloc(line_max + 1);
  if (line == NULL) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }

  enum ftp_state state = IDLE;
  enum ftp_command cmd;

  char *hostname;
  char *username;
  char *password;
  char *filename;

  while (1) {
    printf("TigerC> ");
    // get a line
    if (fgets(line, line_max+1, stdin) == NULL) {
      if (ferror(stdin)) {
        fprintf(stderr, "Error getting command: %s\n", strerror(errno));
        exit(1);
      } else {
        printf("exit\n"); // fake command for EOF
        break;
      }
    }

    // check for too long line
    if (strchr(line, '\n') == NULL) {
      fprintf(stderr, "Input line too long.\n");
      // clear the rest of the input
      while (strchr(line, '\n') == NULL) {
        fgets(line, line_max+1, stdin);
      }
      continue;
    }

    // interpret the command
    int err = parse_cmd(line, &cmd, &hostname, &username, &password, &filename);
    if (err) {
      // error parsing command, start the loop again
      continue;
    }
    // do what the command asks

    int sockfd;

    // **** tconnect command
    if (cmd == TCONNECT) {
      if (state != IDLE) {
        fprintf(stdout, "You are already connected.\n");
        continue;
      }
      // connect to the given server
      sockfd = open_conn(hostname);
      if (sockfd == -1) {
        fprintf(stdout, "Could not connect to server.\n");
        continue;
      }

      // authenticate ourselves
      err = do_auth(sockfd, username, password);
      if (err == 1) {
        fprintf(stdout, "Incorrect username or password.\n");
        close_conn(sockfd);
        continue;
      } else if (err == -1) {
        fprintf(stdout, "Error occurred during authentication.\n");
        close_conn(sockfd);
        continue;
      }

      // connected and authenticated successfully
      state = CONNECTED;
      printf("Connected successfully.\n");

    // **** tget command
    } else if (cmd == TGET) {
      if (state != CONNECTED) {
        fprintf(stdout, "You need to connect first.\n");
        continue;
      }

      err = do_get(sockfd, filename);
      if (err) {
        printf("Unable to complete get request.\n");
      }

    // **** tput command
    } else if (cmd == TPUT) {
      if (state != CONNECTED) {
        fprintf(stdout, "You need to connect first.\n");
        continue;
      }

      err = do_put(sockfd, filename);
      if (err) {
        printf("Unable to complete put request.\n");
      }

    // **** exit command
    } else if (cmd == EXIT) {
      // close down the client
      if (state == CONNECTED) {
        err = send_close(sockfd);
        if (err) {
          fprintf(stderr, "Failed to close gracefully.\n");
        }
        err = close_conn(sockfd);
        if (err) {
          fprintf(stderr, "Error closing connection.\n");
        }
      }
      break;

    // **** unknown command
    } else {
      // shouldn't get here, parse_cmd deals with this
      fprintf(stdout, "Unknown command.\n");
    }
  }

  // clean up input stuff and show message
  free(line);
  printf("Quitting\n");
  return 0;
}

// connect to the given server
// return: socket file descriptor
// hostname: the hostname of the server to connect to
int open_conn(char *hostname) {

  int err;

  // parse the hostname to get the addrinfo
  struct addrinfo *hostinfo;

  // hints tell getaddrinfo what kind of address we want
  struct addrinfo hints = {0};
  hints.ai_flags = 0;               // nothing special
  hints.ai_family = AF_INET;        // IPv4
  hints.ai_socktype = SOCK_STREAM;  // TCP
  hints.ai_protocol = IPPROTO_TCP;  // TCP

  err = getaddrinfo(hostname, STR(FTP_PORT), &hints, &hostinfo);
  if (err) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
    return -1;
  }

  // create a socket with the first address response
  int sockfd = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);
  if (sockfd == -1) {
    fprintf(stderr, "socket: %s\n", strerror(errno));
    return -1;
  }

  // connect to the specified server
  err = connect(sockfd, hostinfo->ai_addr, hostinfo->ai_addrlen);
  if (err) {
    fprintf(stderr, "connect: %s\n", strerror(errno));
    return -1;
  }

  freeaddrinfo(hostinfo);
  
  return sockfd;
}

// send an authentication request to the server
// return: authentication result (-1 error, 0 success, 1 denied)
// sockfd: socket file descriptor
// user: username to try
// pass: password to try
int do_auth(int sockfd, char *user, char *pass) {

  struct ftp_auth_request req = {0};
  req.type = htonl(AUTH_REQ);
  req.username_len = htonl(strlen(user));
  req.password_len = htonl(strlen(pass));

  int err;

  err = send_all(sockfd, &req, sizeof(req));
  if (err == -1) {
    fprintf(stderr, "Error sending auth request.\n");
    return -1;
  }
  send_all(sockfd, user, strlen(user));
  if (err == -1) {
    fprintf(stderr, "send: %s\n", strerror(errno));
    return -1;
  }
  send_all(sockfd, pass, strlen(pass));
  if (err == -1) {
    fprintf(stderr, "send: %s\n", strerror(errno));
    return -1;
  }

  struct ftp_auth_response resp = {0};
  ssize_t received = recv(sockfd, &resp, sizeof(resp), MSG_WAITALL);
  if (received == 0) {
    fprintf(stderr, "Connection closed during authentication.\n");
    return -1;
  } else if (received == -1) {
    fprintf(stderr, "recv: %s\n", strerror(errno));
    return -1;
  } else if ((size_t)received < sizeof(resp)) {
    fprintf(stderr, "Not enough data received during authentication.\n");
    return -1;
  }

  resp.type = ntohl(resp.type);

  if (resp.type != AUTH_RESP) {
    fprintf(stderr, "Sequence error: expected AUTH_RESP\n");
    return -1;
  }
  resp.result = ntohl(resp.result);
  if (resp.result == SUCCESS) {
    return 0;
  } else if (resp.result == FAILURE) {
    return 1;
  }
  return -1;
}

// send a get request to the server
// return: get result
// filename: the filename to get from the server
int do_get(int sockfd, char *filename) {
  // make the get request
  struct ftp_file_request req = {0};
  req.type = htonl(GET);
  req.filename_len = htonl(strlen(filename));

  int err = send_all(sockfd, &req, sizeof(req));
  if (err == -1) {
    fprintf(stderr, "Error sending get request.\n");
    return -1;
  }

  err = send_all(sockfd, filename, strlen(filename));
  if (err == -1) {
    fprintf(stderr, "Error sending get filename.\n");
    return -1;
  }

  // get server response
  struct ftp_file_response resp = {0};

  ssize_t received = recv(sockfd, &resp, sizeof(resp), MSG_WAITALL);
  if (received == 0) {
    fprintf(stderr, "Connection closed during response.\n");
    return -1;
  } else if (received == -1) {
    fprintf(stderr, "recv: %s\n", strerror(errno));
    return -1;
  } else if ((size_t)received < sizeof(resp)) {
    fprintf(stderr, "Not enough data received during response.\n");
    return -1;
  }

  // check response results
  resp.type = ntohl(resp.type);

  if (resp.type != GET) {
    fprintf(stderr, "Sequence error: expected GET\n");
    return -1;
  }

  resp.result = ntohl(resp.result);
  if (resp.result != SUCCESS) {
    fprintf(stderr, "Server failed to read file.\n");
    return -1;
  }
  // get the file size
  resp.filesize = ntohl(resp.filesize);

  // create new file for writing
  FILE *file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "Failed to open requested file for writing.\n");
    return -1;
  }

  char buf[512];

  size_t num_received = 0;
  int to_receive;
  while (num_received < resp.filesize) {
    // determine how much to receive
    if (resp.filesize - num_received >= sizeof(buf)) {
      to_receive = sizeof(buf);
    } else {
      to_receive = resp.filesize - num_received;
    }
    // receive and write to the file
    ssize_t received = recv(sockfd, buf, to_receive, 0);
    if (received == 0) {
      fprintf(stderr, "Connection closed.\n");
      return -1;
    } else if (received == -1) {
      fprintf(stderr, "recv: %s\n", strerror(errno));
      return -1;
    }
    fwrite(buf, 1, received, file);
    if (ferror(file)) {
      fprintf(stderr, "fwrite: %s\n", strerror(errno));
      return -1;
    }
    num_received += received;
  }

  err = fclose(file);
  if (err) {
    fprintf(stderr, "fclose: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}

// send a put request to the server
// return: put result
// filename: the filename to upload to the server
int do_put(int sockfd, char *filename) {
  struct ftp_file_request req = {0};
  req.type = htonl(PUT);
  req.filename_len = htonl(strlen(filename));

  int err = send_all(sockfd, &req, sizeof(req));
  if (err == -1) {
    fprintf(stderr, "Error sending put request.\n");
    return -1;
  }

  err = send_all(sockfd, filename, strlen(filename));
  if (err == -1) {
    fprintf(stderr, "Error sending put filename.\n");
    return -1;
  }


  char buf[512];

  return 0;
}

// parse the input and provide the command
// return: parse/command status
// line: the line containing command(s) to parse
// cmd: set to the command parsed
// hostname: set to the provided hostname, if any
// username: set to the provided username, if any
// password: set to the provided password, if any
// filename: set to the provided filename, if any
int parse_cmd(char *line, enum ftp_command *cmd, char **hostname,
    char **username, char **password, char **filename) {

  // Parse the line. First, separate the command.
  static char *strtok_state;
  char *token;
  token = strtok_r(line, " \r\n", &strtok_state);

  // check for no command
  if (token == NULL) {
    // go to next line for new prompt
    printf("\n");
    // start over
    return -1;
  }
  if (strcmp(token, "help") == 0) {
    // help command, list commands
    usage();
    return -1;
  } else if (strcmp(token, "tconnect") == 0) {
    // tconnect command, manage the connection to the server
    *cmd = TCONNECT;
    token = strtok_r(NULL, " \n", &strtok_state);
    if (token) {
      *hostname = token;
    } else {
      fprintf(stdout, "tconnect requires a host to connect to.\n");
      return -1;
    }
    token = strtok_r(NULL, " \n", &strtok_state);
    if (token) {
      *username = token;
    } else {
      fprintf(stdout, "tconnect requires a username.\n");
      return -1;
    }
    token = strtok_r(NULL, " \n", &strtok_state);
    if (token) {
      *password = token;
    } else {
      fprintf(stdout, "tconnect requires a password.\n");
      return -1;
    }
  } else if (strcmp(token, "tget") == 0) {
    // tget command, print the filename
    *cmd = TGET;
    token = strtok_r(NULL, "\n", &strtok_state);
    if (token) {
      *filename = token;
    } else {
      fprintf(stdout, "tget requires a filename.\n");
      return -1;
    }
  } else if (strcmp(token, "tput") == 0) {
    // tput command, print the filename
    *cmd = TPUT;
    token = strtok_r(NULL, "\n", &strtok_state);
    if (token) {
      *filename = token;
    } else {
      fprintf(stdout, "tput requires a filename.\n");
      return -1;
    }
  } else if (strcmp(token, "exit") == 0) {
    // exit command
    *cmd = EXIT;
  } else {
    fprintf(stdout, "Command not recognized: %s\n", token);
    return -1;
  }
  return 0;
}
// print usage message
void usage(void) {
  printf("Commands:\n");
  printf("  tconnect <ip> <user> <pass>\n");
  printf("  tget <filename>\n");
  printf("  tput <filename>\n");
  printf("  help\n");
}

