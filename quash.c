/**
 * @file quash.c
 *
 * Blake Morrell & Matthew Felsen
 *
 * EECS 678 Spring 20'
 *
 * Project Quash .c
 */
#include "quash.h"

 // Private global variables

static bool m_file_run;

static bool m_running;

static struct m_job all_jobs[MAX_NUM_JOBS];

static int num_jobs = 0;

/**************************************************************************
 * Private Functions
 **************************************************************************/
/**
 * Start the main loop by setting the running flag to true
	*/
static void start_quash()
{
	m_running = true;
}

/**
	* Flag file commands supply
	*/
static void start_from_file()
{
	m_file_run = true;
}

/**************************************************************************
 * Helper Functions
 **************************************************************************/

/**
	* Query if quash should accept more input or not.
	*
	* @return True if Quash should accept more input, otherwise false
	*/
bool m_get_command()
{
	return m_running || m_file_run;
}

/**
	* Causes the execution loop to end.
	*/
void suspend()
{
	m_running = false;
}

/**
	* suspends quash file execution
	*/
void suspend_from_file()
{
	m_file_run = false;
}

/**
	* Mask Signal
	*	example Signals Lab05
	* silences signals during quash execution for safety
	* @param signal integer
 */
void sig_mask(int sig)
{
	printf("\n");
}

/**
	* sig unmask
	* example Signals Lab05
	* @param signal integer
 */
void sig_unmask(int sig)
{
	exit(0);
}

/**
	* Print tokens from a cmd struct
	*
	* @param cmd command struct
	*/
void print_cmd_tokens(m_command* cmd)
{
	int t = 0;
	puts("Struct Token String\n");
	for ( ;t <= cmd->toklen; t++)
		printf("%d: %s\n", t, cmd->m_c_tok[t]);
}

/**
	* Print Current Working Directory before shell commands
	*/
void curr_dir_print()
{
	char cwd[MAX_COMMAND_LENGTH];	//cwd arg - print before each shell command
	if ( getcwd(cwd, sizeof(cwd)) && !m_file_run )
		printf("\n[Quash: %s] q$ ", cwd);
}

/**
	* Handles exiting signal from background processes
	*
	* @param signal int
	* @param sig struct
	* @param slot
	*/
void m_handle_job(int signal, siginfo_t* sig, void* slot) {
	pid_t p = sig->si_pid;
	int i = 0;
	for ( ; i < num_jobs; ++i) {
		if ( all_jobs[i].pid == p )
			break;
	}
	if ( i < num_jobs ) {
		printf("\n[%d] %d finished %s\n", all_jobs[i].jid, p, all_jobs[i].cmdstr);
		all_jobs[i].running = true;
		free(all_jobs[i].cmdstr);
	}
}

/*
	* Kill Command from jobs listing
	*
	* @param cmd command struct
	* @return: Exit status
*/
int kill_proc(m_command* cmd) {
	////////////////////////////////////////////////////////////////////////////////
	// Kill requires 3 arguments
	////////////////////////////////////////////////////////////////////////////////
	if ( cmd->toklen == 3 ) {
		int k_sig;
		sscanf(cmd->m_c_tok[1], "%d", &k_sig);
		int num;
		sscanf(cmd->m_c_tok[2], "%d", &num);

		////////////////////////////////////////////////////////////////////////////////
		// Kill provided Job
		////////////////////////////////////////////////////////////////////////////////
		if ( all_jobs[num].pid )
			kill(all_jobs[num].pid, k_sig);
		else {
			printf("Error: process does not exist \n");
			return EXIT_FAILURE;
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	// Otherwise error status
	////////////////////////////////////////////////////////////////////////////////
	else
	{
		puts("kill: Incorrect syntax. provide 2 arguments:\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/**
	* Make fork for file redirection
	*
	* @param cmd command struct
	* @param fdi file descriptor in
	* @param fdo file descriptor out
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int m_fork_assist (m_command* cmd, int fdi, int fdo, char* envp[])
{
	pid_t p;

	if ( !(p = fork ()) ) {

		// -- file out implementation --

		if ( fdo != 1 ) {
			if ( dup2(fdo, STDOUT_FILENO) < 0 ) {
				fprintf(stderr, "\nstdout file direction error. ERRNO\"%d\"\n", errno);
				exit(EXIT_FAILURE);
			}
			close (fdo);
		}

		// -- file in implementation --

		if ( fdi != 0 ) {
			if ( dup2(fdi, STDIN_FILENO ) < 0) {
				fprintf(stderr, "\nstdin file direction error. ERRNO\"%d\"\n", errno);
				exit(EXIT_FAILURE);
			}
			close (fdi);
		}
		//------------------------------------------------------------------------------
		// cmd execute
		//-----------------------------------------------------------------------------
		if ( execvpe(cmd->m_c_tok[0], cmd->m_c_tok, envp) < 0	&& errno == 2 ) {
			fprintf(stderr, "Command: \"%s\" cannot be found.\n", cmd->m_c_tok[0]);
			exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "Command error %s. ERRNO\"%d\"\n", cmd->m_c_tok[0], errno);
			exit(EXIT_FAILURE);
		}
		return EXIT_SUCCESS;
	}
	return p;
}

/**
	* Parse Raw Command string
	*
	* @param cmd command struct
	* @param in instream
	* @return bool successful parse
	*/
bool m_cmd_eval(m_command* cmd, FILE* in) {
	if ( fgets(cmd->cmdstr, MAX_COMMAND_LENGTH, in) != NULL ) {
		size_t len = strlen(cmd->cmdstr);
		char last_char = cmd->cmdstr[len - 1];

		if ( last_char == '\n' || last_char == '\r' ) {
			// Remove trailing new line character.
			cmd->cmdstr[len - 1] = '\0';
			cmd->cmdlen = len - 1;
		}
		else
			cmd->cmdlen = len;

		//------------------------------------------------------------------------------
		// Empty Command return true - MUST BE HANDLED
		//------------------------------------------------------------------------------
		if ( !(int)cmd->cmdlen )
			return true;

		//------------------------------------------------------------------------------
		// Tokenize command arguments
		//------------------------------------------------------------------------------
		cmd->m_c_tok = malloc( sizeof(char*) * MAX_COMMAND_ARGLEN );
		cmd->toklen = 0;

		char* token = malloc( sizeof(char*) * MAX_COMMAND_ARGLEN );

		token = strtok (cmd->cmdstr," ");
		while ( token != NULL )
		{
			//debug print - printf ("%d: %s\n", (int)cmd->toklen, token);
			cmd->m_c_tok[cmd->toklen] = token;
			cmd->toklen++;
			token = strtok(NULL, " ");
		}

		free(token);
		//------------------------------------------------------------------------------
		// Remove NULL token from end
		//------------------------------------------------------------------------------
		cmd->m_c_tok[cmd->toklen] = '\0';

		return true;
	}
	else
		return false;
}

/**
	*
	* @param original string
	* @param beginning of substring
	* @param end of substring
	* @return substring
	*/
char* substring(char* str, int begin, int end)
{
	char* substr = malloc(strlen(str));
	int i = 0;
	int j = 0;
	for(i = begin; i < end; i++,j++)
	{
		memcpy(&substr[j],&str[i],sizeof(str[i]));
	}
	return substr;
}

/**************************************************************************
 * Shell Fuctionality
 **************************************************************************/

/**
	* CD Implementation
	*
	* @param cmd command struct
	* @return void
	* Note: chdir will make new dir's if they don't exist
	*/
void cd(m_command* cmd)
{
	if ( cmd->toklen < 2 ) {
		if ( chdir(getenv("HOME")) )
			printf("cd: %s: Cannot navigate to $HOME\n", getenv("HOME"));
	}
	else if ( cmd->toklen > 2 )
		puts("Too many arguments");
	else {
		if ( chdir(cmd->m_c_tok[1]) )
			printf("cd: %s: No such file or directory\n", cmd->m_c_tok[1]);
	}
}

/**
	* Echo Implementation
	*
	* @param cmd command struct
	* @return void
	*/
void echo(m_command* cmd)
{
	if ( cmd->toklen == 2 ) {
		if ( !strcmp(cmd->m_c_tok[1], "$HOME") )
			puts(getenv("HOME"));
		else if ( !strcmp(cmd->m_c_tok[1], "$PATH") )
			puts(getenv("PATH"));
		else if ( strstr(cmd->m_c_tok[1], "$") != NULL )
			puts(getenv(substring(cmd->m_c_tok[1],1,strlen(cmd->m_c_tok[1]))));
		else
			puts(cmd->m_c_tok[1]);
	}
	else if ( cmd->toklen == 1 ) {
		puts(getenv("HOME"));
	}
	else{
		int i = 1;
		for ( ; i < cmd->toklen; i++ )
			printf("%s ", cmd->m_c_tok[i]);
		puts("");
	}
}

/**
	* Displays all currently running jobs
	* @return void
	*/
void jobs(m_command* cmd) {
	int i;

	for ( i = 0; i < num_jobs; i++ ) {
		if ( kill(all_jobs[i].pid, 0) == 0 && !all_jobs[i].running ) {
			printf("[%d] %d %s \n", all_jobs[i].jid, all_jobs[i].pid, all_jobs[i].cmdstr);
		}
	}
}

/**
	* Command "Set" Implementation
	*
	* Assigns the specified environment variable (HOME or PATH),
	* or displays an error for user mistakes.
	*
	* @param cmd command struct
	* @return void
	*/
void set(m_command* cmd) {
	if ( cmd->m_c_tok[1] == NULL )
		printf("set error: No command given\n");
	else {
		// Get the environment variable and directory
		// (Delimited by '=')
		char* env = strtok(cmd->m_c_tok[1], "=");
		char* dir = strtok(NULL, "=");

		if ( env == NULL || dir == NULL ) {
			printf("set: Incorrect syntax. Possible Usages:\n");
			printf("\tset PATH=/directory/to/use/for/path\n");
			printf("\tset HOME=/directory/to/use/for/home\n");
		}
		////////////////////////////////////////////////////////////////////////////////
		// Set the environment variable
		////////////////////////////////////////////////////////////////////////////////
		else //if ( !strcmp(env, "PATH") || !strcmp(env, "HOME") )
			setenv(env, dir, 1);
		//else
			//printf("set: available only for PATH or HOME environment variables\n");
	}
}

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
void m_command_file(char** argv, int argc, char* envp[]) {

	////////////////////////////////////////////////////////////////////////////////
	// Args
	////////////////////////////////////////////////////////////////////////////////
	m_command cmd;

	////////////////////////////////////////////////////////////////////////////////
	// Redirect Quash Standard Input
	////////////////////////////////////////////////////////////////////////////////
	start_from_file();

	////////////////////////////////////////////////////////////////////////////////
	// Command Loop
	////////////////////////////////////////////////////////////////////////////////
	while (m_cmd_eval(&cmd, stdin)) {
		quash_run(&cmd, envp);
	}

	////////////////////////////////////////////////////////////////////////////////
	// suspend File execution and start normal program execution
	////////////////////////////////////////////////////////////////////////////////
	suspend_from_file();
}

/**************************************************************************
 * Execution Functions
 **************************************************************************/

/**
	* Runs the specified Quash command
	*
	* @param cmd command struct
	* @param envp environment variables
	*/
void quash_run(m_command* cmd, char** envp) {
	////////////////////////////////////////////////////////////////////////////////
	// Command Decision Structure
	////////////////////////////////////////////////////////////////////////////////
	if ( !strcmp(cmd->cmdstr, "exit") || !strcmp(cmd->cmdstr, "quit") )
		suspend(); // Exit Quash
	////////////////////////////////////////////////////////////////////////////////
	// Do nothing -- just print the cwd to display we're still in the shell.
	////////////////////////////////////////////////////////////////////////////////
	else if ( !cmd->cmdlen ) {}
	else if ( strcmp(cmd->m_c_tok[0], "cd") == 0 )
		cd(cmd);
	else if ( strcmp(cmd->m_c_tok[0], "echo") == 0 )
		echo(cmd);
	else if ( !strcmp(cmd->m_c_tok[0], "jobs") )
		jobs(cmd);
	else if ( !strcmp(cmd->m_c_tok[0], "kill") )
		kill_proc(cmd);
	else if ( !strcmp(cmd->m_c_tok[0], "set") )
		set(cmd);
	else
		command_logic(cmd, envp);

	if ( m_running )
		curr_dir_print();
}

/**
	* Command Logic Structure
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int command_logic(m_command* cmd, char* envp[])
{
	////////////////////////////////////////////////////////////////////////////////
	// Command Flag Initializations
	////////////////////////////////////////////////////////////////////////////////
	bool b_bool = false;	//background
	bool i_bool = false;	//input redirection
	bool o_bool = false;	//output redirection
	bool p_bool = false;	//pipe

	////////////////////////////////////////////////////////////////////////////////
	// Walk command tokens and flip flags
	////////////////////////////////////////////////////////////////////////////////
	int i = 1;
	for (; i < cmd->toklen; i++) {
		if ( !strcmp(cmd->m_c_tok[i], "&") )
			b_bool = true;
		else if ( !strcmp(cmd->m_c_tok[i], "<") )
			i_bool = true;
		else if ( !strcmp(cmd->m_c_tok[i], ">") )
			o_bool = true;
		else if ( !strcmp(cmd->m_c_tok[i], "|") )
			p_bool = true;
	}

	//run command

	int RETURN_CODE = 0;

	if ( b_bool )
	{
		cmd->m_c_tok[cmd->toklen - 1] = '\0'; // remove & token
		cmd->toklen--;
		RETURN_CODE = exec_backg_command(cmd, envp);
	}
	else if ( i_bool )
		RETURN_CODE = input_io_cmd(cmd, true, envp);
	else if ( o_bool )
		RETURN_CODE = input_io_cmd(cmd, false, envp);
	else if ( p_bool )
		RETURN_CODE = m_command_pipe(cmd, envp);
	else
		RETURN_CODE = prim_cmd(cmd, envp);
	return RETURN_CODE;
}

/**
	* Run command with execvpe (ergo free of |, <, >, or &)
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int prim_cmd(m_command* cmd, char* envp[])
{
	// sig mask

	pid_t ret;
	int m_wait;
	signal(SIGINT, sig_mask);

	//Fork and check

	ret = fork();
	if ( ret < 0 )
	{
		fprintf(stderr, "Primary forking error. Error:%d\n", errno);
		exit(EXIT_FAILURE);
	}

	// -- parent p --

	if ( ret != 0 )
	{
		if ( waitpid(ret, &m_wait, 0) < 0 )
		{
			signal(SIGINT, sig_unmask);
			fprintf(stderr, "Primary forking error child	%d. ERRNO\"%d\"\n", ret, errno);
			return EXIT_FAILURE;
		}
		if ( WIFEXITED(m_wait) && WEXITSTATUS(m_wait) == EXIT_FAILURE )
			return EXIT_FAILURE;
		signal(SIGINT, sig_unmask);
		return EXIT_SUCCESS;
	}

	// -- child p --

	else
	{
		if ( execvpe(cmd->m_c_tok[0], cmd->m_c_tok, envp) < 0	&& errno == 2 )
		{
			fprintf(stderr, "Command: \"%s\" cannot be located.\n", cmd->m_c_tok[0]);
			exit(EXIT_FAILURE);
		}
		else
		{
			fprintf(stderr, "Run error %s. ERRNO\"%d\"\n", cmd->m_c_tok[0], errno);
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}
}

/**
	* Executes any command with an I/O redirection present
	*
	* @param cmd command struct
	* @param io true is stdin, false is stdout
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int input_io_cmd(m_command* cmd, bool io, char* envp[])
{
	////////////////////////////////////////////////////////////////////////////////
	// Mask Inturrept Signals and Initialize Variables
	////////////////////////////////////////////////////////////////////////////////
	pid_t p;
	int m_wait;
	int file_desc;
	signal(SIGINT, sig_mask);

	////////////////////////////////////////////////////////////////////////////////
	// Fork And Verify Process
	////////////////////////////////////////////////////////////////////////////////
	p = fork();
	if ( p < 0 )
	{
		fprintf(stderr, "Error forking redir command. ERRNO\"%d\"\n", errno);
		exit(EXIT_FAILURE);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Parent
	////////////////////////////////////////////////////////////////////////////////
	if ( p != 0 )
	{
		if (waitpid(p, &m_wait, 0) == -1)
		{
			fprintf(stderr, "Error with redir command's child	%d. ERRNO\"%d\"\n", p, errno);
			return EXIT_FAILURE;
		}
		if ( WIFEXITED(m_wait) && WEXITSTATUS(m_wait) == EXIT_FAILURE )
			return EXIT_FAILURE;

		signal(SIGINT, sig_unmask);
		return EXIT_SUCCESS;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Child
	////////////////////////////////////////////////////////////////////////////////
	else
	{
		////////////////////////////////////////////////////////////////////////////////
		// Initialize and Verify File Descriptor
		////////////////////////////////////////////////////////////////////////////////
		if ( io )
			file_desc = open(cmd->m_c_tok[cmd->toklen - 1], O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		else
			file_desc = open(cmd->m_c_tok[cmd->toklen - 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if ( file_desc < 0 ) {
			fprintf(stderr, "\nError opening %s. ERRNO\"%d\"\n", cmd->m_c_tok[cmd->toklen - 1], errno);
			exit(EXIT_FAILURE);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Redirect I/O Streams
		////////////////////////////////////////////////////////////////////////////////
		if ( io )
		{
			if (dup2(file_desc, STDIN_FILENO) < 0)
			{
				fprintf(stderr, "\nError redirecting STDIN to %s. ERRNO\"%d\"\n", cmd->m_c_tok[cmd->toklen - 1], errno);
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			if (dup2(file_desc, STDOUT_FILENO) < 0)
			{
				fprintf(stderr, "\nError redirecting STDOUT to %s. ERRNO\"%d\"\n", cmd->m_c_tok[cmd->toklen - 1], errno);
				exit(EXIT_FAILURE);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// Execute Command - remove last two redirection arguments
		////////////////////////////////////////////////////////////////////////////////
		close(file_desc);
		cmd->m_c_tok[cmd->toklen - 1] = NULL;
		cmd->m_c_tok[cmd->toklen - 2] = NULL;
		cmd->toklen = cmd->toklen - 2;

		if ( execvpe(cmd->m_c_tok[0], cmd->m_c_tok, envp) < 0	&& errno == 2 ) {
			fprintf(stderr, "Command: \"%s\" not found.\n", cmd->m_c_tok[0]);
			exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", cmd->m_c_tok[0], errno);
			exit(EXIT_FAILURE);
		}
		signal(SIGINT, sig_unmask);
		exit(EXIT_SUCCESS);
	}
}

/**
	* Executes any command with an & present
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int exec_backg_command(m_command* cmd, char* envp[])
{
	pid_t p;
	int m_wait;
	int file_desc;

	////////////////////////////////////////////////////////////////////////////////
	// signal masking struct
	////////////////////////////////////////////////////////////////////////////////
	struct sigaction action;
	action.sa_sigaction = *m_handle_job;
	action.sa_flags = SA_SIGINFO | SA_RESTART ;
	if ( sigaction(SIGCHLD, &action, NULL) < 0 )
		fprintf(stderr, "Background signal handler error: ERRNO\"%d\"\n", errno);
	sigprocmask(SIG_BLOCK, &sigmask_1, &sigmask_2);

	////////////////////////////////////////////////////////////////////////////////
	// Fork And Verify Process
	////////////////////////////////////////////////////////////////////////////////
	p = fork();
	if ( p < 0 )
	{
		fprintf(stderr, "\nEncounter error forking background command. ERRNO\"%d\"\n", errno);
		exit(EXIT_FAILURE);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Parent
	////////////////////////////////////////////////////////////////////////////////
	if ( p != 0 )
	{

		//Create new job process struct

		struct m_job create_job;
		create_job.cmdstr = (char*) malloc(M_COMMAND_NAME);
		strcpy(create_job.cmdstr, cmd->m_c_tok[0]);
		create_job.running = false;
		create_job.pid = p;
		create_job.jid = num_jobs;


		// -- Background job scruct --

		printf("[%d] %d running in background\n", num_jobs, p);
		all_jobs[num_jobs] = create_job;
		num_jobs++;


		// Signal unmasking, wait for finish

		sigprocmask(SIG_UNBLOCK, &sigmask_1, &sigmask_2);
		while (waitpid(p, &m_wait, WNOHANG) > 0) {}
		return EXIT_SUCCESS;

	}


	// child process after fork

	else {
		////////////////////////////////////////////////////////////////////////////////
		// Map Child Process to different output
		////////////////////////////////////////////////////////////////////////////////
		char temp_file[MAX_COMMAND_LENGTH];
		char cpid [MAX_COMMAND_ARGLEN];
		int child_pid = getpid();

		snprintf(cpid, MAX_COMMAND_ARGLEN,"%d",child_pid);
		strcpy(temp_file, cpid);
		strcat(temp_file, "-temp_file_output.out");

		file_desc = open(temp_file, O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if ( file_desc < 0 ) {
			fprintf(stderr, "\nError opening %s. ERRNO\"%d\"\n", temp_file, errno);
			exit(EXIT_FAILURE);
		}

		if ( dup2(file_desc, STDOUT_FILENO) < 0 ) {
			fprintf(stderr, "\nError redirecting STDOUT to %s. ERRNO\"%d\"\n", temp_file, errno);
			exit(EXIT_FAILURE);
		}

		if ( (execvpe(cmd->m_c_tok[0], cmd->m_c_tok, envp) < 0)	&& (errno == 2) )
		{
			fprintf(stderr, "Command: \"%s\" not found.\n", cmd->m_c_tok[0]);
			exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", cmd->m_c_tok[0], errno);
			exit(EXIT_FAILURE);
		}
		signal(SIGINT, sig_unmask);
		exit(EXIT_SUCCESS);
	}

}

/**
	* Sort pipe commands
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int m_command_pipe(m_command* cmd, char* envp[])
{

	// Sig handle interrupts

	signal(SIGINT, sig_mask);


	// -- Distribute tokens into array --

	int i = 0, j = 0, k = 0, cmd_numbers;

	m_command* cmds = malloc(MAX_COMMAND_ARGLEN * (sizeof *cmds) );

	cmds[0].m_c_tok = malloc( sizeof(char*) * MAX_COMMAND_ARGLEN );

	for ( ; i < cmd->toklen; i++ ) {
		if ( !strcmp(cmd->m_c_tok[i], "|") ) {
			//matches pipe
			cmds[j].m_c_tok[k] = NULL;
			cmds[j].toklen = k;
			j++;
			cmds[j].m_c_tok = malloc( sizeof(char*) * MAX_COMMAND_ARGLEN );
			k = 0;
		}
		else {
			cmds[j].m_c_tok[k] = malloc( sizeof(char) * M_COMMAND_NAME );
			strcpy(cmds[j].m_c_tok[k], cmd->m_c_tok[i]);

			k++;
		}
	}
	cmds[j].m_c_tok[k] = NULL;
	cmds[j].toklen = k;
	cmd_numbers = j;


	i = 0;
	j = 0;
	int file_desc[2];

	////////////////////////////////////////////////////////////////////////////////
	// Create and link pipes
	////////////////////////////////////////////////////////////////////////////////
	for ( i = 0; i < cmd_numbers - 1; ++i ) {
		if ( pipe(file_desc) < 0 ) {
			fprintf(stderr, "\nError in pipe creation. ERRNO:%d\n", errno);
			return EXIT_FAILURE;
		}
		m_fork_assist(&cmds[i], j, file_desc[1], envp);
		close(file_desc [1]);
		j = file_desc[0];
	}

	if ( j != 0 )
		dup2(j, STDIN_FILENO);


	// Complete final command

	if ( execvpe(cmds[i].m_c_tok[0], cmds[i].m_c_tok, envp) < 0 )
	{
		fprintf(stderr, "Error completing final command: %s. ERRNO\"%d\"\n", cmd->m_c_tok[0], errno);
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, sig_unmask);
	return EXIT_SUCCESS;
}

/**
	* Quash entry point
	*
	* @param argc argument count from the command line
	* @param argv argument vector from the command line
	* @param envp environment variables
	* @return program exit status
	*/
int main(int argc, char** argv, char** envp) {


	// Signal Masking SIGINT

	sigemptyset(&sigmask_1);
	sigaddset(&sigmask_1, SIGCHLD);


	// Allow for commmand redirection if input comes from FILE

	if ( !isatty( (fileno(stdin) ) ) )
	{
		m_command_file(argv, argc, envp);
		return EXIT_SUCCESS;
	}

	// create command object

	m_command cmd;

	start_quash();

	puts("Starting Quash ... Hello!\nType \"quit\" or \"exit\" to leave this shell");

	curr_dir_print();

	while ( m_cmd_eval(&cmd, stdin) && m_get_command() )
	{
		quash_run(&cmd, envp);
	}

	return EXIT_SUCCESS;
}
