CLIENT=./tcpclient
SERVER=./tcpserver

# ---------------
# make all
# ---------------
all: client server

client:
	$(MAKE) all -C $(CLIENT)
	@find $(CLIENT) -name "*.elf" | xargs cp -u -t .
	@echo ''

server:
	$(MAKE) all -C $(SERVER)
	@find $(SERVER) -name "*.elf" | xargs cp -u -t .
	@echo ''

# ---------------
# make transfer
# ---------------
transfer: transfer_client transfer_server

transfer_client:
	$(MAKE) transfer -C $(CLIENT)
	@echo ''

transfer_server:
	$(MAKE) transfer -C $(SERVER)
	@echo ''

# ---------------
# make clean
# ---------------
clean:
	@rm -f *.elf
	$(MAKE) $@ -C $(CLIENT)
	@echo ''
	$(MAKE) $@ -C $(SERVER)

.PHONY: all clean transfer
