/**
	* @file quash.h
	*
  * Blake Morrell & Matthew Felsen
  *
  * EECS 678 Spring 20'
	*
	* Quash header file.
	*/


#define _GNU_SOURCE


#ifndef QUASH_H
#define QUASH_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>



/**
	* Specify the maximum number of characters accepted by the command string
	*/
#define MAX_COMMAND_TITLE (128)
/**
	* Specify the maximum number of arguments in a command
	*/

#define MAX_COMMAND_LENGTH (1024)

#define MAX_COMMAND_ARGLEN (32)
/**
	* Specify the maximum number of characters accepted by the command string
	*/
/**
	* Specify the maximum number of background jobs
	*/
#define MAX_NUM_JOBS (100)

/**
	* Command structure to hold information regarding given
	*
	*/
typedef struct m_command {
	char** m_c_tok;							///< tokenized command array
	char cmdstr[MAX_COMMAND_LENGTH];	///< character buffer to store the
										///< command string. You may want
										///< to modify this to accept
										///< arbitrarily long strings for
										///< robustness.
	size_t cmdlen;						///< length of the cmdstr character buffer
	size_t toklen;						///< tokenized command array length
} m_command;


	//Signal Masking variables

sigset_t sigmask_1;

sigset_t sigmask_2;


	//	Job struct for handeling a running process

typedef struct m_job {
	char* cmdstr;							///< The command issued for this process
	bool status;							///< Status for this process (running or not)
	int pid;								///< Process ID #
	int jid;								///< Job ID #
} m_job;

/**************************************************************************
 * Helper Functions
 **************************************************************************/

/**
	* Query if quash should accept more input or not.
	*
	* @return True if Quash should accept more input and false otherwise
	*/
bool is_running();

/**
	* Causes the execution loop to end.
	*/
void suspend();

/**
	* suspends quash file execution
	*/
void suspend_from_file();

/**
	* Mask Signal
	*
	* silences signals during quash execution for safety
	*
	* @param signal integer
 */
void mask_signal(int signal);

/**
	* Unmask Signal
	*
	* @param signal integer
 */
void unmask_signal(int signal);

/**
	* Print tokens from a cmd struct
	*
	* @param cmd command struct
	*/
void print_cmd_tokens(m_command* cmd);

/**
	* Print Current Working Directory before shell commands
	*/
void print_init();

/**
	* Handles exiting signal from background processes
	*
	* @param signal int
	* @param sig struct
	* @param slot
	*/
void job_handler(int signal, siginfo_t* sig, void* slot);

/*
	* Kill Command from jobs listing
	*
	* @param cmd command struct
	* @return: RETURN_CODE
*/
int kill_proc(m_command* cmd);

/**
	* Creates forks and redirects file streams for use in iterative fashion
	*
	* @param cmd command struct
	* @param fsi file descriptor in
	* @param fso file descriptor out
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int iterative_fork_helper (m_command* cmd, int fsi, int fso, char* envp[]);

/**************************************************************************
 * String Manipulation Functions
 **************************************************************************/

/**
	*  Read in a command and setup the #m_command struct. Also perform some minor
	*  modifications to the string to remove trailing newline characters.
	*
	*  @param cmd - a m_command structure. The #m_command.cmdstr and
	*               #m_command.cmdlen fields will be modified
	*  @param in - an open file ready for reading
	*  @return True if able to fill #m_command.cmdstr and false otherwise
	*/
bool get_command(m_command* cmd, FILE* in);

/**************************************************************************
 * Shell Fuctionality
 **************************************************************************/

/**
	* CD Implementation
	*
	* @param cmd command struct
	* Note: chdir will make new dir's if they don't exist
 */
void cd(m_command* cmd);

/**
	* Echo Implementation
	*
	* @param cmd command struct
 */
void echo(m_command* cmd);

/**
	* Displays all currently running jobs
	*/
void jobs(m_command* cmd);

/**
	* Set Implementation
	*
	* Assigns the specified environment variable (HOME or PATH),
	* or displays an error for user mistakes.
	*
	* @param cmd command struct
 */
void set(m_command* cmd);

/**************************************************************************
 * File Execution Functions
 **************************************************************************/

/**
	* Executes any Quash commands from the given file
	*
	* @param argc argument count from the command line
	* @param argv argument vector from the command line
	* @param envp environment variables
	*/
void exec_from_file(char** argv, int argc, char* envp[]);

/**************************************************************************
 * Execution Functions
 **************************************************************************/

/**
	* Runs the specified Quash command
	*
	* @param cmd command struct
	* @param envp environment variables
	*/
void quash_run(m_command* cmd, char** envp);

/**
	* Command logic struct
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
 */
int command_logic(m_command* cmd, char* envp[]);

/**
	* Executes any command that can be handled with execvpe (ergo free of |, <, >, or &)
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
 */
int exec_basic_command(m_command* cmd, char* envp[]);

/**
	* Executes any command with an I/O redirection present
	*
	* @param cmd command struct
	* @param io true is stdin, false is stdout
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int exec_redir_command(m_command* cmd, bool io, char* envp[]);

/**
	* Executes any command with an & present
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int exec_backg_command(m_command* cmd, char* envp[]);

/**
	* Executes any command with an | present
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int exec_pipe_command(m_command* cmd, char* envp[]);

#endif // QUASH_H
