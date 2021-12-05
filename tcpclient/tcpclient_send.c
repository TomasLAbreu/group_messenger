#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <mqueue.h>   /* mq_* functions */

/********************************************************************
 * Private defines
 *******************************************************************/
// max length of a message queue
#define MAX_MSG_LEN_R     10000

/* name of the POSIX object referencing the queue */
#define MSGQSEND_NAME    "/send_queue"
#define MSGQRECV_NAME    "/recv_queue"

/********************************************************************
 * Error signaling
 *******************************************************************/
void panic(char *msg);
#define panic(m)	{perror(m); abort();}

/********************************************************************
 * Send a message to the server, via msgqueue
 *******************************************************************/
int main(int argc, char *argv[])
{
	mqd_t msgq_recv_id;
	mqd_t msgq_send_id;

	/* opening the queue in NONBLOCK mode */
  	msgq_recv_id = mq_open(MSGQRECV_NAME, O_RDWR | O_NONBLOCK,
  												S_IRWXU | S_IRWXG, NULL);

   	if (msgq_recv_id == (mqd_t)-1)
   		panic("In mq_open()");

	/* opening the queue in NONBLOCK mode */
  	msgq_send_id = mq_open(MSGQSEND_NAME, O_RDWR | O_NONBLOCK,
  												S_IRWXU | S_IRWXG, NULL);

   	if (msgq_send_id == (mqd_t)-1)
   		panic("In mq_open()");

	// msg queue sucessfuly open
	char msg[MAX_MSG_LEN_R];
	struct mq_attr msgq_attr;

	// get attributes from queue
	mq_getattr(msgq_recv_id, &msgq_attr);
	printf("(%ld messages to read)\n", msgq_attr.mq_curmsgs);

	while(1)
	{
		// read message from msgqueue
		if(mq_receive(msgq_recv_id, msg, MAX_MSG_LEN_R, NULL) == -1)
		{
			// get error from errno
			int err = errno;

			// is the queue empty?
			if(err == EAGAIN)
				// no more messages to read
				break;
			
			// else, error not expected
			panic("In mq_receive()");
		}
		
		// else, print queued message
		puts(msg);
	}

	// send messages, passed by arguments via command line args
	for(int i = 1; i < argc; i++)
	{
		// printf("Sending '%s'\n", argv[i]);
		/* sending the message */
		if(mq_send(msgq_send_id, argv[i], strlen(argv[i])+1, 1) != 0)
			panic("In mq_send()");
	}

	// close queue
	mq_close(msgq_recv_id);
	mq_close(msgq_send_id);
	return 0;
}