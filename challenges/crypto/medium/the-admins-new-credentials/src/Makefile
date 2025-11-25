CC=clang
CFLAGS=-std=c99 -DLOG_USE_COLOR -g -fno-omit-frame-pointer
CWARNINGS=-Wall -Wextra -Waddress -Wstrict-overflow -Wstrict-aliasing -pedantic -Wpointer-arith
CSANITIZE=-fsanitize=address -fsanitize=undefined -fsanitize=leak
LIBS=-lssl -lcrypto -lllhttp
TARGET=server
SOURCE=main.c http.c log.c
HEADERS=http.h log.h

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(CWARNINGS) $(CSANITIZE) -o $(TARGET) $(SOURCE) $(LIBS)

format:
	clang-format -i $(SOURCE) $(HEADERS)

clean:
	rm -f $(TARGET) flag.h

.PHONY: all clean format
