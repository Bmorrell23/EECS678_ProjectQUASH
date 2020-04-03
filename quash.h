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
#define M_COMMAND_NAME (128)
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
#define MAX_NUM_JOBS (50)

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


//	Job struct for handeling a running process

typedef struct m_job {
	char* cmdstr;							///< The command issued for this process
	bool running;							///< Status for this process (running or not)
	int pid;								///< Process ID #
	int jid;								///< Job ID #
} m_job;




	//Signal Masking variables

sigset_t sigmask_1;

sigset_t sigmask_2;



/**************************************************************************
 * Helper Functions
 **************************************************************************/

char* substring(char* str, int begin, int end);


/**
	* Determine if Quash gets more commands
	*
	* @return T if program is receiving commands, F otherwise
	*/
bool m_get_command();

/**
	* suspends execution loop
	*/
void suspend();

/**
	* suspends quash file execution
	*/
void suspend_from_file();

/**
	* sig mask
	*
	* @param signal int
 */
void sig_mask(int sig);

/**
	* sig unmask
	*
	* @param signal integer
 */
void sig_unmask(int sig);


/**
	* Print Current Working Directory before shell commands
	*/
void curr_dir_print();


/**
	* Print tokens from a cmd struct
	*
	* @param cmd command struct
	*/
void print_cmd_tokens(m_command* cmd);

/**
	* Handles exiting signal from background processes
	*
	* @param signal int
	* @param sig struct
	* @param slot
	*/
void m_handle_job(int signal, siginfo_t* sig, void* slot);

/*
	* Kill Command from jobs listing
	*
	* @param cmd command struct
	* @return: RETURN_CODE
*/
int kill_proc(m_command* cmd);

/**
	* Make fork for file redirection
	*
	* @param cmd command struct
	* @param fsi file descriptor in
	* @param fso file descriptor out
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int m_fork_assist (m_command* cmd, int fsi, int fso, char* envp[]);


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
bool m_cmd_eval(m_command* cmd, FILE* in);

/**
	* command cd
	*
	* @param cmd command struct
	* Note: chdir will make new dir's if they don't exist
 */
void cd(m_command* cmd);

/**
	* command echo
	*
	* @param cmd command struct
 */
void echo(m_command* cmd);

/**
	* command jobs
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
void m_command_file(char** argv, int argc, char* envp[]);

/**************************************************************************
 * Running Quash Functions
 **************************************************************************/

/**
	* Execute Quash command
	*
	* @param cmd command struct
	* @param envp environment variables
	*/
void quash_run(m_command* cmd, char** envp);

/**
	* Executes any command that can be handled with execvpe (ergo free of |, <, >, or &)
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
 */
int prim_cmd(m_command* cmd, char* envp[]);

/**
	* Command logic
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
 */
int command_logic(m_command* cmd, char* envp[]);


/**
	* Executes any command with an I/O redirection present
	*
	* @param cmd command struct
	* @param io true is stdin, false is stdout
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int input_io_cmd(m_command* cmd, bool io, char* envp[]);

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
	* @param envp environment array
	* @return RETURN_CODE
	*/
int m_command_pipe(m_command* cmd, char* envp[]);

#endif // QUASH_H
