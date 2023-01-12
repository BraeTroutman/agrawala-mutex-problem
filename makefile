cc = gcc
all: brae-node print-server hacker
.PHONY: clean

brae-node: src/brae-node.c
	$(cc) -o target/brae-node src/brae-node.c

print-server: src/print-server.c
	$(cc) -o target/print-server src/print-server.c

hacker: src/hacker.c
	$(cc) -o target/hacker src/hacker.c

clean:
	rm target/*
