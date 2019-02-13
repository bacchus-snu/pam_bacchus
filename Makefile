CC = gcc
TARGET = pam_bacchus.so
OBJECTS = src/pam_bacchus.o src/utils.o
CFLAGS = -Wall -O2 -fPIC
LDFLAGS = -shared -Xlinker -x -lcurl

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f test/id_test test/id_test.o

test: test/id_test.o
	$(CC) -o test/id_test test/id_test.c -lpam -lpam_misc
	./run_test.sh

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
