CC = gcc
TARGET = pam_bacchus.so
OBJECTS = src/pam_bacchus.o
CFLAGS = -Wall -O2
LDFLAGS = -shared -Xlinker -x -lcurl -fPIC

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

$(TARGET): src/pam_bacchus.o
	$(CC) -o $@ $^ $(LDFLAGS)
