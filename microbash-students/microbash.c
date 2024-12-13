//#error Please read the accompanying microbash.pdf before hacking this source code (and removing this line).
/*
 * Micro-bash v2.2
 *
 * Programma sviluppato a supporto del laboratorio di Sistemi di
 * Elaborazione e Trasmissione dell'Informazione del corso di laurea
 * in Informatica presso l'Università degli Studi di Genova, a.a. 2024/2025.
 *
 * Copyright (C) 2020-2024 by Giovanni Lagorio <giovanni.lagorio@unige.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifndef NO_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <stdint.h>

void fatal(const char * const msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

void fatal_errno(const char * const msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void *my_malloc(size_t size)
{
	void *rv = malloc(size);
	if (!rv)
		fatal_errno("my_malloc");
	return rv;
}

void *my_realloc(void *ptr, size_t size)
{
	void *rv = realloc(ptr, size);
	if (!rv)
		fatal_errno("my_realloc");
	return rv;
}

char *my_strdup(char *ptr)
{
	char *rv = strdup(ptr);
	if (!rv)
		fatal_errno("my_strdup");
	return rv;
}

#define malloc I_really_should_not_be_using_a_bare_malloc
#define realloc I_really_should_not_be_using_a_bare_realloc
#define strdup I_really_should_not_be_using_a_bare_strdup

static const int NO_REDIR = -1;

typedef enum { CHECK_OK = 0, CHECK_FAILED = -1 } check_t;

static const char *const CD = "cd";

typedef struct {
	int n_args;
	char **args; // in an execv*-compatible format; i.e., args[n_args]=0
	char *out_pathname; // 0 if no output-redirection is present
	char *in_pathname; // 0 if no input-redirection is present
} command_t;

typedef struct {
	int n_commands;
	command_t **commands;
} line_t;

void free_command(command_t * const c)
{
	assert(c==0 || c->n_args==0 || (c->n_args > 0 && c->args[c->n_args] == 0)); /* sanity-check: if c is not null, then it is either empty (in case of parsing error) or its args are properly NULL-terminated */
	/*** TO BE DONE START ***/
	//freeing the parsed command string
	for(size_t i = 0;i < c->n_args;++i)
	{
		free(c->args[i]);
	}
	//freeing the pathnames
	free(c->out_pathname);
	free(c->in_pathname);
	//freeing the argument array and the command
	free(c->args);
	free(c);
	/*** TO BE DONE END ***/
}

void free_line(line_t * const l)
{
	assert(l==0 || l->n_commands>=0); /* sanity-check */
	/*** TO BE DONE START ***/
	//freeing all commands in the array
	for(size_t i = 0;i < l->n_commands;++i)
	{
		//getting the command pointer
		command_t* command = l->commands[i];
		if(command != NULL)
		{
			free_command(command);
		}
	}
	//freeing the array and the line
	free(l->commands);
	free(l);
	/*** TO BE DONE END ***/
}

#ifdef DEBUG
void print_command(const command_t * const c)
{
	if (!c) {
		printf("Command == NULL\n");
		return;
	}
	printf("[ ");
	for(int a=0; a<c->n_args; ++a)
		printf("%s ", c->args[a]);
	assert(c->args[c->n_args] == 0);
	printf("] ");
	printf("in: %s out: %s\n", c->in_pathname, c->out_pathname);
}

void print_line(const line_t * const l)
{
	if (!l) {
		printf("Line == NULL\n");
		return;
	}
	printf("Line has %d command(s):\n", l->n_commands);
	for(int a=0; a<l->n_commands; ++a)
		print_command(l->commands[a]);
}
#endif

command_t *parse_cmd(char * const cmdstr)
{
	static const char *const BLANKS = " \t";
	command_t * const result = my_malloc(sizeof(*result));
	memset(result, 0, sizeof(*result));
	char *saveptr, *tmp;
	tmp = strtok_r(cmdstr, BLANKS, &saveptr);
	while (tmp) {
		result->args = my_realloc(result->args, (result->n_args + 2)*sizeof(char *));
		if (*tmp=='<') {
			if (result->in_pathname) {
				fprintf(stderr, "Parsing error: cannot have more than one input redirection\n");
				goto fail;
			}
			if (!tmp[1]) {
				fprintf(stderr, "Parsing error: no path specified for input redirection\n");
				goto fail;
			}
			result->in_pathname = my_strdup(tmp+1);
		} else if (*tmp == '>') {
			if (result->out_pathname) {
				fprintf(stderr, "Parsing error: cannot have more than one output redirection\n");
				goto fail;
			}
			if (!tmp[1]) {
				fprintf(stderr, "Parsing error: no path specified for output redirection\n");
				goto fail;
			}
			result->out_pathname = my_strdup(tmp+1);
		} else {
			if (*tmp=='$') {
				/* Make tmp point to the value of the corresponding environment variable, if any, or the empty string otherwise */
				/*** TO BE DONE START ***/
				//c string always has terminator
				char* temp = &tmp[1];
				if((temp = getenv(temp)) == NULL)
				{
					fprintf(stderr,"failed to get enviroment variable %s\n",&tmp[1]);
					tmp[0] = ' ';
					tmp[1] = '\0';
				}else
				{
					tmp = temp;
				}
			
				/*** TO BE DONE END ***/
			}
			result->args[result->n_args++] = my_strdup(tmp);
			result->args[result->n_args] = 0;
		}
		tmp = strtok_r(0, BLANKS, &saveptr);
	}
	if (result->n_args)
		return result;
	fprintf(stderr, "Parsing error: empty command\n");
fail:
	free_command(result);
	return 0;
}

line_t *parse_line(char * const line)
{
	static const char * const PIPE = "|";
	char *cmd, *saveptr;
	cmd = strtok_r(line, PIPE, &saveptr);
	if (!cmd)
		return 0;
	line_t *result = my_malloc(sizeof(*result));
	memset(result, 0, sizeof(*result));
	while (cmd) {
		command_t * const c = parse_cmd(cmd);
		if (!c) {
			free_line(result);
			return 0;
		}
		result->commands = my_realloc(result->commands, (result->n_commands + 1)*sizeof(command_t *));
		result->commands[result->n_commands++] = c;
		cmd = strtok_r(0, PIPE, &saveptr);
	}
	return result;
}

check_t check_redirections(const line_t * const l)
{
	assert(l);
	/* This function must check that:
	 * - Only the first command of a line can have input-redirection
	 * - Only the last command of a line can have output-redirection
	 * and return CHECK_OK if everything is ok, print a proper error
	 * message and return CHECK_FAILED otherwise
	 */
	/*** TO BE DONE START ***/
	for(size_t i = 1;i < l->n_commands;++i)
	{
		command_t* ref_comm = l->commands[i];
		if(ref_comm == NULL)
		{
			continue;
			//invalid command found
			fatal("one command was a null reference");
		}
		if(ref_comm->in_pathname)
		{
			fprintf(stderr,"cannot redirect input of anything but the first command\n");
			return CHECK_FAILED;
		}	
	}
	for(size_t i = 0;i < l->n_commands -1;++i)
	{
		command_t* ref_comm = l->commands[i];
		if(ref_comm == NULL)
		{
			continue;
			//invalid command found
			fatal("one command was a null reference");
		}
		if(ref_comm->out_pathname)
		{
			fprintf(stderr,"cannot redirect output of anything but the last command\n");
			return CHECK_FAILED;
		}	
	}

	/*** TO BE DONE END ***/
	return CHECK_OK;
}

check_t check_cd(const line_t * const l)
{
	assert(l);
	/* This function must check that if command "cd" is present in l, then such a command
	 * 1) must be the only command of the line
	 * 2) cannot have I/O redirections
	 * 3) must have only one argument
	 * and return CHECK_OK if everything is ok, print a proper error
	 * message and return CHECK_FAILED otherwise
	 */
	/*** TO BE DONE START ***/
	//look for cds
	for(size_t i = 0;i < l->n_commands;++i)
	{
		command_t* ref_comm = l->commands[i];
		if(ref_comm == NULL)
		{
			continue;
			//invalid command found
			fatal("one command was a null reference");
		}
		//check wheter the command is cd
		if(ref_comm->n_args > 0 && strcmp(ref_comm->args[0],CD) == 0)
		{
			//check if there are multiple commands
			if(l->n_commands > 1)
			{
				fprintf(stderr,"error cd can be the only one command in the line\n");
				return CHECK_FAILED;
			}
			if(ref_comm->n_args != 2 )
			{
				fprintf(stderr,"error the cd command must have only one argument\n");
				return CHECK_FAILED;
			}
			//check for redirections
			if(ref_comm->in_pathname || ref_comm->out_pathname)
			{
				fprintf(stderr,"error cd cannot have redirections\n");
				return CHECK_FAILED;
			}
		}	
	
	}
	/*** TO BE DONE END ***/
	return CHECK_OK;
}

void wait_for_children()
{
	/* This function must wait for the termination of all child processes.
	 * If a child exits with an exit-status!=0, then you should print a proper message containing its PID and exit-status.
	 * Similarly, if a child is killed by a signal, then you should print a message specifying its PID, signal number and signal name.
	 */
	/*** TO BE DONE START ***/
	
	int wstatus;
	int ret;
	while((ret = wait(&wstatus)) != ECHILD)
	{
		if(errno == ECHILD)
		{
			break;
		}
		//check if the child process exited incorrectly
		if(!WIFEXITED(wstatus))
		{
			//check if the child process was terminated by a signal
			if(WIFSIGNALED(wstatus))
			{
				//get the signal
				int signal_num = WTERMSIG(wstatus); 
				fprintf(stderr,"process pid=%d was terminated by signal with number=%d called %s\n",
						ret,
						signal_num,
						sigabbrev_np(signal_num));
			}
		}else
		{
			//exited so we can get the exit status
			int exit_val = WEXITSTATUS(wstatus);
			if(exit_val)
			{
				//ret is the pid of the child process since it terminated correctly with exit
				fprintf(stdout,"process with pid=%d exited with error code %d\n",ret,exit_val);
			}
		}
	}
	/*** TO BE DONE END ***/
}

void redirect(int from_fd, int to_fd)
{
	/* If from_fd!=NO_REDIR, then the corresponding open file should be "moved" to to_fd.
	 * That is, use dup/dup2/close to make to_fd equivalent to the original from_fd, and then close from_fd
	 */
	/*** TO BE DONE START ***/
	
	//if this is called by a function that creates a child process the child will inherith its file descriptors that will point to the same
	//file description in their open file table but 

	if(from_fd != NO_REDIR)
	{
		//locking the the open file to redirect to by creating a new fd
		int tempfd = dup(to_fd);
		if(tempfd == -1 && errno != EBADF)
		{
			fatal_errno("cannot duplicate new file descriptor to save it");
		}
	
		//changing new file descriptor to old one while there is another file descriptor on this process
		int dup_err = dup2(from_fd,to_fd);
		if(dup_err == -1)
		{
			fatal_errno("cannot change file descriptor while redirecting");
		}
		

		if(tempfd != -1)
		{
			int close_err = close(tempfd);
			if(close_err == -1)
			{
				fatal_errno("error while redirecting new file descriptor to old one");
			}
		}

		int from_err = close(from_fd);
		if(from_err == -1)
		{
			fatal_errno("cannot close old file descriptor");
		}
	}

	/*** TO BE DONE END ***/
}

void run_child(const command_t * const c, int c_stdin, int c_stdout)
{
	/* This function must:
	 * 1) create a child process, then, in the child
	 * 2) redirect c_stdin to STDIN_FILENO (=0)
	 * 3) redirect c_stdout to STDOUT_FILENO (=1)
	 * 4) execute the command specified in c->args[0] with the corresponding arguments c->args
	 * (printing error messages in case of failure, obviously)
	 */
	/*** TO BE DONE START ***/
	//creating child process 
	int pid = fork();
	if(pid == -1)
	{
		fatal_errno("could not create child process");
	}else if(!pid)
	{
		//child process
		//redirections need to be done here to prevent altering file descriptors of the main process
		redirect(c_stdin,0);
		redirect(c_stdout,1);
		//on error this function can return
		execvpe(c->args[0],c->args,environ);
		fprintf(stderr,"error running command %s",c->args[0]);
		fatal_errno("=");
	}
	//parent process
	/*** TO BE DONE END ***/
}

void change_current_directory(char *newdir)
{
	/* Change the current working directory to newdir
	 * (printing an appropriate error message if the syscall fails)
	 */
	/*** TO BE DONE START ***/
	
	if(chdir(newdir))
	{
		switch(errno)
		{
		case EACCES:
			fprintf(stderr,"permission denied\n");
			break;
		case EFAULT:
			fprintf(stderr,"the chosen path cannot be accesed\n");
			break;
		case EIO:
			fatal("I/O error while setting directory");
			break;
		case ELOOP:
			fprintf(stderr,"could not resolve path due to loop in simbolyc links ELOOP\n");
			break;
		case ENAMETOOLONG:
		     	fprintf(stderr,"path name too long\n");
			break;
		case ENOENT:
		     	fprintf(stderr,"path not found\n");
			break;
		case ENOMEM:
			fatal("not enough memeory to change working directory: ENOMEM");
			break;
		case ENOTDIR:
			fprintf(stderr,"error the given path is not a directory: ENOTDIR\n");
			break;
		default:
			fatal_errno("unrecognized error");
		}
	}
	/*** TO BE DONE END ***/
}

void close_if_needed(int fd)
{
	if (fd==NO_REDIR)
		return; // nothing to do
	if (close(fd))
		perror("close in close_if_needed");
}

void execute_line(const line_t * const l)
{
	if (strcmp(CD, l->commands[0]->args[0])==0) {
		assert(l->n_commands == 1 && l->commands[0]->n_args == 2);
		change_current_directory(l->commands[0]->args[1]);
		return;
	}
	int next_stdin = NO_REDIR;
	for(int a=0; a<l->n_commands; ++a) {
		int curr_stdin = next_stdin, curr_stdout = NO_REDIR;
		const command_t * const c = l->commands[a];
		if (c->in_pathname) {
			assert(a == 0);
			/* Open c->in_pathname and assign the file-descriptor to curr_stdin
			 * (handling error cases) */
			/*** TO BE DONE START ***/
			curr_stdin = open(c->in_pathname, O_CLOEXEC | O_RDONLY);
			if(curr_stdin == -1)
			{
				perror("error redirecting input: ");
				switch(errno)
				{	
					case EACCES:
						break;
					case EINVAL:
						break;
					case EISDIR:
						break;
					case ELOOP:
						break;
					case ENAMETOOLONG:
						break;
					case ENOENT:
						break;
					case ENOTDIR:
						break;
					case EOVERFLOW:
						break;
					case EPERM:
						break;
					case ETXTBSY:
						break;
					default:
						fatal_errno("fatal error while opening input redirection");
				}
				//does not execute the command line
				return;
			}
			/*** TO BE DONE END ***/
		}
		if (c->out_pathname) {
			assert(a == (l->n_commands-1));
			/* Open c->out_pathname and assign the file-descriptor to curr_stdout
			 * (handling error cases) */
			/*** TO BE DONE START ***/
			mode_t mode = S_IWUSR;
			curr_stdout = open(c->out_pathname,(O_CLOEXEC | O_CREAT | O_WRONLY | O_TRUNC),mode);
			if(curr_stdout == -1)
			{
				perror("error redirecting output");
				switch(errno)
				{
					case EEXIST:
						break;
					case EACCES:
						break;
					case EINVAL:
						break;
					case EISDIR:
						break;
					case ELOOP:
						break;
					case ENAMETOOLONG:
						break;
					case ENOENT:
						break;
					case ENOTDIR:
						break;
					case EOVERFLOW:
						break;
					case EPERM:
						break;
					case ETXTBSY:
						break;
					default:
						fatal_errno("fatal error while opening output redirection");
				}
				//redirecting to /dev/zero
				curr_stdout = open("/dev/null",O_CLOEXEC | O_NOCTTY | O_WRONLY | O_NOFOLLOW);
				if(curr_stdout == -1)
				{
					fatal_errno("/dev/null could not be opened something is very wrong");
				}
			}
			/*** TO BE DONE END ***/
		} else if (a != (l->n_commands - 1)) { /* unless we're processing the last command, we need to connect the current command and the next one with a pipe */
			int fds[2];
			/* Create a pipe in fds, and set FD_CLOEXEC in both file-descriptor flags */
			/*** TO BE DONE START ***/
			if(pipe2(fds,O_CLOEXEC))
			{
				fatal_errno("failed to create a pipe");
			}
			/*** TO BE DONE END ***/
			curr_stdout = fds[1];
			next_stdin = fds[0];
		}
		run_child(c, curr_stdin, curr_stdout);
		close_if_needed(curr_stdin);
		close_if_needed(curr_stdout);
	}
	wait_for_children();
}

void execute(char * const line)
{
	line_t * const l = parse_line(line);
#ifdef DEBUG
	print_line(l);
#endif
	if (l) {
		if (check_redirections(l)==CHECK_OK && check_cd(l)==CHECK_OK)
			execute_line(l);
		free_line(l);
	}
}

int main()
{
	const char * const prompt_suffix = " $ ";
	const size_t prompt_suffix_len = strlen(prompt_suffix);
	for(;;) {
		char *pwd;
		/* Make pwd point to a string containing the current working directory.
		 * The memory area must be allocated (directly or indirectly) via malloc.
		 */
		/*** TO BE DONE START ***/
		//pwd is freed at the end
		pwd = getcwd(NULL,0);
		if(pwd == NULL)
		{
			fatal_errno("cannot get current directory");
		}
		/*** TO BE DONE END ***/
		pwd = my_realloc(pwd, strlen(pwd) + prompt_suffix_len + 1);
		strcat(pwd, prompt_suffix);
#ifdef NO_READLINE
		const int max_line_size = 512;
		char * line = my_malloc(max_line_size);
		printf("%s", pwd);
		if (!fgets(line, max_line_size, stdin)) {
			free(line);
			line = 0;
			putchar('\n');
		} else {
			size_t l = strlen(line);
			if (l && line[--l]=='\n')
				line[l] = 0;
		}
#else
		char * const line = readline(pwd);
#endif
		free(pwd);
		if (!line) break;
		execute(line);
		free(line);
	}
}

