TARGET=server

all: $(TARGET) certs templates/flag.html

flag.txt:
	echo "wpctf{REDACTED}" > flag.txt

templates/flag.html: flag.txt
	echo "<!doctype html>"
	echo "<html><body><h1>Flag</h1><p>" >> templates/flag.html
	echo "<p> Here is your flag: <strong>$(shell cat flag.txt)</strong></p>" >> templates/flag.html
	echo "</body></html>" >> templates/flag.html

$(TARGET): src/*
	make -C src
	cp src/$(TARGET) .

clean:
	rm -f $(TARGET) *.crt *.key templates/flag.html
	make clean -C src

# Generate self-signed server certificate and key
certs:
	openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365 -nodes -subj "/C=US/ST=CTF/L=Challenge/O=WPCTF/CN=localhost"

format:
	make format -C src

.PHONY: all clean certs format
