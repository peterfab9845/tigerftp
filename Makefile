# Programs and sources
SRC_DIR = src/

SERVER_SRC = $(SRC_DIR)server.c
SERVER_H = $(SRC_DIR)server.h
SERVER_DIR = server/
SERVER_BIN = $(SERVER_DIR)TigerS

CLIENT_SRC = $(SRC_DIR)client.c
CLIENT_H = $(SRC_DIR)client.h
CLIENT_DIR = client
CLIENT_BIN = $(CLIENT_DIR)TigerC

COMMON_H = $(SRC_DIR)common.h

# result files
TEST_SCRIPT = test.sh

# compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g

# disable echoing commands for nicer output
.SILENT:

# default - build the programs
.PHONY: all
all: $(SERVER_BIN) $(CLIENT_BIN)

# compile modules and programs
$(SERVER_BIN): $(SERVER_SRC) $(COMMON_H) $(SERVER_H)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $@

$(CLIENT_BIN): $(CLIENT_SRC) $(COMMON_H) $(CLIENT_H)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $@

# run the client program
.PHONY: run_client
run_client: $(CLIENT_BIN)
	cd $(CLIENT_DIR); ./$(CLIENT_BIN)

# run the server program
.PHONY: run_server
run_server: $(SERVER_BIN)
	cd $(SERVER_DIR); ./$(SERVER_BIN)

# run the test script
.PHONY: test
test: $(CLIENT_BIN)
	chmod a+x client/test.sh
	cd client; ./test.sh

# clean up binaries and output files
.PHONY: clean
clean:
	-rm -f $(SERVER_BIN) $(CLIENT_BIN)

# help target - lists all targets
.PHONY: help
help:
	echo "Targets:"
	echo "all:        build TigerS and TigerC"
	echo "run_server: run TigerS"
	echo "run_client: run TigerC"
	echo "test:       run test.sh"
	echo "clean:      remove output and binary files"
	echo "help:       show this help"
