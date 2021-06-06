all: id_test check_signature

.PHONY: clean
clean:
	rm -f id_test check_signature
	rm -f id_test.o check_signature.o

id_test: id_test.o
	$(CC) -o $@ $^ -lpam -lpam_misc

check_signature: check_signature.o ../src/tweetnacl.o ../src/utils.o
	$(CC) -o $@ $^
