# default compiler
CC=gcc

CFLAGS=-pthread -lrt
TARGET=tcpclient_send.elf tcpclient_recv.elf

all:$(TARGET) 

tcpclient_recv.elf: tcpclient_recv.c ../utils/parser.c
	$(CC) -o $@ $^ $(CFLAGS)

tcpclient_send.elf: tcpclient_send.c
	$(CC) -o $@ $< $(CFLAGS)

transfer:
	scp $(TARGET) root@$(IP):$(DIR)

.PHONY: clean all transfer
clean:
	rm -rf *.elf
