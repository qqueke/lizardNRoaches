# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Wextra

# Libraries
LIBS = -lncurses -lzmq -lm -lpthread -lprotobuf-c

# Source files
CLIENT_SRC = Lizard-client.c ../src/clients.c ../src/client_server.pb-c.c ../src/client_server.c

# Dependency files
CLIENT_DPD = Lizard-client.c ../src/clients.c ../src/clients.h  ../src/client_server.pb-c.h ../src/client_server.h

# Output executables
CLIENT_OUT = ../bin/Lizard-client

# Executables
all: $(CLIENT_OUT) 

$(CLIENT_OUT): $(CLIENT_DPD)
	$(CC) $(CFLAGS) -o $(CLIENT_OUT) $(CLIENT_SRC) $(LIBS)

clean:
	@rm -v -f $(CLIENT_OUT) 
