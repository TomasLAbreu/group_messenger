#include <sys/stat.h> // umask
#include <stdio.h>	// perror
#include <stdlib.h> // pid_t, EXIT_FAILURE
#include <unistd.h> // fork, setsid, getsid,...

#include <ctype.h>	// isdigit
#include <pthread.h>
#include <mqueue.h>   /* mq_* functions */
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <netdb.h>
#include <syslog.h>

#include <signal.h>
#include <sys/time.h>

#include "parser.h"
#include <errno.h>
/********************************************************************
 * Private defines
 *******************************************************************/
// max length of a message queue
#define MAX_MSG_LEN_R     10000

/* name of the POSIX object referencing the queue */
#define MSGQSEND_NAME    "/send_queue"
#define MSGQRECV_NAME    "/recv_queue"

#define CLI_CHK_STAT_TIME	15

typedef enum {
	CLIENT_DEAD 	= 0,
	CLIENT_ALIVE,
	CLIENT_AFK
} client_state_e;

/********************************************************************
 * Global variables
 *******************************************************************/
mqd_t msgq_send_id;
mqd_t msgq_recv_id;

int sd;
int client_id;
char client_name[32];
// pid_t client_recv_pid;

// client state
volatile int client_state = CLIENT_DEAD;

/********************************************************************
 * Error signaling
 *******************************************************************/
void panic(char *msg);
// #define panic(m)	{perror(m); abort();}
#define panic(m)	{syslog(LOG_ERR, "ERROR: %s\n", (char*)m); abort();}

/********************************************************************
 * Function prototypes
 *******************************************************************/
void* thread_recv(void *arg);
void* thread_send(void *arg);

void my_alarm(int seconds);
static void sig_handler(int signum);
void close_connection(void);

// client commands
int state_cb(int argc, char *argv[]);
int recv_id_cb(int argc, char *argv[]);
int recv_msg_cb(int argc, char *argv[]);
int recv_broad_cb(int argc, char *argv[]);
int exit_cb(int argc, char *argv[]);

const Command_t cmd_list[] = 
{
 	{"exit", exit_cb},
 	{"Broad", recv_broad_cb},
 	{"MSG", recv_msg_cb},
	{"State",state_cb},
	{"ID",recv_id_cb},
	{0,0}
};

/********************************************************************
 * main
 * 
 * create a daemon
 * start a TCP/IP connection with a server
 * create a message queue to exchange messages between client-server
 * 
 * 
 * recv message from msgqueue
 * send queued message to server
 * 
 * recv message from server
 * insert message into msgqueue
 *******************************************************************/
int main(int argc, char *argv[])
{
	pid_t pid, sid;
	
	if(argc < 3)
	{
		printf("Usage: %s <servername> <protocol or portnum>\n", argv[0]);
		exit(0);
	}

	// create a new process (child)
	pid = fork();
	if (pid < 0)
	{
		// on error exit
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid > 0)
	{
		// client_recv_pid = pid;
		printf("Creating daemon (PID:%d)\n", pid);

		// exit parent process
		exit(EXIT_SUCCESS);
	}

	// create a new session
	sid = setsid();
	if (sid < 0)
	{
		// on error exit
		perror("setsid");
		exit(EXIT_FAILURE);
	}
	
	// make '/' the root directory
	if (chdir("/") < 0)
	{
		// on error exit
		perror("chdir");
		exit(EXIT_FAILURE);
	}
	
	// reset umask to 0
	// Any permission may be set (rwx)
	umask(0); 
	// close standard input file descriptor
	close(STDIN_FILENO); 
	// close standard output file descriptor
	close(STDOUT_FILENO);
	// close standard error file descriptor
	close(STDERR_FILENO);

	syslog(LOG_INFO, "Daemon created\n");
	/******************************************************
	* Service implementation (LED device driver)
	******************************************************/
	syslog(LOG_INFO, "Inserting Device Driver...\n");
    system("insmod /etc/class/ddrivers/led.ko");

    // syslog(LOG_INFO, "Check device driver:\n");
    // system("lsmod");

    // syslog(LOG_INFO, "Is the device driver in /dev:\n");
    // system("ls -l /dev/led0");

	int fd0 = open("/dev/led0", O_WRONLY);
    char ledOn = '1';
    char ledOff = '0';

    system("echo none > /sys/class/leds/led0/trigger");

    write(fd0, &ledOn, 1);
   	syslog(LOG_INFO, "LED on!\n");
	/******************************************************
	* Service implementation (TCP client)
	******************************************************/
	struct hostent* host;
	struct sockaddr_in addr;
	int port;

	// Get server's IP and standard service connection
	host = gethostbyname(argv[1]);

	if ( !isdigit(argv[2][0]) )
	{
		struct servent *srv = getservbyname(argv[2], "tcp");
		if ( srv == NULL )
			panic(argv[2]);

		// printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		port = srv->s_port;
	}
	else
		port = htons(atoi(argv[2]));

	// Create socket
	sd = socket(PF_INET, SOCK_STREAM, 0);
	if ( sd < 0 )
		panic("socket");
	
	/* create & zero struct */
	memset(&addr, 0, sizeof(addr));
	/* select internet protocol */
	addr.sin_family = AF_INET;
	/* set the port # */
	addr.sin_port = port;
	/* set the addr */
	addr.sin_addr.s_addr = *(long*)(host->h_addr_list[0]);

	syslog(LOG_INFO, "Connecting to server on %s:%d\n", host->h_name, atoi(argv[2]));
	// signal(SIGINT, sig_handler);
	
	// get host name
	gethostname(client_name, sizeof(client_name));

	// connect to server
	if (connect(sd, (struct sockaddr*)&addr, sizeof(addr)) == 0)
	{
		pthread_t child_recv;
		pthread_t child_send;

		syslog(LOG_INFO, "Connected to server");
		
		/* opening the queue using default attributes */
	    msgq_send_id = mq_open(MSGQSEND_NAME, O_RDWR | O_CREAT | O_NONBLOCK , S_IRWXU | S_IRWXG, NULL);
	    if (msgq_send_id == (mqd_t)-1)
	    	panic("In mq_open(send)");

		syslog(LOG_INFO, "Opened queue '%s' sucessfuly\n", MSGQSEND_NAME);

		/* opening the queue using default attributes */
	    msgq_recv_id = mq_open(MSGQRECV_NAME, O_RDWR | O_CREAT , S_IRWXU | S_IRWXG, NULL);
	    if (msgq_recv_id == (mqd_t)-1)
	    	panic("In mq_open(recv)");

	    syslog(LOG_INFO, "Opened queue '%s' sucessfuly\n", MSGQRECV_NAME);

	    // set alarm to check client status
		my_alarm(CLI_CHK_STAT_TIME);
		client_state = CLIENT_ALIVE;

		// create threads to handle receive and send
		pthread_create(&child_recv, 0, thread_recv, NULL);
		pthread_create(&child_send, 0, thread_send, NULL);
		
		pthread_join(child_send, NULL);
		pthread_join(child_recv, NULL);
	}
	else
		panic("connect");

	/******************************************************
	* Exiting
	******************************************************/
	syslog(LOG_INFO, "EXITING DAEMON\n");

	write(fd0, &ledOff, 1);
	sleep(2);
  	syslog(LOG_INFO, "LED off!\n");

	syslog(LOG_INFO, "Closing Device Driver.\n");
    close(fd0);
    syslog(LOG_INFO, "Removing Device Driver.\n");
    system("rmmod led");

	return 0;
}

void* thread_recv(void *arg)
{
	char buffer[256];
	int err;

	while(client_state != CLIENT_DEAD)
	{
		// recv message from server
		if(recv(sd,buffer,sizeof(buffer),0))
		{
			syslog(LOG_INFO, "RX: '%s' to mqueue\n", buffer);
			
			err = parse_cmd(cmd_list, buffer);
			// if(err != (-ECMDNF))
			// { }
		}
	}
	syslog(LOG_INFO, "Exiting th_recv\n");
	return 0;
}

void* thread_send(void *arg)
{
	char msg[MAX_MSG_LEN_R];

	while(client_state != CLIENT_DEAD)
	{
		// read message from msgqueue (NON BLOCKING MODE)
		if (mq_receive(msgq_send_id, msg, MAX_MSG_LEN_R, NULL) > 0)
		{
			// message read OK
			syslog(LOG_INFO, "TX: '%s' from mqueue\n", msg);
	        // client has sent a message
	        client_state = CLIENT_ALIVE;
	        // reset alarm
	        my_alarm(CLI_CHK_STAT_TIME);

	        if(strcmp(msg,"exit") != 0)
	       	{
				// send queued message to server
				char str[MAX_MSG_LEN_R+8];
				snprintf(str, sizeof(str), "MSG;%d;%s",client_id, msg);
				send(sd, str, strlen(str)+1, 0);
			}
			else
			{
				char str[16];
				snprintf(str, sizeof(str), "exit;%d",client_id);
				// printf("send exit\n");
				// send "exit" to notify the server that this is shutting down
				send(sd, str, strlen(str)+1, 0);
				close_connection();
			}
		}
		else
		{
			// error in mq_receive()
			int err = errno;
			
			// EAGAIN is returned when the queue is empty
			if((err == EBADF) && (client_state != CLIENT_DEAD))
				// client has already been killed.
				break;

			if(err != EAGAIN)
			{
				syslog(LOG_INFO, "errno %d\n", err);
				// error not expected
	       		panic("In mq_receive()");
	       	}
	       	// else, queue is empty
	    }
	}

	syslog(LOG_INFO, "Exiting th_send\n");
}

void close_connection(void)
{
	if(client_state == CLIENT_DEAD)
		return;

	// close the tcp connection
	shutdown(sd,SHUT_RD);
	shutdown(sd,SHUT_WR);
	shutdown(sd,SHUT_RDWR);	

	// closing the queue
	mq_close(msgq_send_id); 
	mq_close(msgq_recv_id); 
	
	// unlink message queue to remove it from the system
	if (mq_unlink(MSGQRECV_NAME) == -1)
 		panic("In mq_unlink()");

 	if (mq_unlink(MSGQSEND_NAME) == -1)
 		panic("In mq_unlink()");

 	client_state = CLIENT_DEAD;
 	syslog(LOG_INFO, "Cliend dead\n");
}

static void sig_handler(int signum)
{
	if(signum == SIGALRM)
	{
		client_state = CLIENT_AFK;
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
 * Client commands
 *******************************************************************/
int state_cb(int argc, char *argv[])
{
	// answer server request for client state
	char msg[128];
	snprintf(msg, sizeof(msg), "State;%d;%d",client_id, client_state);
	
	syslog(LOG_INFO, "State cb '%s'\n", msg);
	// send client state
	send(sd, msg, strlen(msg)+1, 0);

	return 0;
}

int recv_id_cb(int argc, char *argv[])
{
	if(argc < 2)
		return -1;

	// receive client ID through 'id' field
	client_id = atoi(argv[1]);
	syslog(LOG_INFO, "Recv id cb '%d'\n", client_id);

	// when ID is received we then can send the client name
	char msg[128];
	snprintf(msg, sizeof(msg), "Name;%d;%s",client_id, client_name);

	syslog(LOG_INFO, "Send name cb '%s'\n", msg);
	send(sd, msg, strlen(msg)+1, 0);
}

int recv_broad_cb(int argc, char *argv[])
{
	if(argc < 2)
		return -1;

	char str[64];
	snprintf(str, sizeof(str), "Server: %s", argv[1]);

	// insert message into msgqueue
	if(mq_send(msgq_recv_id, str, strlen(str)+1, 1) != 0)
		panic("In mq_send()");
}

int recv_msg_cb(int argc, char *argv[])
{
	if(argc < 4)
		return -1;

	char str[64];
	snprintf(str, sizeof(str), "%s(%d) said: %s", argv[2], atoi(argv[1]), argv[3]);

	// insert message into msgqueue
	if(mq_send(msgq_recv_id, str, strlen(str)+1, 1) != 0)
		panic("In mq_send()");
}

int exit_cb(int argc, char *argv[])
{
	syslog(LOG_INFO, "Exit cb\n");
	close_connection();
}
