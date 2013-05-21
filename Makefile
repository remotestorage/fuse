CFLAGS=`getconf LFS_CFLAGS` -ggdb -std=gnu99
LDFLAGS=`getconf LFS_LDFLAGS` -lfuse `curl-config --libs`
OBJECTS=src/main.o src/operations.o src/node_store.o src/helpers.o

rs-mount: $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJECTS) rs-mount

btree: src/bplus.o
	$(CC) src/bplus.o -o $@ $(LDFLAGS)

trie: src/trie.o
	$(CC) src/trie.o -o $@ $(LDFLAGS)
