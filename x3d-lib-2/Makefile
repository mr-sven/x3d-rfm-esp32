# Makefile for Writing Make Files Example

# *****************************************************
# Variables to control Makefile operation

NAME = main
SRC = src
OBJ = obj
BIN = bin
TEST = test

CC = gcc
CFLAGS = -Wall -g -DUNIT_TEST
LDFLAGS =
LDLIBS =

.PHONY: test clean

LIB_SRCS = $(wildcard $(SRC)/*.c)
LIB_OBJS = $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(LIB_SRCS))

TEST_SRCS = $(wildcard $(TEST)/*.c)
TEST_BINS = $(patsubst $(TEST)/%.c,$(BIN)/$(TEST)/%,$(TEST_SRCS))

# ****************************************************
# Targets needed to bring the executable up to date

$(BIN)/%: $(OBJ)/%.o $(LIB_OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -c -MMD -o $@

test: $(TEST_BINS)
	@bash $(TEST)/run_tests.sh $^

$(BIN)/$(TEST)/%: $(TEST)/%.c $(LIB_OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LIB_OBJS) -MMD -o $@

clean:
	@rm -rf $(BIN) $(OBJ)
