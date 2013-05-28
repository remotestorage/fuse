CFLAGS=`getconf LFS_CFLAGS` -ggdb -std=gnu99
LDFLAGS=`getconf LFS_LDFLAGS` -lfuse `curl-config --libs`
HEADERS=src/remotestorage-fuse.h
OBJECTS=src/main.o src/operations.o src/trie.o src/node_store.o src/helpers.o src/sync.o src/remote.o

rs-mount: $(OBJECTS) $(HEADERS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJECTS) rs-mount

btree: src/bplus.o $(HEADERS)
	$(CC) src/bplus.o -o $@ $(LDFLAGS)

trie: src/trie.c $(HEADERS)
	$(CC) src/trie.c -o $@ $(CFLAGS) $(LDFLAGS) -DDEBUG_TRIE

helpers: src/helpers.c $(HEADERS)
	$(CC) src/helpers.c -o $@ $(CFLAGS) $(LDFLAGS) -DDEBUG_HELPERS
