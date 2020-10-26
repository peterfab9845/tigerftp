// Data & Communication Networks
// Project 1 - Socket Programming
// Peter Fabinski (pnf9945)
// TigerC - client

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

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
      if (err) {
        fprintf(stdout, "Incorrect username or password.\n");
        continue;
      }

      state = CONNECTED;

    // **** tget command
    } else if (cmd == TGET) {
      if (state != CONNECTED) {
        fprintf(stdout, "You need to connect first.\n");
        continue;
      }

      err = do_get(sockfd, filename);

    // **** tput command
    } else if (cmd == TPUT) {
      if (state != CONNECTED) {
        fprintf(stdout, "You need to connect first.\n");
        continue;
      }

      err = do_put(sockfd, filename);

    // **** exit command
    } else if (cmd == EXIT) {
      // close down the client
      if (state == CONNECTED) {
        err = close_conn(sockfd);
        if (err) {
          fprintf(stderr, "Could not close connection.\n");
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
  hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;  // TCP
  hints.ai_protocol = IPPROTO_TCP;  // TCP

  err = getaddrinfo(hostname, STR(FTP_PORT), &hints, &hostinfo);
  if (err) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
    return -1;
  }

  // create a socket
  int sockfd = socket(hostinfo->ai_family, hostinfo->ai_socktype, getprotobyname("tcp")->p_proto);
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
  
  return sockfd;
}

// send an authentication request to the server
// return: authentication result
// sockfd: socket file descriptor
// user: username to try
// pass: password to try
int do_auth(int sockfd, char *user, char *pass) {
  printf("tconnect user: %s\n", user);
  printf("tconnect pass: %s\n", pass);

  char test[12] = "hello world";
  send(sockfd, test, 12, 0);
  read(sockfd, test, 5);
  printf("%.5s", test);


  return 0;
}

// send a get request to the server
// return: put result
// filename: the filename to get from the server
int do_get(int sockfd, char *filename) {
  printf("tget filename: %s\n", filename);
  return 0;
}

// send a put request to the server
// return: get result
// filename: the filename to upload to the server
int do_put(int sockfd, char *filename) {
  printf("tput filename: %s\n", filename);

  return 0;
}

// close the connection
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

