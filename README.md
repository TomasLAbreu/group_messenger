# group_messenger
## Overview
This application implements a "Broadcasting Chat service", making use of three services:

**Client Message Receiver Service**: *tcpclient_recv*

**Client Message Sender Service**: *tcpclient_send*

**Server App**: *tcpserver*

- Each Client is connected to the Server via TCP/IP.
- Each Client connected to the Server is be able to send to the Server a character string passed by argument via command line arguments.
- The Server forwards the received messages to all connected Clients, and identifies the client that has sent the message.
- Every 5 seconds the Server checks if each client is still ONLINE or AFK. (internal feature). The Client can switch his status from ONLINE to AFK if has not sent any new message for more than a minute.
- The Server has special commands to check the client status, and others.
- The Client Message Receiver Service runs in the background. 

## How it works
This application makes use of POSIX Threads, Daemons, IPC (message queues), TCP/IP and Device Drivers.
-  ***tcpserver*** implements a TCP server.
- ***tcpclient_recv*** establishes a TCP connection with the server. This is used to exchange all messages between the user and the server. So the user doesn't interact directly with the server.
- ***tcpclient_recv*** creates two message queues to communicate with the user, ***tcpclient_send***
- ***tcpclient_send***  allows printing of messages and input reading from user. One message queue is used to receive the messages from the user, the other one is used to send the received messages from the server to the client.

## Dependencies
- ***make***
- ***scp***
- ***rm***


## Installation
1. Clone this repository into your local machine.
```shell
$ git clone git@github.com:TomasLAbreu/group_messenger.git
```
2. Change working directory.
```shell
$ cd group_messenger/
```
3. Create all targets using ***make*** tool. This will compile and create all needed object files.
```shell
$ make all
```
This step uses the predefined compiler *gcc*. To define another compiler define it when using ***make*** tool, into **CC**. Example: compile all targets with *arm-linux-gcc*, the one used to compile files for Raspberry Pi.

```shell
$ make all CC=arm-linux-gcc
```
If you want to compile server files and client files separately you can do it by typing ```make server``` or ```make client```, respectively. This allows you to compile using different compilers. 

```shell
$ make client CC=arm-linux-gcc
$ make server CC=gcc
```
4. Delete all generated files (using ***rm***).
```shell
$ make clean
```
5. Copy all generated files to a remote host on a network, defined by a given **IP**, and with destination **DIR** (using ***scp***).

   *Example*: copy all generated files to a remote host that has an IP of 10.42.0.254, and into the folder /etc/group_messenger/.
```shell
$ make transfer IP=10.42.0.254 DIR=/etc/group_messenger/
```
Like in step 3, if you want to send just the server files, or the client files, separately you can do it by using ```make transfer_server``` or ```make transfer_client```, respectively. This allows you to send files to different hosts, or to different folders.
```shell
$ make transfer_server IP=10.42.0.254 DIR=/etc/group_messenger/
$ make transfer_client IP=10.42.0.90 DIR=/code/
```

## Usage
Check that you are in the right directory:
```shell
$ pwd
/.../group_messenger
```
1. Starts a **TCP server** on a given port.

```shell
$ ./tcpserver <port>
```
2. Starts a **TCP client** connected to a given server name on a given port. (If you are running this on the Raspberry Pi 4 model B, a led (*led0* - green led) is light up after executing this command. This is done via a device driver, developed in previous classes.)

```shell
$ ./tcpclient_recv <servername> <port>
```
3. Send message(s) to the server or to see messages that weren't yet read. If a message is not supplied, the this command will just print the messages that weren't yet read.

```shell
$ ./tcpclient_send [msg msg1 ... msgN]
```
### Close connection
1. **Close client** connection to server. (When this happens, the TCP client connection terminates, and the led that was previously light up (led0) is turned off.)

```shell
$ ./tcpclient_send exit
```
2. **Close server**, type 'exit' or use CTRL^C.
```shell
...
exit
$
```

## Done by
José Tomás Abreu, a88218

Masters in Embedded Systems @ Universidade do Minho, 2021