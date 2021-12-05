#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <pthread.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>	// isdigit
#include <unistd.h> // gethostname

#include <signal.h>
#include <sys/time.h>

#include <syslog.h>
#include "../utils/parser.h"
/********************************************************************
 * Private defines
 *******************************************************************/
// define max number of clients
#define MAX_CLIENT_NUM		4

// message queue name size
#define MSG_QUEUE_NAME_SZ	4
// max length of a message queue
#define MAX_MSG_LEN_R     	10000

// check client status time (in seconds)
#define CHK_CLI_STAT_TIME	5

// define client states
typedef enum {
	CLIENT_DEAD 	= 0,
	CLIENT_ALIVE,
	CLIENT_AFK
} client_state_e;

#define CLI_STATE_STR_SZ	8
char client_state_str[3][CLI_STATE_STR_SZ] =
{
	[CLIENT_DEAD]	="DEAD",
	[CLIENT_ALIVE]	="ALIVE",
	[CLIENT_AFK]	="AFK"
};

/********************************************************************
 * Global variables
 *******************************************************************/
typedef struct client_socket_info client_socket_info_t;
struct client_socket_info
{
	int sockfd;
	int state;
	int index;

	// mqd_t msgq_id;
	// char msgq_name[MSG_QUEUE_NAME_SZ];
	char client_name[32];
};

// clients socket table
client_socket_info_t socket_table[MAX_CLIENT_NUM]={0};
// number of clients connected to the server
int num_clients = 0;

/********************************************************************
 * Error signaling
 *******************************************************************/
void panic(char *msg);
#define panic(m)	{perror(m); abort();}

/********************************************************************
 * Functions prototypes
 *******************************************************************/
// threads
void *thread_send(void *arg);
void *thread_recv(void *arg);

void broadcast(char *msg, int len);

void close_connection(int client_num);
static void sig_handler(int signum);
void my_alarm(int seconds);

// server commands
int recv_name_cb(int argc, char *argv[]);
int recv_cli_state_cb(int argc, char *argv[]);
int send_msg_cb(int argc, char *argv[]);
int exit_cb(int argc, char *argv[]);

const Command_t cmd_list[] = 
{
 	{"exit", exit_cb},
	{"State",recv_cli_state_cb},
	{"Name",recv_name_cb},
	{"MSG", send_msg_cb},
	{0,0}
};

/********************************************************************
 *  Creates a TCP server, supporting MAX_CLIENT_NUM clients
 *  Creates a message queue for each client, to exchange information
 * regarding the client status (ONLINE, AFK)
 *******************************************************************/
int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	int listen_sd, port;

	if(argc != 2)
	{
		printf("Usage: %s <protocol or portnum>\n", argv[0]);
		exit(0);
	}
	
	// Get server's IP and standard service connection
	if ( !isdigit(argv[1][0]) )
	{
		// if arg[1] was not a digit
		struct servent *srv = getservbyname(argv[1], "tcp");
		if(srv == NULL)
			panic(argv[1]);

		printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		port = srv->s_port;
	}
	else
	{
		// Translate an unsigned short integer into network byte order
		port = htons(atoi(argv[1]));
	}

	// create tcp socket using IPv4 and connection-based byte stream
	listen_sd = socket(PF_INET, SOCK_STREAM, 0);
	if ( listen_sd < 0 )
		// socket file descriptor not created
		panic("socket");

	// bind port/addr to socket 
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	// any interface
	addr.sin_addr.s_addr = INADDR_ANY;
	
	if ( bind(listen_sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
		panic("bind");

	// make listener with MAX_CLIENT_NUM slots
	if ( listen(listen_sd, MAX_CLIENT_NUM) != 0 )
		panic("listen")

	// get server host name
	char host_name[256];
	gethostname(host_name, sizeof(host_name));
	printf("Starting server on %s:%d\n", host_name, atoi(argv[1]));
	printf("Listening for incoming connections on sockfd:%d\n", listen_sd);

	// handle ctrl^c
	signal(SIGINT, sig_handler);
	
	// periodic alarm to check client status every CHK_CLI_STAT_TIME seconds
	// my_alarm(CHK_CLI_STAT_TIME);

	// create thread to listen cmd terminal input
	pthread_t child_send;
	pthread_create(&child_send, 0, thread_send, NULL);
	pthread_detach(child_send);

	// wait for incoming connections
	while(1)
	{
		int n = sizeof(addr);
		// accept connections
		int sd = accept(listen_sd, (struct sockaddr*)&addr, &n);

		if((sd != -1) && (num_clients < MAX_CLIENT_NUM))
		{
			pthread_t child_recv;

			socket_table[num_clients].sockfd = sd;
			socket_table[num_clients].state = CLIENT_ALIVE;
			socket_table[num_clients].index = num_clients;

			// send ID to new client(index in socket table)
			char msg[16];
			// send ID in 'ID' field
			snprintf(msg, sizeof(msg), "ID;%d", num_clients);
			send(socket_table[num_clients].sockfd, msg, strlen(msg)+1, 0);

			printf("Client(%d) connected (%d slots available)\n", sd, MAX_CLIENT_NUM - (num_clients+1));
			// create thread to receive this socket's information
			pthread_create(&child_recv, 0, thread_recv, &socket_table[num_clients]);
			num_clients++;

			// dont track this
			pthread_detach(child_recv);
		} else
			printf("Cannot accept new connection\n");
	}

	return 0;
}

/********************************************************************
 * Signal Handler
 *******************************************************************/
static void sig_handler(int signum)
{
	if(signum == SIGALRM)
	{
		// send a periodic message for every alive client
		char msg[8] = "State;";
		// send a msg to trigger the answer from the client
		broadcast(msg, strlen(msg));
	}
	else if(signum == SIGINT)
	{
		// close all existent connections
		printf("Server closing...\n");
		syslog(LOG_INFO, "Server closing...\n");

		for(int i = 0; i < MAX_CLIENT_NUM; i++)
		{
			if(socket_table[i].state == CLIENT_DEAD)
				// client is already dead
				continue;

			// notify client to close his connection
			send(socket_table[i].sockfd, "exit", 5, 0);
			close_connection(i);

			printf("Client(%d) '%s' closed connection (%d slots available)\n",
				socket_table[i].sockfd,
				socket_table[i].client_name,
				MAX_CLIENT_NUM - num_clients);
		}

		printf("Server closed\n");
		syslog(LOG_INFO, "Server closed\n");
		exit(0);
	}
}

/********************************************************************
 * My alarm
 * 
 * sets a periodic alarm for every xx seconds
 *******************************************************************/
void my_alarm(int seconds)
{
	struct itimerval itv;

	// set handling of SIGALRM
	signal(SIGALRM,sig_handler);

	// period between successive timer interrupts
	itv.it_interval.tv_sec = seconds;
	itv.it_interval.tv_usec = 0;

	// period between now and the first timer interrupt
	itv.it_value.tv_sec = seconds;
	itv.it_value.tv_usec = 0;
	setitimer (ITIMER_REAL, &itv, NULL);
}

/********************************************************************
 * Closes tcp connection on the given client index in socket_table
 *******************************************************************/
void close_connection(int index)
{
	// return if the client is already dead
	if(socket_table[index].state == CLIENT_DEAD)
		return;

	// close the client's channel
	shutdown(socket_table[index].sockfd,SHUT_RD);
	shutdown(socket_table[index].sockfd,SHUT_WR);
	shutdown(socket_table[index].sockfd,SHUT_RDWR);	

	socket_table[index].state = CLIENT_DEAD;
	num_clients--;
}

/********************************************************************
 * Thread to receive data, sent by the client
 *******************************************************************/
void *thread_recv(void *arg)                    
{	
	char buffer[128];
	// get and convert the socket
	client_socket_info_t *info = (client_socket_info_t *)arg;
	int err;

	while(info->state != CLIENT_DEAD)
	{
		// waits for a message to arrive
		if(recv(info->sockfd, buffer, sizeof(buffer), 0))
		{
			err = parse_cmd(cmd_list, buffer);
			if(err == (-ECMDNF))
				// cmd not recognized
				printf("'%s' No command\n", buffer);		
		}
	}

	close_connection(info->index);
	// terminate the thread
	return 0;
}

/********************************************************************
 * Thread to send data to a client
 *******************************************************************/
void *thread_send(void *arg)                    
{	
	char buffer[32];

	while(1)
	{
		scanf("%s", buffer);

		// user wants to close server?
		if(strcmp(buffer, "exit") == 0)
			raise(SIGINT);
	
		// else, broadcast received message
		char str[64];
		snprintf(str, sizeof(str), "Broad;%s", buffer);
		broadcast(str, strlen(str));
	}

	// terminate the thread
	return 0;
}

/********************************************************************
 * Broadcast
 *******************************************************************/
void broadcast(char *msg, int len)
{
	// broadcast message to all clients
	for(int i = 0; i < MAX_CLIENT_NUM; i++)
	{
		if(socket_table[i].state != CLIENT_DEAD)
			send(socket_table[i].sockfd, msg, len+1, 0);
	}
}

/********************************************************************
 * Server commands
 *******************************************************************/
int recv_name_cb(int argc, char *argv[])
{
	if(argc < 3)
		return -1;

	// client id is the index in socket table for his socket
	int index = atoi(argv[1]);

	// define client name
	strcpy(socket_table[index].client_name, argv[2]);
	
	printf("Client(%d) known by the name '%s'\n",
			socket_table[index].sockfd,
			socket_table[index].client_name);

	return 0;
}

int recv_cli_state_cb(int argc, char *argv[])
{
	if(argc < 3)
		return -1;

	// client id is the index in socket table for his socket
	int index = atoi(argv[1]);

	// get client state
	socket_table[index].state = atoi(argv[2]);

	printf("%s(%d) is '%s'\n",  socket_table[index].client_name,
								socket_table[index].sockfd,
								client_state_str[socket_table[index].state]);
}

int send_msg_cb(int argc, char *argv[])
{
	if(argc < 3)
		return -1;

	// client id is the index in socket table for his socket
	int index = atoi(argv[1]);

	// print received message
	printf("%s(%d): %s\n", 	socket_table[index].client_name,
							socket_table[index].sockfd,
							argv[2]);

	// broadcast received message
	char str[256];
	snprintf(str, sizeof(str), "MSG;%d;%s;%s",  socket_table[index].sockfd,
												socket_table[index].client_name,
												argv[2]);
	broadcast(str, strlen(str));
	return 0;
}

int exit_cb(int argc, char *argv[])
{
	if(argc < 2)
		return -1;

	// client id is the index in socket table for his socket
	int index = atoi(argv[1]);
	close_connection(index);

	printf("Client(%d) '%s' closed connection (%d slots available)\n",
				socket_table[index].sockfd,
				socket_table[index].client_name,
				MAX_CLIENT_NUM - num_clients);
}
