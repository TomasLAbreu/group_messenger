CLIENT=./tcpclient
SERVER=./tcpserver

# ---------------
# make all
# ---------------
all: client server

client:
	$(MAKE) all -C $(CLIENT)
	@echo ''

server:
	$(MAKE) all -C $(SERVER)
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
	$(MAKE) $@ -C $(CLIENT)
	@echo ''
	$(MAKE) $@ -C $(SERVER)

.PHONY: all clean transfer
