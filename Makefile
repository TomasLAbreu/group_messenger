CC=gcc
# CC=arm-buildroot-linux-gnueabihf-gcc

CFLAGS=-pthread -lrt
#DEPS=parser.h
OBJS=tcpclient_send.elf tcpclient_recv.elf tcpserver.elf
# TARGET=
IP=10.42.0.254

all:$(OBJS) 

tcpclient_recv.elf: tcpclient_recv.c parser.c
	$(CC) -o tcpclient_recv.elf tcpclient_recv.c parser.c $(CFLAGS)

tcpserver.elf: tcpserver.c parser.c
	$(CC) -o tcpserver.elf tcpserver.c parser.c $(CFLAGS)

tcpclient_send.elf: tcpclient_send.c
	$(CC) -o tcpclient_send.elf tcpclient_send.c $(CFLAGS)

# $(OBJS): %.elf: %.c
# 	$(CC) -o  $@ $< $(CFLAGS)

transfer: 
	scp tcpclient_recv.elf tcpserver.elf tcpclient_send.elf root@$(IP):/etc/code/tcp/messengerv2	

.PHONY: clean
clean:
	rm -rf *.elf *.o