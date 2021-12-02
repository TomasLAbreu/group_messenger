# group_messenger

# Start TCP server
```shell
$ ./tcpserver <port>
```
# Start TCP client
```shell
$ ./tcpclient_recv <servername> <port>
```
# Client - Send message / see received messages
```shell
$ ./tcpclient_send [msg msg1 ... msgN]
```
# Close connection
## On client
Type 'exit', or use CTRL^C 
```shell
$ ./tcpclient_send exit
```
## On server
Type 'exit', or use CTRL^C

# After establishing connection
# client can send a message to the server
# server receives all messages and broadcasts them to the other clients

# server asks periodically the client states through message queues
	- every 5 seconds asks "State?"
	- every client replies with is state (ONLINE, AFK)
	- each client checks his own state every 60sec
		> ONLINE: if he sent a message in the last 60sec
		> AFK: if he didnt't send a message for over 60 sec