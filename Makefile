CC=gcc
SRC=./
BIN=./bin/
BIN_TESTERS=./bin/testers/
OBJ=./obj/
PROGRAM=cclynx
CFLAGS=-std=c11 -g2 -Wall -Wextra -pedantic -O0
HEADERS=./headers/
TESTERS=./testers/

vpath %.c $(SRC) $(TESTERS)

all: build

OBJECTS_HASHMAP_TESTER+=$(TESTERS)hashmap-tester.o
OBJECTS_HASHMAP_TESTER+=hashmap.o
OBJECTS_HASHMAP_TESTER+=cclynx.o
OBJECTS_HASHMAP_TESTER+=allocator.o
OBJECTS_HASHMAP_TESTER+=errors.o
OBJECTS_HASHMAP_TESTER+=util.o


OBJECTS+=cclynx.o
OBJECTS+=allocator.o
OBJECTS+=errors.o
OBJECTS+=hashmap.o
OBJECTS+=identifier.o
OBJECTS+=symbol.o
OBJECTS+=type.o
OBJECTS+=scope.o
OBJECTS+=print.o
OBJECTS+=tokenizer.o
OBJECTS+=parser.o
OBJECTS+=ir.o
OBJECTS+=target-arm64.o
OBJECTS+=util.o
OBJECTS+=main.o

hashmap-tester: $(addprefix $(OBJ), $(OBJECTS_HASHMAP_TESTER))
	@mkdir -p $(BIN_TESTERS)
	$(CC) $(LFLAGS) $^ -o $(BIN_TESTERS)hashmap-tester


build: $(addprefix $(OBJ), $(OBJECTS))
	$(CC) $(LFLAGS) $^ -o $(BIN)$(PROGRAM)

build-testers: hashmap-tester

test: clean build build-testers
	jcunit --no-cache tests/

test-examples: build
	./scripts/check-examples.sh

test-all: test test-examples

clean:
	rm -rfv $(BIN)$(PROGRAM)
	rm -rfv $(BIN_TESTERS)
	rm -rfv $(OBJ)$(TESTERS)*.o
	rm -rfv $(OBJ)*.o

$(OBJ)%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(HEADERS) -c $(SRC)$*.c -o $@
