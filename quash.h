/**
	* @file quash.h
	*
  * Blake Morrell & Matthew Felsen
  *
  * EECS 678 Spring 20'
	*
	* Quash header file.
	*/

// -- Define Via StackOverflow to get rid of crying execvpe warnings --
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
	* Command structure to hold information regarding given
	*
	*/
typedef struct m_command {

	// -- 2d command arrary --
	char** m_c_tok;

	// -- character array to store a command string --
	char q_comm_str[512];

	// -- length of the q_comm_str character buffer
	size_t comm_siz;

	// -- tokenized command array length --
	size_t symb_size;
} m_command;


//	Job struct for handeling a running process

typedef struct m_job {
	// -- command string for job proccess --
	char* j_comm_str;

	// -- process running true, false otherwise --
	bool running;

	// -- process id --
	int pid;

	// -- job id --
	int jid;
} m_job;

	// -- Signal Masking variables --

sigset_t sigmask_1;

sigset_t sigmask_2;

// -- Parsing assist functions --

char* substring(char* str, int begin, int end);


/**
	* @brief Determine if Quash gets more commands
	*
	* @return T if program is receiving commands, F otherwise
	*/
bool m_get_command();

/**
	* @brief suspends execution loop
	*/
void suspend();

/**
	* @brief suspends quash file execution
	*/
void suspend_file_exec();

/**
	* @brief sig mask
	* example Signals Lab05
	* @param signal int
 */
void sig_mask(int sig);

/**
	* @brief sig unmask
	* example Signals Lab05
	*
	* @param signal int
 */
void sig_unmask(int sig);


/**
	* @brief Print current directory before shell input
	*/
void curr_dir_print();


/**
	* @brief background job handler
	*
	* @param signal int
	* @param sig struct
	* @param pos
	*/
void m_handle_job(int signal, siginfo_t* sig, void* pos);


/**
	* @brief Make fork for file redirection
	*
	* @param qcommd command struct
	* @param fdi file descriptor in
	* @param fdo file descriptor out
	* @param envp environment variables
	* @return exit status
	*/
int m_fork_assist (m_command* qcommd, int fdi, int fdo, char* envp[]);

/**
	* @brief Parse Raw Command string
	*
	* @param qcommd command struct
	* @param in instream
	* @return bool successful parse
	*/
bool m_cmd_parse(m_command* qcommd, FILE* in);

/**
	* @brief command cd
	*
	* @param qcommd command struct
  */
void cd(m_command* qcommd);

/**
	* @brief command echo
	*
	* @param qcommd command struct
 */
void echo(m_command* qcommd);

/**
	* @brief command jobs
	*
	* @param qcommd command struct
	*/
void jobs(m_command* qcommd);

/**
	* @brief command set
	*
	* Assigns the specified environment variable (HOME or PATH)
	*
	* @param qcommd command struct
 */
void set(m_command* qcommd);


/**
	* @brief file command function
	*
	* @param argc argument count from the command line
	* @param argv argument vector from the command line
	* @param envp environment variables
	*/
void m_command_file(char** argv, int argc, char* envp[]);



/**
	* @brief run quash command
	*
	* @param qcommd command struct
	* @param envp environment variables
	*/
void quash_run(m_command* qcommd, char** envp);


/**
	* @brief command logic
	*
	* @param qcommd command struct
	* @param envp environment variables
	* @return exit status
 */
int command_logic(m_command* qcommd, char* envp[]);

/**
	* @brief runs any command that can be handled with execvpe
	*
	* @param qcommd command struct
	* @param envp environment variables
	* @return exit status
 */
int prim_cmd(m_command* qcommd, char* envp[]);


/**
	* @brief runs any command with an I/O redirection present
	*
	* @param qcommd command struct
	* @param io true is stdin, false is stdout
	* @param envp environment variables
	* @return exit status
	*/
int input_io_cmd(m_command* qcommd, bool i_o, char* envp[]);

/**
	* @brief runs any command with a "&" present
	*
	* @param qcommd command struct
	* @param envp environment variables
	* @return exit status
	*/
int m_cmd_background(m_command* qcommd, char* envp[]);

/**
	* @brief runs any command with a "|" present
	*
	* @param qcommd command struct
	* @param envp environment array
	* @return exit status
	*/
int m_command_pipe(m_command* qcommd, char* envp[]);

#endif // QUASH_H
