#ifndef __PARSER_H__
#define __PARSER_H__
 
#include <stdint.h>

/********************************************************************
 * Errors definitions
 *******************************************************************/
#define ECMDNF  	1 // Command not found
#define ENOCMD  	2 // Command is empty

#define ENOLIST 	3 // List is empty
#define ENOMEM  	4 // No memory available or bad allocation of memory
#define EPERM 		5 // Permission error

#define EINVARG 	6 // Invalid (number of) argument(s)
#define EALREADY 	7 // Operation already in progress
#define ENOP		8 // No operation in progress

#define ENOKEY		9 // Required parameter/key not defined

/********************************************************************
 * Command
 *******************************************************************/
typedef int (*Command_cb)(int, char *[]); // callback - function pointer

/** \brief simple struct to hold data for a single command */
typedef struct Command
{
	const char *cmd;            // the command string to match against
	// const char *help;         // the help text associated with cmd
	Command_cb fn;              // the function to call when cmd is matched
} Command_t;

/********************************************************************
 * Function prototypes
 *******************************************************************/
#define DELIMETER ";" // command arguments delimiter

char parse_cmd(const Command_t cmd_list[], const char *str);

#endif // !__PARSER_H__
