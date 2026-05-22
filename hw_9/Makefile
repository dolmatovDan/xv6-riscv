CC      = gcc
CFLAGS  = -Wall -Wextra -g -D_FILE_OFFSET_BITS=64
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

valgrind: all ext2.img
	valgrind --leak-check=full --error-exitcode=1 ./ext2info ext2.img 2
	valgrind --leak-check=full --error-exitcode=1 ./ext2cat  ext2.img 2 >/dev/null

clean:
	rm -f $(TARGETS) *.o

.PHONY: all test valgrind clean
