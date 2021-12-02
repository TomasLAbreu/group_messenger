# group_messenger
This application implements a "Broadcasting Chat service".
- Each Client (tcpclient_recv) is connected to the Server via TCP/IP.
- The Server (tcpserver) can host more than 3 clients. 
- Each Client connected to the Server is be able to send to the Server a character string passed by argument via command line arguments. (tcpclient_send)
- The Server forwards the received messages to all connected Clients.
- The Server identifies the client that has sent the message. (Ex. "Bento said: Hi Everyone!")
- Every 5 seconds the Server checks if each client is still ONLINE or AFK. (This message exchange does not show up on the chat terminal, is an internal feature).
- The Server has special commands to check the client status, and others.
- The Client can switch his status from ONLINE to AFK if has not sent any new message for more than a minute.
- The Client Message Receiver Service runs in the background. 
    
## Start TCP server
Starts a TCP server on port <port>.
```shell
$ ./tcpserver <port>
```
## Start TCP client
Starts a TCP client connected to <servername> on port <port>
```shell
$ ./tcpclient_recv <servername> <port>
```
## Client - Send message / see received messages
Send message to server or to see messages that have been send to the client since last time.
```shell
$ ./tcpclient_send [msg msg1 ... msgN]
```
## Close connection
### On client
Type 'exit', or use CTRL^C 
```shell
$ ./tcpclient_send exit
```
### On server
Type 'exit', or use CTRL^C

## After establishing connection
client can send a message to the server
server receives all messages and broadcasts them to the other clients
server asks periodically the client states through message queues
	- every 5 seconds asks "State?"
	- every client replies with is state (ONLINE, AFK)
	- each client checks his own state every 60sec
		> ONLINE: if he sent a message in the last 60sec
		> AFK: if he didnt't send a message for over 60 sec