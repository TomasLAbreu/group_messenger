# ---------------  Start TCP server ------------ 
$ ./server <port>

# ---------------- Start TCP client ------------
$ ./client <servername> <port> [msg1 msg2 ... msgN]

# -------------- Close connections -------------
$ exit
or
$ ^C

# ---------- After establishing connection ----
# client can send a message to the server
# server receives all messages and broadcasts them to the other clients

# server asks periodically the client states through message queues
	- every 5 seconds asks "State?"
	- every client replies with is state (ONLINE, AFK)
	- each client checks his own state every 60sec
		> ONLINE: if he sent a message in the last 60sec
		> AFK: if he didnt't send a message for over 60 sec
