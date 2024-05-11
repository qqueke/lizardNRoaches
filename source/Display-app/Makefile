# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Wextra -Wno-int-conversion -Wno-switch-outside-range -Wno-uninitialized -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch

# Libraries
LIBS = -lncurses -lzmq -lm -lprotobuf-c

# Source files
DISPLAY_SRC = Display-app.c ../src/lizardNroach.c 

# Dependency files
DISPLAY_DPD = Display-app.c  ../src/lizardNroach.c ../src/lizardNroach.h 

# Output executables
DISPLAY_OUT = ../bin/Display-app

# Executables
all: $(DISPLAY_OUT)

$(DISPLAY_OUT): $(DISPLAY_DPD)
	$(CC) $(CFLAGS) -o $(DISPLAY_OUT) $(DISPLAY_SRC) $(LIBS)

clean:
	@rm -v -f $(DISPLAY_OUT)

.PHONY: all clean