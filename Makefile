OBJECTS = pam-bacchus.so
CC = gcc
CFLAGS = -Wall

all: $(OBJECTS)

clean:
	rm -f $(OBJECTS) *.o

pam-bacchus.so: src/pam-bacchus.c
	$(CC) -fPIC -shared -Xlinker -x -o $@ $^ -lcurl
