CC = gcc
TARGET = pam_bacchus.so
OBJECTS = src/pam_bacchus.o src/utils.o src/tweetnacl.o src/randombytes.o
CFLAGS = -Wall -O2 -fPIC
LDFLAGS = -shared -Xlinker -x -lcurl

all: $(TARGET)

.PHONY: clean
clean:
	$(MAKE) -C test clean
	rm -f $(OBJECTS) $(TARGET)
	rm -f pam_bacchus_genkey src/pam_bacchus_genkey.o

.PHONY: testdir
testdir:
	$(MAKE) -C test

test: $(TARGET) genkey testdir
	./run_test.sh

genkey: src/pam_bacchus_genkey.o src/tweetnacl.o src/randombytes.o
	$(CC) -o pam_bacchus_genkey $^

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
