# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Wextra -Wno-int-conversion -Wno-switch-outside-range -Wno-uninitialized -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch

# Libraries
LIBS = -lncurses -lzmq -lm 

# Source files
MACHINE_SRC = Roaches-client.c lizardNroach.c

# Dependency files
MACHINE_DPD = Roaches-client.c lizardNroach.c lizardNroach.h

# Output executables
MACHINE_OUT = ../bin/Roaches-client

# Executables
all: $(MACHINE_OUT)

$(MACHINE_OUT): $(MACHINE_DPD)
	$(CC) $(CFLAGS) -o $(MACHINE_OUT) $(MACHINE_SRC) $(LIBS)

clean:
	rm -f $(MACHINE_OUT)

.PHONY: all clean
