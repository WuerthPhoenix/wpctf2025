CC = gcc
CFLAGS=-Wall -no-pie -fstack-protector-all -O0 -Wl,-z,now -Wl,-z,relro
LDFLAGS=
SRC_DIR = src
OBJ_DIR = obj
EXEC = pwnlemetry

# find all the .o files in the src directory
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Default target
all: $(EXEC)

# Actually build the executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Create the object directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Build files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@


# clean
clean:
	rm -f $(OBJ_DIR)/*.o $(EXEC)

.PHONY: all clean
