OBJECTS = pam_bacchus.so
CC = gcc
CFLAGS = -Wall

all: $(OBJECTS)

clean:
	rm -f $(OBJECTS) *.o

pam_bacchus.so: src/pam_bacchus.c
	$(CC) -fPIC -shared -Xlinker -x -o $@ $^ -lcurl
