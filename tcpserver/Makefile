# default compiler
CC=gcc

CFLAGS=-pthread -lrt
TARGET=tcpserver.elf

all:$(TARGET)

tcpserver.elf: tcpserver.c ../utils/parser.c
	$(CC) -o $@ $^ $(CFLAGS)

transfer:
	scp $(TARGET) root@$(IP):$(DIR)

.PHONY: clean all transfer
clean:
	rm -rf *.elf
