CC     = gcc
CFLAGS = -Wall -Wextra -g -D_FILE_OFFSET_BITS=64
TARGETS = ext2info ext2cat ext2ls

all: $(TARGETS)

ext2info: ext2info.c ext2.h
	$(CC) $(CFLAGS) -o $@ $<

ext2cat: ext2cat.c ext2.h
	$(CC) $(CFLAGS) -o $@ $<

ext2ls: ext2ls.c ext2.h
	$(CC) $(CFLAGS) -o $@ $<

test: all
	bash test.sh

valgrind: all
	@test -f ext2.img || { echo "run 'make test' first to create ext2.img"; exit 1; }
	valgrind --leak-check=full --error-exitcode=1 ./ext2info ext2.img 2
	valgrind --leak-check=full --error-exitcode=1 ./ext2cat  ext2.img 2 >/dev/null
	valgrind --leak-check=full --error-exitcode=1 sh -c './ext2cat ext2.img 2 | ./ext2ls' >/dev/null

.PHONY: all test valgrind
