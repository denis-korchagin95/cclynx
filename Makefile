CC=gcc
SRC=./
BIN=./bin/
OBJ=./obj/
PROGRAM=cclynx
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
OBJECTS_TOKENIZER_TESTER+=type.o
OBJECTS_TOKENIZER_TESTER+=scope.o
OBJECTS_TOKENIZER_TESTER+=util.o

OBJECTS_PARSER_TESTER+=$(TESTERS)parser-tester.o
OBJECTS_PARSER_TESTER+=parser.o
OBJECTS_PARSER_TESTER+=allocator.o
OBJECTS_PARSER_TESTER+=tokenizer.o
OBJECTS_PARSER_TESTER+=identifier.o
OBJECTS_PARSER_TESTER+=symbol.o
OBJECTS_PARSER_TESTER+=print.o
OBJECTS_PARSER_TESTER+=type.o
OBJECTS_PARSER_TESTER+=scope.o
OBJECTS_PARSER_TESTER+=util.o

OBJECTS_IR_GENERATOR_TESTER+=$(TESTERS)ir-generator-tester.o
OBJECTS_IR_GENERATOR_TESTER+=parser.o
OBJECTS_IR_GENERATOR_TESTER+=tokenizer.o
OBJECTS_IR_GENERATOR_TESTER+=symbol.o
OBJECTS_IR_GENERATOR_TESTER+=type.o
OBJECTS_IR_GENERATOR_TESTER+=allocator.o
OBJECTS_IR_GENERATOR_TESTER+=scope.o
OBJECTS_IR_GENERATOR_TESTER+=identifier.o
OBJECTS_IR_GENERATOR_TESTER+=print.o
OBJECTS_IR_GENERATOR_TESTER+=ir.o
OBJECTS_IR_GENERATOR_TESTER+=util.o

OBJECTS_TARGET_CODE_GENERATOR_TESTER+=$(TESTERS)target-code-generator-tester.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=parser.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=tokenizer.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=symbol.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=type.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=allocator.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=scope.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=identifier.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=print.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=ir.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=target-arm64.o
OBJECTS_TARGET_CODE_GENERATOR_TESTER+=util.o

OBJECTS+=allocator.o
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

tokenizer-tester: $(addprefix $(OBJ), $(OBJECTS_TOKENIZER_TESTER))
	$(CC) $(LFLAGS) $^ -o $(BIN)tokenizer-tester

parser-tester: $(addprefix $(OBJ), $(OBJECTS_PARSER_TESTER))
	$(CC) $(LFLAGS) $^ -o $(BIN)parser-tester

ir-generator-tester: $(addprefix $(OBJ), $(OBJECTS_IR_GENERATOR_TESTER))
	$(CC) $(LFLAGS) $^ -o $(BIN)ir-generator-tester

target-code-generator-tester: $(addprefix $(OBJ), $(OBJECTS_TARGET_CODE_GENERATOR_TESTER))
	$(CC) $(LFLAGS) $^ -o $(BIN)target-code-generator-tester

build: $(addprefix $(OBJ), $(OBJECTS))
	$(CC) $(LFLAGS) $^ -o $(BIN)$(PROGRAM)

build-testers: tokenizer-tester parser-tester ir-generator-tester target-code-generator-tester

clean:
	rm -rfv $(BIN)$(PROGRAM)
	rm -rfv $(BIN)*-tester
	rm -rfv $(OBJ)$(TESTERS)*.o
	rm -rfv $(OBJ)*.o

$(OBJ)%.o: %.c
	$(CC) $(CFLAGS) -I$(HEADERS) -c $(SRC)$*.c -o $@
