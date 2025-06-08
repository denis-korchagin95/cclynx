CC=gcc
SRC=./
BIN=./bin/
OBJ=./obj/
PROGRAM=cminx
CFLAGS=-std=c11 -g -Wall -O0
HEADERS=./headers/

all: build

OBJECTS+=allocator.o
OBJECTS+=identifier.o
OBJECTS+=main.o

build: $(addprefix $(OBJ), $(OBJECTS))
	$(CC) $(LFLAGS) $^ -o $(BIN)$(PROGRAM)

clean:
	rm -rfv $(BIN)$(PROGRAM)
	rm -rfv $(OBJ)*.o

$(OBJ)%.o: %.c
	$(CC) $(CFLAGS) -I$(HEADERS) -c $(SRC)$*.c -o $@