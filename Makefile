CC=gcc
SRC=./
BIN=./bin/
OBJ=./obj/
PROGRAM=cminx
CFLAGS=-std=c11 -g2 -Wall -O0
HEADERS=./headers/
TESTERS=./testers/

vpath %.c $(SRC) $(TESTERS)

all: build

OBJECTS_TOKENIZER_TESTER+=$(TESTERS)tokenizer-tester.o
OBJECTS_TOKENIZER_TESTER+=tokenizer.o
OBJECTS_TOKENIZER_TESTER+=allocator.o
OBJECTS_TOKENIZER_TESTER+=print.o
OBJECTS_TOKENIZER_TESTER+=identifier.o
OBJECTS_TOKENIZER_TESTER+=symbol.o

OBJECTS_PARSER_TESTER+=$(TESTERS)parser-tester.o
OBJECTS_PARSER_TESTER+=parser.o
OBJECTS_PARSER_TESTER+=allocator.o
OBJECTS_PARSER_TESTER+=tokenizer.o
OBJECTS_PARSER_TESTER+=identifier.o
OBJECTS_PARSER_TESTER+=symbol.o
OBJECTS_PARSER_TESTER+=print.o

OBJECTS+=allocator.o
OBJECTS+=identifier.o
OBJECTS+=symbol.o
OBJECTS+=print.o
OBJECTS+=tokenizer.o
OBJECTS+=parser.o
OBJECTS+=main.o

tokenizer-tester: $(addprefix $(OBJ), $(OBJECTS_TOKENIZER_TESTER))
	$(CC) $(LFLAGS) $^ -o $(BIN)tokenizer-tester

parser-tester: $(addprefix $(OBJ), $(OBJECTS_PARSER_TESTER))
	$(CC) $(LFLAGS) $^ -o $(BIN)parser-tester

build: $(addprefix $(OBJ), $(OBJECTS))
	$(CC) $(LFLAGS) $^ -o $(BIN)$(PROGRAM)

clean:
	rm -rfv $(BIN)$(PROGRAM)
	rm -rfv $(BIN)tokenizer-tester
	rm -rfv $(OBJ)$(TESTERS)tokenizer-tester.o
	rm -rfv $(OBJ)*.o

$(OBJ)%.o: %.c
	$(CC) $(CFLAGS) -I$(HEADERS) -c $(SRC)$*.c -o $@