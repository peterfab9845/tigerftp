# Programs and sources
SRC_DIR = src/

SERVER_SRC = $(SRC_DIR)server.c
SERVER_H = $(SRC_DIR)server.h
SERVER_DIR = server/
SERVER_NAME = TigerS
SERVER_BIN = $(SERVER_DIR)$(SERVER_NAME)

CLIENT_SRC = $(SRC_DIR)client.c
CLIENT_H = $(SRC_DIR)client.h
CLIENT_DIR = client/
CLIENT_NAME = TigerC
CLIENT_BIN = $(CLIENT_DIR)$(CLIENT_NAME)

COMMON_SRC = $(SRC_DIR)common.c
COMMON_H = $(SRC_DIR)common.h

# result files
TEST_SCRIPT = test.sh

# compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g
PTHREAD_FLAG = -lpthread

# disable echoing commands for nicer output
.SILENT:

# default - build the programs
.PHONY: all
all: $(SERVER_BIN) $(CLIENT_BIN)

# compile modules and programs
$(SERVER_BIN): $(SERVER_SRC) $(COMMON_H) $(SERVER_H)
	$(CC) $(CFLAGS) $(PTHREAD_FLAG) $(SERVER_SRC) $(COMMON_SRC) -o $@

$(CLIENT_BIN): $(CLIENT_SRC) $(COMMON_H) $(CLIENT_H)
	$(CC) $(CFLAGS) $(CLIENT_SRC) $(COMMON_SRC) -o $@

# run the client program
.PHONY: run_client
run_client: $(CLIENT_BIN)
	cd $(CLIENT_DIR); ./$(CLIENT_NAME)

# run the server program
.PHONY: run_server
run_server: $(SERVER_BIN)
	cd $(SERVER_DIR); ./$(SERVER_NAME)

# generate test files
.PHONY: gen
gen:
	chmod a+x gen.sh
	./gen.sh

# remove the test files
.PHONY: cleangen
cleangen:
	-rm -f $(SERVER_DIR)down*.txt
	-rm -f $(SERVER_DIR)upload*.txt
	-rm -f $(CLIENT_DIR)down*.txt
	-rm -f $(CLIENT_DIR)upload*.txt

# run the test script
.PHONY: test
test: $(CLIENT_BIN)
	chmod a+x $(CLIENT_DIR)/test.sh
	cd $(CLIENT_DIR); ./test.sh

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
	echo "gen:        generate test files"
	echo "cleangen:   remove test files"
	echo "test:       run test.sh"
	echo "clean:      remove output and binary files"
	echo "help:       show this help"
