CC=gcc
# CC=arm-buildroot-linux-gnueabihf-gcc

CFLAGS=-pthread -lrt
TARGET=tcpserver.elf

IP=10.42.0.254
DIR=/etc/code/tcp/messengerv2

all:$(TARGET)

tcpserver.elf: tcpserver.c ../utils/parser.c
	$(CC) -o $@ $^ $(CFLAGS)

transfer:
	scp $(TARGET) root@$(IP):$(DIR)

.PHONY: clean all transfer
clean:
	rm -rf *.elf
