CC = gcc
TARGET = pam_bacchus.so
OBJECTS = src/pam_bacchus.o src/utils.o
CFLAGS = -Wall -O2 -fPIC
LDFLAGS = -shared -Xlinker -x -lcurl

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
