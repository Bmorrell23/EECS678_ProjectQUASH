/**
 * @file quash.c
 *
 * Blake Morrell
 * Matthew Felsen
 * EECS 678 Spring 20'
 *
 * Project 1 Quash's main file
 */

/**************************************************************************
 * Included Files
 **************************************************************************/ 
#include "quash.h"	// Putting this above the other includes allows us to ensure
					// this file's headder's #include statements are self
					// contained.

/**************************************************************************
 * Private Variables
 **************************************************************************/
/**
 * Keep track of whether Quash should request another command or not.
	*/
// NOTE: "static" causes the "running" variable to only be declared in this
// compilation unit (this file and all files that include it). This is similar
// to private in other languages.
static bool running;

static bool running_from_file;

static struct job all_jobs[MAX_NUM_JOBS];

static int num_jobs = 0;

/**************************************************************************
 * Private Functions
 **************************************************************************/
/**
 * Start the main loop by setting the running flag to true
	*/
static void start() {
	running = true;
}

/**
	* Flag file commands supply
	*/
static void start_from_file() {
	running_from_file = true;
}

/**************************************************************************
 * Helper Functions
 **************************************************************************/

/**
	* Query if quash should accept more input or not.
	*
	* @return True if Quash should accept more input and false otherwise
	*/
bool is_running() {
	return running || running_from_file;
}

/**
	* Causes the execution loop to end.
	*/
void terminate() {
	running = false;
}

/**
	* Terminates quash file execution
	*/
void terminate_from_file() {
	running_from_file = false;
}

/**
	* Mask Signal
	*
	* silences signals during quash execution for safety
	*
	* @param signal integer
 */
void mask_signal(int signal) {
	printf("\n");
}

/**
	* Unmask Signal
	*
	* @param signal integer
 */
void unmask_signal(int signal) {
	exit(0);
}

/**
	* Print tokens from a cmd struct
	*
	* @param cmd command struct
	*/
void print_cmd_tokens(command_t* cmd) {
	int i = 0;
	puts("Struct Token String\n");
	for ( ;i <= cmd->toklen; i++)
		printf("%d: %s\n", i, cmd->tok[i]);
}

/**
	* Print Current Working Directory before shell commands
	*/
void print_init() {
	char cwd[MAX_COMMAND_LENGTH];	//cwd arg - print before each shell command
	if ( getcwd(cwd, sizeof(cwd)) && !running_from_file )
		printf("\n[Quash: %s] q$ ", cwd);
}

/**
	* Handles exiting signal from background processes
	*
	* @param signal int
	* @param sig struct
	* @param slot
	*/
void job_handler(int signal, siginfo_t* sig, void* slot) {
	pid_t p = sig->si_pid;
	int i = 0;
	for ( ; i < num_jobs; ++i) {
		if ( all_jobs[i].pid == p )
			break;
	}
	if ( i < num_jobs ) {
		printf("\n[%d] %d finished %s\n", all_jobs[i].jid, p, all_jobs[i].cmdstr);
		all_jobs[i].status = true;
		free(all_jobs[i].cmdstr);
	}
}

/*
	* Kill Command from jobs listing
	*
	* @param cmd command struct
	* @return: RETURN_CODE
*/
int kill_proc(command_t* cmd) {
	////////////////////////////////////////////////////////////////////////////////
	// Kill requires 3 arguments
	////////////////////////////////////////////////////////////////////////////////
	if ( cmd->toklen == 3 ) {
		int ksignal;
		sscanf(cmd->tok[1], "%d", &ksignal);
		int num;
		sscanf(cmd->tok[2], "%d", &num);

		////////////////////////////////////////////////////////////////////////////////
		// Kill provided Job
		////////////////////////////////////////////////////////////////////////////////
		if ( all_jobs[num].pid )
			kill(all_jobs[num].pid,ksignal);
		else {
			printf("Error: process does not exist \n");
			return EXIT_FAILURE;
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	// Otherwise error status
	////////////////////////////////////////////////////////////////////////////////
	else {
		puts("kill: Incorrect syntax. provide 2 arguments:\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/**
	* Creates forks and redirects file streams for use in iterative fashion
	*
	* @param cmd command struct
	* @param fsi file descriptor in
	* @param fso file descriptor out
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int iterative_fork_helper (command_t* cmd, int fsi, int fso, char* envp[])
{
	pid_t p;

	if ( !(p = fork ()) ) {
		////////////////////////////////////////////////////////////////////////////////
		// Redirect for STDOUT
		////////////////////////////////////////////////////////////////////////////////
		if ( fso != 1 ) {
			if ( dup2(fso, STDOUT_FILENO) < 0 ) {
				fprintf(stderr, "\nError redirecting STDOUT. ERRNO\"%d\"\n", errno);
				exit(EXIT_FAILURE);
			}
			close (fso);
		}
		////////////////////////////////////////////////////////////////////////////////
		// Redirect for STDIN
		////////////////////////////////////////////////////////////////////////////////
		if ( fsi != 0 ) {
			if ( dup2(fsi, STDIN_FILENO ) < 0) {
				fprintf(stderr, "\nError redirecting STDIN. ERRNO\"%d\"\n", errno);
				exit(EXIT_FAILURE);
			}
			close (fsi);
		}
		////////////////////////////////////////////////////////////////////////////////
		// Execute Command
		////////////////////////////////////////////////////////////////////////////////
		if ( execvpe(cmd->tok[0], cmd->tok, envp) < 0	&& errno == 2 ) {
			fprintf(stderr, "Command: \"%s\" not found.\n", cmd->tok[0]);
			exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", cmd->tok[0], errno);
			exit(EXIT_FAILURE);
		}
		return EXIT_SUCCESS;
	}
	return p;
}

/**************************************************************************
 * String Manipulation Functions
 **************************************************************************/

/**
	* Parse Raw Command string
	*
	* @param cmd command struct
	* @param in instream
	* @return bool successful parse
	*/
bool get_command(command_t* cmd, FILE* in) {
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

		////////////////////////////////////////////////////////////////////////////////
		// Empty Command return true - MUST BE HANDLED
		////////////////////////////////////////////////////////////////////////////////
		if ( !(int)cmd->cmdlen )
			return true;

		////////////////////////////////////////////////////////////////////////////////
		// Tokenize command arguments
		////////////////////////////////////////////////////////////////////////////////
		cmd->tok = malloc( sizeof(char*) * MAX_COMMAND_ARGLEN );
		cmd->toklen = 0;

		char* token = malloc( sizeof(char*) * MAX_COMMAND_ARGLEN );

		token = strtok (cmd->cmdstr," ");
		while ( token != NULL )
		{
			//debug print - printf ("%d: %s\n", (int)cmd->toklen, token);
			cmd->tok[cmd->toklen] = token;
			cmd->toklen++;
			token = strtok(NULL, " ");
		}

		free(token);
		////////////////////////////////////////////////////////////////////////////////
		// Remove NULL token from end
		////////////////////////////////////////////////////////////////////////////////
		cmd->tok[cmd->toklen] = '\0';

		return true;
	}
	else
		return false;
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
void cd(command_t* cmd)
{
	if ( cmd->toklen < 2 ) {
		if ( chdir(getenv("HOME")) )
			printf("cd: %s: Cannot navigate to $HOME\n", getenv("HOME"));
	}
	else if ( cmd->toklen > 2 )
		puts("Too many arguments");
	else {
		if ( chdir(cmd->tok[1]) )
			printf("cd: %s: No such file or directory\n", cmd->tok[1]);
	}
}

/**
	* Echo Implementation
	*
	* @param cmd command struct
	* @return void
	*/
void echo(command_t* cmd)
{
	if ( cmd->toklen == 2 ) {
		if ( !strcmp(cmd->tok[1], "$HOME") )
			puts(getenv("HOME"));
		else if ( !strcmp(cmd->tok[1], "$PATH") )
			puts(getenv("PATH"));
		else
			puts(cmd->tok[1]);
	}
	else if ( cmd->toklen == 1 ) {
		puts(getenv("HOME"));
	}
	else{
		int i = 1;
		for ( ; i < cmd->toklen; i++ )
			printf("%s ", cmd->tok[i]);
		puts("");
	}
}

/**
	* Displays all currently running jobs
	* @return void
	*/
void jobs(command_t* cmd) {
	int i;

	for ( i = 0; i < num_jobs; i++ ) {
		if ( kill(all_jobs[i].pid, 0) == 0 && !all_jobs[i].status ) {
			printf("[%d] %d %s \n", all_jobs[i].jid, all_jobs[i].pid, all_jobs[i].cmdstr);
		}
	}
}

/**
	* Set Implementation
	*
	* Assigns the specified environment variable (HOME or PATH),
	* or displays an error for user mistakes.
	*
	* @param cmd command struct
	* @return void
	*/
void set(command_t* cmd) {
	if ( cmd->tok[1] == NULL )
		printf("set: No command given\n");
	else {
		// Get the environment variable and directory
		// (Delimited by '=')
		char* env = strtok(cmd->tok[1], "=");
		char* dir = strtok(NULL, "=");

		if ( env == NULL || dir == NULL ) {
			printf("set: Incorrect syntax. Possible Usages:\n");
			printf("\tset PATH=/directory/to/use/for/path\n");
			printf("\tset HOME=/directory/to/use/for/home\n");
		}
		////////////////////////////////////////////////////////////////////////////////
		// Set the environment variable
		////////////////////////////////////////////////////////////////////////////////
		else if ( !strcmp(env, "PATH") || !strcmp(env, "HOME") )
			setenv(env, dir, 1);
		else
			printf("set: available only for PATH or HOME environment variables\n");
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
void exec_from_file(char** argv, int argc, char* envp[]) {

	////////////////////////////////////////////////////////////////////////////////
	// Args
	////////////////////////////////////////////////////////////////////////////////
	command_t cmd;

	////////////////////////////////////////////////////////////////////////////////
	// Redirect Quash Standard Input
	////////////////////////////////////////////////////////////////////////////////
	start_from_file();

	////////////////////////////////////////////////////////////////////////////////
	// Command Loop
	////////////////////////////////////////////////////////////////////////////////
	while (get_command(&cmd, stdin)) {
		run_quash(&cmd, envp);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Terminate File execution and start normal program execution
	////////////////////////////////////////////////////////////////////////////////
	terminate_from_file();
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
void run_quash(command_t* cmd, char** envp) {
	////////////////////////////////////////////////////////////////////////////////
	// Command Decision Structure
	////////////////////////////////////////////////////////////////////////////////
	if ( !strcmp(cmd->cmdstr, "exit") || !strcmp(cmd->cmdstr, "quit") )
		terminate(); // Exit Quash
	////////////////////////////////////////////////////////////////////////////////
	// Do nothing -- just print the cwd to display we're still in the shell.
	////////////////////////////////////////////////////////////////////////////////
	else if ( !cmd->cmdlen ) {}
	else if ( strcmp(cmd->tok[0], "cd") == 0 )
		cd(cmd);
	else if ( strcmp(cmd->tok[0], "echo") == 0 )
		echo(cmd);
	else if ( !strcmp(cmd->tok[0], "jobs") )
		jobs(cmd);
	else if ( !strcmp(cmd->tok[0], "kill") )
		kill_proc(cmd);
	else if ( !strcmp(cmd->tok[0], "set") )
		set(cmd);
	else
		exec_command(cmd, envp);

	if ( running )
		print_init();
}

/**
	* Command Decision Structure
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int exec_command(command_t* cmd, char* envp[])
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
		if ( !strcmp(cmd->tok[i], "&") )
			b_bool = true;
		else if ( !strcmp(cmd->tok[i], "<") )
			i_bool = true;
		else if ( !strcmp(cmd->tok[i], ">") )
			o_bool = true;
		else if ( !strcmp(cmd->tok[i], "|") )
			p_bool = true;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Execute designated command
	////////////////////////////////////////////////////////////////////////////////
	int RETURN_CODE = 0;
	// we have access to numArgs here and this will be portable
	if ( b_bool ) {
		cmd->tok[cmd->toklen - 1] = '\0'; // remove & token
		cmd->toklen--;
		RETURN_CODE = exec_backg_command(cmd, envp);
	}
	else if ( i_bool )
		RETURN_CODE = exec_redir_command(cmd, true, envp);
	else if ( o_bool )
		RETURN_CODE = exec_redir_command(cmd, false, envp);
	else if ( p_bool )
		RETURN_CODE = exec_pipe_command(cmd, envp);
	else
		RETURN_CODE = exec_basic_command(cmd, envp);
	return RETURN_CODE;
}

/**
	* Executes any command that can be handled with execvpe (ergo free of |, <, >, or &)
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int exec_basic_command(command_t* cmd, char* envp[])
{
	////////////////////////////////////////////////////////////////////////////////
	// Mask Inturrept Signals and Initialize Variables
	////////////////////////////////////////////////////////////////////////////////
	pid_t p;
	int wait_status;
	signal(SIGINT, mask_signal);

	////////////////////////////////////////////////////////////////////////////////
	// Fork And Verify Process
	////////////////////////////////////////////////////////////////////////////////
	p = fork();
	if ( p < 0 ) {
		fprintf(stderr, "Error forking basic command. Error:%d\n", errno);
		exit(EXIT_FAILURE);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Parent
	////////////////////////////////////////////////////////////////////////////////
	if ( p != 0 ) {
		if ( waitpid(p, &wait_status, 0) < 0 ) {
			signal(SIGINT, unmask_signal);
			fprintf(stderr, "Error with basic command's child	%d. ERRNO\"%d\"\n", p, errno);
			return EXIT_FAILURE;
		}
		if ( WIFEXITED(wait_status) && WEXITSTATUS(wait_status) == EXIT_FAILURE )
			return EXIT_FAILURE;
		signal(SIGINT, unmask_signal);
		return EXIT_SUCCESS;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Child
	////////////////////////////////////////////////////////////////////////////////
	else {
		if ( execvpe(cmd->tok[0], cmd->tok, envp) < 0	&& errno == 2 ) {
			fprintf(stderr, "Command: \"%s\" not found.\n", cmd->tok[0]);
			exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", cmd->tok[0], errno);
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
int exec_redir_command(command_t* cmd, bool io, char* envp[])
{
	////////////////////////////////////////////////////////////////////////////////
	// Mask Inturrept Signals and Initialize Variables
	////////////////////////////////////////////////////////////////////////////////
	pid_t p;
	int wait_status;
	int file_desc;
	signal(SIGINT, mask_signal);

	////////////////////////////////////////////////////////////////////////////////
	// Fork And Verify Process
	////////////////////////////////////////////////////////////////////////////////
	p = fork();
	if ( p < 0 ) {
		fprintf(stderr, "Error forking redir command. ERRNO\"%d\"\n", errno);
		exit(EXIT_FAILURE);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Parent
	////////////////////////////////////////////////////////////////////////////////
	if ( p != 0 ) {
		if (waitpid(p, &wait_status, 0) == -1) {
			fprintf(stderr, "Error with redir command's child	%d. ERRNO\"%d\"\n", p, errno);
			return EXIT_FAILURE;
		}
		if ( WIFEXITED(wait_status) && WEXITSTATUS(wait_status) == EXIT_FAILURE )
			return EXIT_FAILURE;

		signal(SIGINT, unmask_signal);
		return EXIT_SUCCESS;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Child
	////////////////////////////////////////////////////////////////////////////////
	else {
		////////////////////////////////////////////////////////////////////////////////
		// Initialize and Verify File Descriptor
		////////////////////////////////////////////////////////////////////////////////
		if ( io )
			file_desc = open(cmd->tok[cmd->toklen - 1], O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		else
			file_desc = open(cmd->tok[cmd->toklen - 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if ( file_desc < 0 ) {
			fprintf(stderr, "\nError opening %s. ERRNO\"%d\"\n", cmd->tok[cmd->toklen - 1], errno);
			exit(EXIT_FAILURE);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Redirect I/O Streams
		////////////////////////////////////////////////////////////////////////////////
		if ( io ) {
			if (dup2(file_desc, STDIN_FILENO) < 0) {
				fprintf(stderr, "\nError redirecting STDIN to %s. ERRNO\"%d\"\n", cmd->tok[cmd->toklen - 1], errno);
				exit(EXIT_FAILURE);
			}
		}
		else {
			if (dup2(file_desc, STDOUT_FILENO) < 0) {
				fprintf(stderr, "\nError redirecting STDOUT to %s. ERRNO\"%d\"\n", cmd->tok[cmd->toklen - 1], errno);
				exit(EXIT_FAILURE);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// Execute Command - remove last two redirection arguments
		////////////////////////////////////////////////////////////////////////////////
		close(file_desc);
		cmd->tok[cmd->toklen - 1] = NULL;
		cmd->tok[cmd->toklen - 2] = NULL;
		cmd->toklen = cmd->toklen - 2;

		if ( execvpe(cmd->tok[0], cmd->tok, envp) < 0	&& errno == 2 ) {
			fprintf(stderr, "Command: \"%s\" not found.\n", cmd->tok[0]);
			exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", cmd->tok[0], errno);
			exit(EXIT_FAILURE);
		}
		signal(SIGINT, unmask_signal);
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
int exec_backg_command(command_t* cmd, char* envp[])
{
	pid_t p;
	int wait_status;
	int file_desc;

	////////////////////////////////////////////////////////////////////////////////
	// Handle and Initialize Signal Masking
	////////////////////////////////////////////////////////////////////////////////
	struct sigaction action;
	action.sa_sigaction = *job_handler;
	action.sa_flags = SA_SIGINFO | SA_RESTART;
	if ( sigaction(SIGCHLD, &action, NULL) < 0 )
		fprintf(stderr, "Error background signal handler: ERRNO\"%d\"\n", errno);
	sigprocmask(SIG_BLOCK, &sigmask_1, &sigmask_2);

	////////////////////////////////////////////////////////////////////////////////
	// Fork And Verify Process
	////////////////////////////////////////////////////////////////////////////////
	p = fork();
	if ( p < 0 ) {
		fprintf(stderr, "\nError forking background command. ERRNO\"%d\"\n", errno);
		exit(EXIT_FAILURE);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Parent
	////////////////////////////////////////////////////////////////////////////////
	if ( p != 0 ) {
		////////////////////////////////////////////////////////////////////////////////
		// Populate new job struct
		////////////////////////////////////////////////////////////////////////////////
		struct job create_job;
		create_job.cmdstr = (char*) malloc(MAX_COMMAND_TITLE);
		strcpy(create_job.cmdstr, cmd->tok[0]);
		create_job.status = false;
		create_job.pid = p;
		create_job.jid = num_jobs;

		////////////////////////////////////////////////////////////////////////////////
		// Augment Global all_jobs struct
		////////////////////////////////////////////////////////////////////////////////
		printf("[%d] %d running in background\n", num_jobs, p);
		all_jobs[num_jobs] = create_job;
		num_jobs++;

		////////////////////////////////////////////////////////////////////////////////
		// Unmask Signals, and Wait for completion
		////////////////////////////////////////////////////////////////////////////////
		sigprocmask(SIG_UNBLOCK, &sigmask_1, &sigmask_2);
		while (waitpid(p, &wait_status, WNOHANG) > 0) {}
		return EXIT_SUCCESS;

	}

	////////////////////////////////////////////////////////////////////////////////
	// Child
	////////////////////////////////////////////////////////////////////////////////
	else {
		////////////////////////////////////////////////////////////////////////////////
		// Map Child Process to different output
		////////////////////////////////////////////////////////////////////////////////
		char temp_file[MAX_COMMAND_LENGTH];
		char cpid [MAX_COMMAND_ARGLEN];
		int ipid = getpid();

		snprintf(cpid, MAX_COMMAND_ARGLEN,"%d",ipid);
		strcpy(temp_file, cpid);
		strcat(temp_file, "-temp_output.out");

		file_desc = open(temp_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if ( file_desc < 0 ) {
			fprintf(stderr, "\nError opening %s. ERRNO\"%d\"\n", temp_file, errno);
			exit(EXIT_FAILURE);
		}

		if ( dup2(file_desc, STDOUT_FILENO) < 0 ) {
			fprintf(stderr, "\nError redirecting STDOUT to %s. ERRNO\"%d\"\n", temp_file, errno);
			exit(EXIT_FAILURE);
		}

		if ( execvpe(cmd->tok[0], cmd->tok, envp) < 0	&& errno == 2 ) {
			fprintf(stderr, "Command: \"%s\" not found.\n", cmd->tok[0]);
			exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", cmd->tok[0], errno);
			exit(EXIT_FAILURE);
		}
		signal(SIGINT, unmask_signal);
		exit(EXIT_SUCCESS);
	}

}

/**
	* Executes any command with an | present
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int exec_pipe_command(command_t* cmd, char* envp[]) {
	////////////////////////////////////////////////////////////////////////////////
	// Mask Inturrept Signals
	////////////////////////////////////////////////////////////////////////////////
	signal(SIGINT, mask_signal);

	////////////////////////////////////////////////////////////////////////////////
	// Tokenize Piped Command into pieces
	////////////////////////////////////////////////////////////////////////////////
	int i = 0, j = 0, k = 0, num_cmds;
	command_t* cmds = malloc(MAX_COMMAND_ARGLEN * sizeof *cmds);
	cmds[0].tok = malloc( sizeof(char*) * MAX_COMMAND_ARGLEN );

	for ( ; i < cmd->toklen; i++ ) {
		if ( !strcmp(cmd->tok[i], "|") ) {
			//matches pipe
			cmds[j].tok[k] = NULL;
			cmds[j].toklen = k;
			j++;
			cmds[j].tok = malloc( sizeof(char*) * MAX_COMMAND_ARGLEN );
			k = 0;
		}
		else {
			cmds[j].tok[k] = malloc( sizeof(char) * MAX_COMMAND_TITLE );
			strcpy(cmds[j].tok[k], cmd->tok[i]);
			//debug printf("cmds[%d]->tok[%d] = %s\n", j, k, cmds[j].tok[k]);
			k++;
		}
	}
	cmds[j].tok[k] = NULL;
	cmds[j].toklen = k;
	num_cmds = j;

//debug
/*	for (i = 0; i <= num_cmds; i ++) {
		for (j = 0; j <= cmds[i].toklen; j++) {
			printf("cmds[%d].tok[%d] = %s\n", i, j, cmds[i].tok[j]);
		}
	}*/

	i = 0;
	j = 0;
	int file_desc[2];

	////////////////////////////////////////////////////////////////////////////////
	// Create and link pipes
	////////////////////////////////////////////////////////////////////////////////
	for ( i = 0; i < num_cmds - 1; ++i ) {
		if ( pipe(file_desc) < 0 ) {
			fprintf(stderr, "\nError in pipe creation. ERRNO:%d\n", errno);
			return EXIT_FAILURE;
		}
		iterative_fork_helper(&cmds[i], j, file_desc[1], envp);
		close(file_desc [1]);
		j = file_desc[0];
	}

	if ( j != 0 )
		dup2(j, STDIN_FILENO);

	////////////////////////////////////////////////////////////////////////////////
	// Execute final command
	////////////////////////////////////////////////////////////////////////////////
	if ( execvpe(cmds[i].tok[0], cmds[i].tok, envp) < 0 ) {
		fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", cmd->tok[0], errno);
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, unmask_signal);
	return EXIT_SUCCESS;
}

/**************************************************************************
 * MAIN
 **************************************************************************/

/**
	* Quash entry point
	*
	* @param argc argument count from the command line
	* @param argv argument vector from the command line
	* @param envp environment variables
	* @return program exit status
	*/
int main(int argc, char** argv, char** envp) {

	////////////////////////////////////////////////////////////////////////////////
	// Init Signal Masking
	////////////////////////////////////////////////////////////////////////////////
	sigemptyset(&sigmask_1);
	sigaddset(&sigmask_1, SIGCHLD);

	////////////////////////////////////////////////////////////////////////////////
	// Input stems from FILE - Redirects command interpretation structure
	////////////////////////////////////////////////////////////////////////////////
	if ( !isatty( (fileno(stdin) ) ) ) {
		exec_from_file(argv, argc, envp);
		return EXIT_SUCCESS;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Args
	////////////////////////////////////////////////////////////////////////////////
	command_t cmd;										//< Command holder argument

	start();
	puts("Welcome to Quash!\nType \"exit\" or \"quit\" to leave this shell");
	print_init();

	////////////////////////////////////////////////////////////////////////////////
	// Main Execution Loop
	////////////////////////////////////////////////////////////////////////////////
	while ( is_running() && get_command(&cmd, stdin) ) {
		run_quash(&cmd, envp);
	}

	return EXIT_SUCCESS;
}
