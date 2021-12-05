# group_messenger
This application implements a "Broadcasting Chat service", making use of three services:
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

## How it works
The tcpclient_recv establishes a TCP connection with the server, and creates two message queues to commmunicate with the tcpclient_send, that allows printing of messages and input reading from user. One message queue is used to receive the messages from the user, the other one is used to send the received messages from the server to the client (on tcpclient_send).

## Dependencies
...gcc, scp (optional), rm?

## Installation
Use the existent makefile.
```make``` or ```make all``` to create all needed apps.
```make clean``` to delete all generated files.
```make transfer``` to copy all generated files to the Raspberry Pi, to a predefined path, using a predefined IP.

- $(CC) define your compiler.
- $(IP) define your Raspberry Pi IP
- $(PATH) define your Raspberry Pi directory to paste the files

## Usage
### Start TCP server
Starts a TCP server on a given port.
```Shell
$ ./tcpserver <port>
```
### Start TCP client
Starts a TCP client connected to a given server name on a given port.
```Shell
$ ./tcpclient_recv <servername> <port>
```
When this is running, a led (led0 - green led in Raspberry Pi) is light up. This is done via a device driver, developed in previous classes.

### Client - Send message / see received messages
Send message to server or to see messages that have been send to the client since last time. Everytime the user wants to send a message he must use tcpclient_send with the wanted message to be sent. If a message is not supplied, the tcpclient_send will just print the messages that weren't yet read.
```shell
$ ./tcpclient_send [msg msg1 ... msgN]
```
### Close connection
### On client
Type 'exit', or use CTRL^C 
```shell
$ ./tcpclient_send exit
```
When this happens, the (Daemon) ./tcpclient_recv terminates, and the led that was previously light up (led0) is turned off.

### On server
Type 'exit', or use CTRL^C

## Done by
José Tomás Abreu, a88218

Masters in Embedded Systems @ Universidade do Minho, 2021