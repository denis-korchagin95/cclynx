CC=gcc
SRC=./
BIN=./bin/
OBJ=./obj/
PROGRAM=cminx
CFLAGS=-std=c11 -g -Wall -O0

all: build

OBJECTS+=main.o

build: $(addprefix $(OBJ), $(OBJECTS))
	$(CC) $(LFLAGS) $^ -o $(BIN)$(PROGRAM)

clean:
	rm -rfv $(BIN)$(PROGRAM)
	rm -rfv $(OBJ)*.o

$(OBJ)%.o: %.c
	$(CC) $(CFLAGS) -c $(SRC)$*.c -o $@