CFLAGS = -Wall -O2 -fPIC

all: pam_bacchus.so pam_bacchus_genkey

.PHONY: clean
clean:
	$(MAKE) -C test clean
	rm -f src/*.o pam_bacchus.so pam_bacchus_genkey

.PHONY: testdir
testdir:
	$(MAKE) -C test

test: pam_bacchus.so pam_bacchus_genkey testdir
	./run_test.sh

pam_bacchus.so: src/pam_bacchus.o src/utils.o src/tweetnacl.o src/randombytes.o
	$(CC) -o $@ $^ -shared -Xlinker -x -lcurl

pam_bacchus_genkey: src/pam_bacchus_genkey.o src/tweetnacl.o src/randombytes.o
	$(CC) -o $@ $^
