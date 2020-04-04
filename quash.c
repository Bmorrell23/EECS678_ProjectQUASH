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

static struct m_job all_jobs[50];

static int num_jobs = 0;


/**
 	* @brief Quash begin execution
	*/
static void start_quash()
{
	m_running = true;
}

/**
	* @brief Flag file commands
	*/
static void start_from_file()
{
	m_file_run = true;
}


/**
	* @brief determine if Quash gets more commands
	*
	* @return T if program is receiving commands, F otherwise
	*/
bool m_get_command()
{
	return m_running || m_file_run;
}

/**
	* @brief suspends execution loop
	*/
void suspend()
{
	m_running = false;
}

/**
	* @brief suspends quash file execution
	*/
void suspend_file_exec()
{
	m_file_run = false;
}

/**
	* @brief sig mask
	*	example Signals Lab05
	*
	* @param signal int
 */
void sig_mask(int sig)
{
	printf("\n");
}

/**
	* @brief sig unmask
	* example Signals Lab05
	*
	* @param signal int
 */
void sig_unmask(int sig)
{
	exit(0);
}


/**
	* @brief Print current directory before shell inputs
	*/
void curr_dir_print()
{
	// - current working directory command --

	char cwd[1024];
	if ( getcwd(cwd, sizeof(cwd)) && !m_file_run )
		printf("\n[Quash: %s] q$ ", cwd);
}

/**
	* @brief background job handler
	*
	* @param signal int
	* @param sig struct
	* @param pos
	*/
void m_handle_job(int signal, siginfo_t* sig, void* pos)
{
	pid_t p = sig->si_pid;
	int b;
	for ( b = 0; b < num_jobs; b++)
	{
		if ( all_jobs[b].pid == p )
			goto finish;
	}

	finish:
	if ( b < num_jobs )
	{
		printf("\n[%d] %d finished!! %s\n", all_jobs[b].jid, p, all_jobs[b].j_comm_str);
		all_jobs[b].running = true;
		free(all_jobs[b].j_comm_str);
	}
}

/**
	* @brief create fork for file redirection
	*
	* @param qcommd command struct
	* @param fdi file descriptor in
	* @param fdo file descriptor out
	* @param envp environment variables
	* @return exit status
	*/
int m_fork_assist (m_command* qcommd, int fdi, int fdo, char* envp[])
{
	pid_t p;

	if ( !(p = fork ()) )
	{

		// -- file out implementation --

		if ( fdo != 1 )
		{
			if ( dup2(fdo, STDOUT_FILENO) < 0 )
			{
				fprintf(stderr, "\nSTDOUT file direction error. ERRNO\"%d\"\n", errno);
				exit(1);
			}
			// -- close file output --
			close (fdo);
		}

		// -- file in implementation --

		if ( fdi != 0 ) {
			if ( dup2(fdi, STDIN_FILENO ) < 0) {
				fprintf(stderr, "\nstdin file direction error. ERRNO\"%d\"\n", errno);
				exit(1);
			}
			// -- close file input --
			close (fdi);
		}

		// --  qcommd execvpe() --

		if ( execvpe(qcommd->m_c_tok[0], qcommd->m_c_tok, envp) < 0	&& errno == 2 )
		{
			fprintf(stderr, "Command: \"%s\" cannot be found.\n", qcommd->m_c_tok[0]);
			exit(1);
		}
		else {
			fprintf(stderr, "Command error %s. ERRNO\"%d\"\n", qcommd->m_c_tok[0], errno);
			exit(1);
		}
		return 0;
	}
	return p;
}

/**
	* @brief Parse Raw Command string
	*
	* @param qcommd command struct
	* @param in instream
	* @return bool successful parse
	*/
bool m_cmd_parse(m_command* qcommd, FILE* in)
{
	if ( fgets(qcommd->q_comm_str, 1024, in) != NULL )
	{
		size_t cmd_size = strlen(qcommd->q_comm_str);
		char end_char = qcommd->q_comm_str[cmd_size - 1];

		// -- file parsing flags --

		if ( end_char == '\r' || end_char == '\n' )
		{

			qcommd->q_comm_str[cmd_size - 1] = '\0';
			qcommd->comm_siz = cmd_size - 1;
		}
		else
			qcommd->comm_siz = cmd_size;

		// -- command size == 0 --

		if ( !(int)qcommd->comm_siz )
			return true;

		//------------------------------------------------------------------------------
		// Tokenize command arguments

		char* token = malloc( sizeof(char*) * 32 );
		qcommd->m_c_tok = malloc( sizeof(char*) * 32 );

		// -- reset size --
		qcommd->symb_size = 0;

		token = strtok (qcommd->q_comm_str," ");
		while ( token != NULL )
		{

			qcommd->m_c_tok[qcommd->symb_size] = token;
			qcommd->symb_size++;
			token = strtok(NULL, " ");
		}

		free(token);

		// -- remove
		qcommd->m_c_tok[qcommd->symb_size] = '\0';
		return true;
	}
	else
		return false;
}

/**
	* @brief substring command parser
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

/**
	* @brief command cd
	*
	* @param qcommd command struct
	*
	*/
void cd(m_command* qcommd)
{
	if ( qcommd->symb_size > 2 )
		puts("Only specify 1 argument");
	else if ( qcommd->symb_size < 2 )
		{
			if ( chdir(getenv("HOME")) )
				printf("cd: %s: ERROR - There was a problem going to $HOME\n", getenv("HOME"));
		}
	else {
		if ( chdir(qcommd->m_c_tok[1]) )
			printf("cd: %s: No such file or directory\n", qcommd->m_c_tok[1]);
	}
}

/**
	* @brief command echo
	*
	* @param qcommd command struct
	*
	*/
void echo(m_command* qcommd)
{
	if ( qcommd->symb_size == 2 ) {
		if ( !strcmp(qcommd->m_c_tok[1], "$HOME") )
			puts(getenv("HOME"));
		else if ( !strcmp(qcommd->m_c_tok[1], "$PATH") )
			puts(getenv("PATH"));
		else if ( strstr(qcommd->m_c_tok[1], "$") != NULL )
			puts(getenv(substring(qcommd->m_c_tok[1],1,strlen(qcommd->m_c_tok[1]))));
		else
			puts(qcommd->m_c_tok[1]);
	}
	else if ( qcommd->symb_size == 1 ) {
		puts(getenv("HOME"));
	}
	else{
		int i = 1;
		for ( ; i < qcommd->symb_size; i++ )
			printf("%s ", qcommd->m_c_tok[i]);
		puts("");
	}
}

/**
	* @brief command jobs
	*
	* @param qcommd command struct
	*/
void jobs(m_command* qcommd) {
	int i;

	for ( i = 0; i < num_jobs; i++ ) {
		if ( !all_jobs[i].running && kill(all_jobs[i].pid, 0) == 0 )
		{
			printf("[%d] %d %s \n", all_jobs[i].jid, all_jobs[i].pid, all_jobs[i].j_comm_str);
		}
	}
}

/**
	* @brief command set
	*
	* Assigns the specified environment variable (HOME or PATH)
	*
	* @param qcommd command struct
 */
void set(m_command* qcommd)
{
	if ( qcommd->m_c_tok[1] == NULL )
		printf("set error: No command given\n");
	else
	{
		//  --Get environment variable and directory --

		char* env_var = strtok(qcommd->m_c_tok[1], "=");
		char* directory_var = strtok(NULL, "=");

		if ( env_var == NULL || directory_var == NULL ) {
			puts("Command set error TRY:\n");
			puts("\tset HOME=/directory/to/use/for/home\n");
			puts("\tset PATH=/directory/to/use/for/path\n");

		}
		// -- Set the environment variables --
		else
			setenv(env_var, directory_var, 1);
	}
}

/**
	* @brief file command function
	*
	* @param argc argument count from the command line
	* @param argv argument vector from the command line
	* @param envp environment variables
	*/
void m_command_file(char** argv, int argc, char* envp[]) {


	m_command qcommd;


	// -- Redirect Quash Standard Input --

	start_from_file();


	// -- file input --

	while (m_cmd_parse(&qcommd, stdin)) {
		quash_run(&qcommd, envp);
	}


	// -- suspend file execution --

	suspend_file_exec();
}


/**
	* @brief run quash command
	*
	* @param qcommd command struct
	* @param envp environment variables
	*/
void quash_run(m_command* qcommd, char** envp) {

	if ( !strcmp(qcommd->q_comm_str, "exit") || !strcmp(qcommd->q_comm_str, "quit") )
		suspend(); // Exit Quash

	else if ( !qcommd->comm_siz )
	{

	}
	else if ( strcmp(qcommd->m_c_tok[0], "cd") == 0 )
		cd(qcommd);
	else if ( strcmp(qcommd->m_c_tok[0], "echo") == 0 )
		echo(qcommd);
	else if ( !strcmp(qcommd->m_c_tok[0], "jobs") )
		jobs(qcommd);
	else if ( !strcmp(qcommd->m_c_tok[0], "set") )
		set(qcommd);
	else
		command_logic(qcommd, envp);

	if ( m_running )
		curr_dir_print();
}

/**
	* @brief command logic flags
	*
	* @param qcommd command struct
	* @param envp environment variables
	* @return exit status
	*/
int command_logic(m_command* qcommd, char* envp[])
{
	// -- flag for pipe "|" --
	bool p_flag = false;

	// -- flag for background proccess "&" --
	bool b_flag = false;

	// -- flag for command input redirection "<" --
	bool in_flag = false;

	// -- flag for output redirection ">" --
	bool out_flag = false;

	// -- Redirect command logic --

	int c = 1;
	for (; c < qcommd->symb_size; c++)
	{
		if ( !strcmp(qcommd->m_c_tok[c], "&") )
			b_flag = true;
		else if ( !strcmp(qcommd->m_c_tok[c], "|") )
			p_flag = true;
		else if ( !strcmp(qcommd->m_c_tok[c], "<") )
			in_flag = true;
		else if ( !strcmp(qcommd->m_c_tok[c], ">") )
			out_flag = true;
	}

	int q_exit = 0;

	if ( b_flag )
	{
		// -- reset and remove background process '&' symbol --

		qcommd->m_c_tok[qcommd->symb_size - 1] = '\0';

		qcommd->symb_size--;

		q_exit = m_cmd_background(qcommd, envp);
	}
	else if ( in_flag )
		q_exit = input_io_cmd(qcommd, true, envp);
	else if ( out_flag )
		q_exit = input_io_cmd(qcommd, false, envp);
	else if ( p_flag )
		q_exit = m_command_pipe(qcommd, envp);
	else
		q_exit = prim_cmd(qcommd, envp);
	return q_exit;
}

/**
	* @brief runs any command that can be handled with execvpe
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return exit status
 */
int prim_cmd(m_command* qcommd, char* envp[])
{
	// -- sig mask --

	pid_t ret;
	int m_wait;
	signal(SIGINT, sig_mask);

	// -- Fork and check --

	ret = fork();
	if ( ret < 0 )
	{
		fprintf(stderr, "Primary forking error. Error:%d\n", errno);
		exit(1);
	}

	// -- parent process --

	if ( ret != 0 )
	{
		if ( waitpid(ret, &m_wait, 0) < 0 )
		{
			signal(SIGINT, sig_unmask);
			fprintf(stderr, "Primary forking error child	%d. ERRNO\"%d\"\n", ret, errno);
			return 1;
		}
		if ( WIFEXITED(m_wait) && WEXITSTATUS(m_wait) == 1 )
			return 1;
		signal(SIGINT, sig_unmask);
		return(0);
	}

	// -- child process --

	else
	{
		if ( execvpe(qcommd->m_c_tok[0], qcommd->m_c_tok, envp) < 0	&& errno == 2 )
		{
			fprintf(stderr, "Command: \"%s\" cannot be located.\n", qcommd->m_c_tok[0]);
			exit(1);
		}
		else
		{
			fprintf(stderr, "Run error %s. ERRNO\"%d\"\n", qcommd->m_c_tok[0], errno);
			exit(1);
		}
		exit(0);
	}
}

/**
	* @brief run command with input/output
	*
	* @param qcommd command struct
	* @param io true is stdin, false is stdout
	* @param envp environment variables
	* @return exit status
	*/
int input_io_cmd(m_command* qcommd, bool i_o, char* envp[])
{

	// -- Sig masking interrupt --

	pid_t ret;
	int m_wait;
	int fds;
	signal(SIGINT, sig_mask);

	// -- Fork and check --

	ret = fork();
	if ( ret < 0 )
	{
		fprintf(stderr, "Forking error redir command. ERRNO\"%d\"\n", errno);
		exit(1);
	}

	// -- parent process --

	if ( ret != 0 )
	{
		if (waitpid(ret, &m_wait, 0) == -1)
		{
			fprintf(stderr, "Error with redir command's child	%d. ERRNO\"%d\"\n", ret, errno);
			return 1;
		}
		if ( WIFEXITED(m_wait) && WEXITSTATUS(m_wait) == 1 )
			return 1;

		signal(SIGINT, sig_unmask);
		return 0;
	}

	// -- child process --

	else
	{
		// -- assign and check file descriptor --

		if ( i_o )
			fds = open(qcommd->m_c_tok[qcommd->symb_size - 1], O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		else
			fds = open(qcommd->m_c_tok[qcommd->symb_size - 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if ( fds < 0 )
		{
			fprintf(stderr, "\nCould not open %s. ERRNO\"%d\"\n", qcommd->m_c_tok[qcommd->symb_size - 1], errno);
			exit(1);
		}

		// -- Allow input and output streams to be switched --

		if ( i_o )
		{
			if (dup2(fds, STDIN_FILENO) < 0)
			{
				fprintf(stderr, "\nCould not redirect STDIN to %s. ERRNO\"%d\"\n", qcommd->m_c_tok[qcommd->symb_size - 1], errno);
				exit(1);
			}
		}
		else
		{
			if (dup2(fds, STDOUT_FILENO) < 0)
			{
				fprintf(stderr, "\nCould not redirect STDOUT to %s. ERRNO\"%d\"\n", qcommd->m_c_tok[qcommd->symb_size - 1], errno);
				exit(1);
			}
		}


		// -- run command and discard last two redirection args --

		close(fds);
		qcommd->m_c_tok[qcommd->symb_size - 1] = NULL;
		qcommd->m_c_tok[qcommd->symb_size - 2] = NULL;
		qcommd->symb_size = qcommd->symb_size - 2;

		if ( execvpe(qcommd->m_c_tok[0], qcommd->m_c_tok, envp) < 0	&& errno == 2 ) {
			fprintf(stderr, "Command: \"%s\" not found.\n", qcommd->m_c_tok[0]);
			exit(1);
		}
		else {
			fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", qcommd->m_c_tok[0], errno);
			exit(1);
		}
		signal(SIGINT, sig_unmask);
		exit(0);
	}
}

/**
	* @brief run background command with an & present
	*
	* @param qcommd command struct
	* @param envp environment variables
	* @return exit status
	*/
int m_cmd_background(m_command* qcommd, char* envp[])
{
	pid_t ret;
	int m_wait;
	int fds;

	// -- create new background sig masking struct --

	struct sigaction action;
	action.sa_sigaction = *m_handle_job;
	action.sa_flags = SA_SIGINFO | SA_RESTART ;
	if ( sigaction(SIGCHLD, &action, NULL) < 0 )
		fprintf(stderr, "SIG background handler error: ERRNO\"%d\"\n", errno);
	sigprocmask(SIG_BLOCK, &sigmask_1, &sigmask_2);

	// -- create fork and check --

	ret = fork();
	if ( ret < 0 )
	{
		fprintf(stderr, "\nEncounter error forking background command. ERRNO\"%d\"\n", errno);
		exit(1);
	}

	// -- parent process --

	if ( ret != 0 )
	{

		// -- require new job process struct for parent process --

		struct m_job create_job;
		create_job.j_comm_str = (char*) malloc(128);
		strcpy(create_job.j_comm_str, qcommd->m_c_tok[0]);
		create_job.running = false;
		create_job.pid = ret;
		create_job.jid = num_jobs;


		// -- Background job scruct --

		printf("[%d] %d running in background\n", num_jobs, ret);
		all_jobs[num_jobs] = create_job;
		num_jobs++;


		// -- Signal unmasking, wait for finish --

		sigprocmask(SIG_UNBLOCK, &sigmask_1, &sigmask_2);
		while (waitpid(ret, &m_wait, WNOHANG) > 0) {}
		return 0;

	}

	// -- child process after fork --

	else {
		////////////////////////////////////////////////////////////////////////////////
		// Map Child Process to different output

		char temp_file[1024];
		char cpid [32];
		int child_pid = getpid();

		snprintf(cpid, 32,"%d",child_pid);
		strcpy(temp_file, cpid);
		strcat(temp_file, "-out_tmp.txt");

		// -- per the Linux open(3) man page --

		fds = open(temp_file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if ( fds < 0 ) {
			fprintf(stderr, "\nCould not open %s. ERRNO\"%d\"\n", temp_file, errno);
			exit(1);
		}

		if ( dup2(fds, STDOUT_FILENO) < 0 ) {
			fprintf(stderr, "\nCould not redirect STDOUT to %s. ERRNO\"%d\"\n", temp_file, errno);
			exit(1);
		}

		if ( (execvpe(qcommd->m_c_tok[0], qcommd->m_c_tok, envp) < 0)	&& (errno == 2) )
		{
			fprintf(stderr, "Command: \"%s\" not found.\n", qcommd->m_c_tok[0]);
			exit(1);
		}
		else {
			fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", qcommd->m_c_tok[0], errno);
			exit(1);
		}
		signal(SIGINT, sig_unmask);
		exit(0);
	}

}

/**
	* @brief runs any command with a "|" present
	*
	* @param qcommd command struct
	* @param envp environment variables
	* @return exit status
	*/
int m_command_pipe(m_command* qcommd, char* envp[])
{

	// -- SIGINT signal mask --

	signal(SIGINT, sig_mask);


	// -- Distribute tokens into command array --

	int i = 0;
	int j = 0;
	int k = 0;
	int cmd_numbers;

	m_command* cmds = malloc(32 * (sizeof *cmds) );

	cmds[0].m_c_tok = malloc( sizeof(char*) * 32 );

	for ( ; i < qcommd->symb_size; i++ ) {
		if ( !strcmp(qcommd->m_c_tok[i], "|") )
		{
			//matches pipe
			cmds[j].m_c_tok[k] = NULL;
			cmds[j].symb_size = k;
			j++;
			cmds[j].m_c_tok = malloc( sizeof(char*) * 32 );
			k = 0;
		}
		else
		{
			cmds[j].m_c_tok[k] = malloc( sizeof(char) * 128 );
			strcpy(cmds[j].m_c_tok[k], qcommd->m_c_tok[i]);

			k++;
		}
	}
	cmds[j].m_c_tok[k] = NULL;
	cmds[j].symb_size = k;
	cmd_numbers = j;

	i = 0;
	j = 0;
	int fds[2];

	// -- make new pipe and sync --

	for ( i = 0; i < cmd_numbers - 1; ++i )
	{
		if ( pipe(fds) < 0 ) {
			fprintf(stderr, "\nPipe initalize errors. ERRNO:%d\n", errno);
			return 1;
		}
		m_fork_assist(&cmds[i], j, fds[1], envp);
		close(fds [1]);
		j = fds[0];
	}

	if ( j != 0 )
		dup2(j, STDIN_FILENO);


	if ( execvpe(cmds[i].m_c_tok[0], cmds[i].m_c_tok, envp) < 0 )
	{
		fprintf(stderr, "Error completing final command: %s. ERRNO\"%d\"\n", qcommd->m_c_tok[0], errno);
		exit(1);
	}
	signal(SIGINT, sig_unmask);
	return 0;
}

/**
	* @brief Enter Quash program
	*
	* @param argc argument count from the command line
	* @param argv argument vector from the command line
	* @param envp environment variables
	*
	* @return program exit status
	*/
int main(int argc, char** argv, char** envp) {


	// -- Signal Masking SIGINT --

	sigemptyset(&sigmask_1);
	sigaddset(&sigmask_1, SIGCHLD);

	// -- create command object --

	m_command qcommd;

	// -- Allow for commmand redirection if input comes from a file --

	if ( !isatty((fileno(stdin))) )
	{
		m_command_file(argv, argc, envp);
		return 0;
	}

	// -- Start Quash function --
	start_quash();

	puts("Starting Quash ... Hello!\nType \"quit\" or \"exit\" to leave this shell");

	curr_dir_print();

	while ( m_cmd_parse(&qcommd, stdin) && m_get_command() )
	{
		quash_run(&qcommd, envp);
	}

	return 0;
}
