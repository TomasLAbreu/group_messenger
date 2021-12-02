# group_messenger
This application implements a "Broadcasting Chat service".
```
Client Message Receiver Service: tcpclient_recv
Client Message Sender Service: tcpclient_send
Server App: tcpserver
```
- Each Client (tcpclient_recv) is connected to the Server via TCP/IP.
- Each Client connected to the Server is be able to send to the Server a character string passed by argument via command line arguments. (tcpclient_send)
- The Server forwards the received messages to all connected Clients, and identifies the client that has sent the message.
- Every 5 seconds the Server checks if each client is still ONLINE or AFK. (internal feature). The Client can switch his status from ONLINE to AFK if has not sent any new message for more than a minute.
- The Server has special commands to check the client status, and others.
- The Client Message Receiver Service runs in the background. 

# How it works
The tcpclient_recv establishes a TCP connection with the server, and creates two message queues to commmunicate with the tcpclient_send, that allows printing of messages and input reading from user. One message queue is used to receive the messages from the user, the other one is used to send the received messages from the server to the client (on tcpclient_send).

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
When this is running, a led (led0 - green led in Raspberry Pi) is light up. This is done via a device driver, developed in previous classes.

## Client - Send message / see received messages
Send message to server or to see messages that have been send to the client since last time. Everytime the user wants to send a message he must use tcpclient_send with the wanted message to be sent. If a message is not supplied, the tcpclient_send will just print the messages that weren't yet read.
```shell
$ ./tcpclient_send [msg msg1 ... msgN]
```
## Close connection
### On client
Type 'exit', or use CTRL^C 
```shell
$ ./tcpclient_send exit
```
When this happens, the (Daemon) ./tcpclient_recv terminates, and the led that was previously light up (led0) is turned off.

### On server
Type 'exit', or use CTRL^C