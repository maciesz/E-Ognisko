APP=E-Ognisko

SRC_EXTENSION=cc

COMMON_SRC_DIRECTORIES=factories message_converter common_parsers
SERVER_SRC_DIRECTORIES=client_data_on_server connection connection_manager mixer server string_converter server_parser
CLIENT_SRC_DIRECTORIES=client client_parser

CLIENT=client_main
SERVER=server_main

COMMON_SOURCES:=$(shell find $(COMMON_SRC_DIRECTORIES) -name '*.$(SRC_EXTENSION)')
SERVER_SOURCES:=$(shell find $(SERVER_SRC_DIRECTORIES) -name '*.$(SRC_EXTENSION)') src/server_main.cc
CLIENT_SOURCES:=$(shell find $(CLIENT_SRC_DIRECTORIES) -name '*.$(SRC_EXTENSION)') src/client_main.cc

CC=g++
CC_FLAGS=-std=c++11 -Wall --pedantic
BOOST_LIBS=-lboost_system -lboost_program_options

BIN_DIRECTORY=bin
BUILD_BIN_DIRECTORY:=$(shell mkdir -p $(BIN_DIRECTORY))

.PHONY: all

all: $(SERVER) $(CLIENT)

$(SERVER):
	$(CC) $(CC_FLAGS) $(COMMON_SOURCES) $(SERVER_SOURCES) -o $(BIN_DIRECTORY)/$(SERVER) $(BOOST_LIBS)

$(CLIENT):
	$(CC) $(CC_FLAGS) $(COMMON_SOURCES) $(CLIENT_SOURCES) -o $(BIN_DIRECTORY)/$(CLIENT) $(BOOST_LIBS)

clean:
	rm -Rf $(SERVER) $(CLIENT)
	find . -name "*.o" -type f -delete

cleanest:
	rm -Rf $(BIN_DIRECTORY)
