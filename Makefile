CC=gcc
SRC=./
BIN=./bin/
OBJ=./obj/
PROGRAM=cminx
CFLAGS=-std=c11 -g -Wall -O0
HEADERS=./headers/
TESTERS=./testers/

all: build

OBJECTS_TOKENIZER_TESTER+=$(TESTERS)tokenizer-tester.o
OBJECTS_TOKENIZER_TESTER+=allocator.o
OBJECTS_TOKENIZER_TESTER+=print.o
OBJECTS_TOKENIZER_TESTER+=tokenizer.o
OBJECTS_TOKENIZER_TESTER+=identifier.o
OBJECTS_TOKENIZER_TESTER+=symbol.o

OBJECTS+=allocator.o
OBJECTS+=identifier.o
OBJECTS+=symbol.o
OBJECTS+=print.o
OBJECTS+=tokenizer.o
OBJECTS+=main.o

tokenizer-tester: $(addprefix $(OBJ), $(OBJECTS_TOKENIZER_TESTER))
	$(CC) $(LFLAGS) $^ -o $(BIN)tokenizer-tester

build: $(addprefix $(OBJ), $(OBJECTS))
	$(CC) $(LFLAGS) $^ -o $(BIN)$(PROGRAM)

clean:
	rm -rfv $(BIN)$(PROGRAM)
	rm -rfv $(BIN)tokenizer-tester
	rm -rfv $(OBJ)*.o

$(OBJ)%.o: %.c
	$(CC) $(CFLAGS) -I$(HEADERS) -c $(SRC)$*.c -o $@
