CC = g++
CFLAGS = -lrt -g

LIB_DIR = lib
TEST_DIR = tests

OBJ_FILES = $(LIB_DIR)/TCB.o $(LIB_DIR)/uthread.o

pi: $(TEST_DIR)/pi.cpp $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

$(LIB_DIR)/TCB.o: $(LIB_DIR)/TCB.cpp
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_DIR)/uthread.o: $(LIB_DIR)/uthread.cpp
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_DIR)/test.cpp $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f pi
	rm -f test
	rm -f *.o
	rm -f $(OBJ_FILES)
