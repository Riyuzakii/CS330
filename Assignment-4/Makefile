all:objfs
CC = gcc
CFLAGS  = -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -D_GNU_SOURCE -DDBG -g -Wall 
LDFLAGS = -pthread -lfuse
OBJS = objfs.o lib.o objstore.o

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
objfs: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

.Phony: clean
clean:
	rm -f *.o; rm -f objfs;
