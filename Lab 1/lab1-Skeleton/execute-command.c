// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
//additional #include
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

void executingSimple(command_t c);
void executingSubshell(command_t c);
void executingAnd(command_t c);
void executingOr(command_t c);
void executingSequence(command_t c);
void executingPipe(command_t c);

void execute_switch(command_t c)
{
	switch(c->type)
	{
	case SIMPLE_COMMAND:
		executingSimple(c);
		break;
	case SUBSHELL_COMMAND:
		executingSubshell(c);
		break;
	case AND_COMMAND:
		executingAnd(c);
		break;
	case OR_COMMAND:
		executingOr(c);
		break;
	case SEQUENCE_COMMAND:
		executingSequence(c);
		break;
	case PIPE_COMMAND:
		executingPipe(c);
		break;
	default:
		error(1, 0, "Not a valid command");
	}
}

void executingSimple(command_t c)
{
    pid_t p = fork();
    int status;
    int exit_status;
    
    if(p < 0)
        error(1, errno, "fork was unsuccessful");
    
    if(p == 0)
    {
        int inputRedir;
        if(c->input != NULL)
        {
            inputRedir = open(c->input, O_RDONLY);
            if(inputRedir < 0)
            {
                fprintf(stderr,"File %s is an invalid input!\n",c->input);
                _exit(1);
            }

            if(dup2(inputRedir,0) < 0)
                _exit(1);
        }
    
        int outputRedir;
        if(c->output != NULL)
        {
            outputRedir = open(c->output, O_WRONLY|O_CREAT|O_TRUNC);
            if(outputRedir < 0)
            {
                fprintf(stderr,"Output error.\n");
                _exit(1);
            }
        
            if(dup2(outputRedir,1) < 0)
                _exit(1);
        }
        
        execvp(c->u.word[0],c->u.word);
        fprintf(stderr, "execvp shouldn't have returned!\n");
        _exit(1);
    }
    else
    {
        waitpid(p, &status, 0);
        exit_status = WEXITSTATUS(status);
        c->status = exit_status;
        return;
    }
}

void executingSubshell(command_t c)
{
	pid_t pid = fork();
    int status;
    
   	if (pid < 0)
    {
		error(1, errno, "fork was unsuccessful");
    }
    
	if (pid == 0){
		execute_switch(c->u.subshell_command);
		_exit(c->u.subshell_command->status);
	}
    else
    {
		waitpid(pid, &status, 0); // wait until child status is available
		int exitStatus = WEXITSTATUS(status); // extract exit status of child process
		c->status = exitStatus;
		return;
    }
}

void executingAnd(command_t c)
{
	pid_t pid = fork();
    int status;
    
   	if (pid < 0)
    {
		error(1, errno, "fork was unsuccessful");
    }
	
	if (pid == 0){
		execute_switch(c->u.command[0]);
		_exit(c->u.command[0]->status);
	}
	
	if (pid > 0) {
		waitpid(pid, &status, 0); // wait until child status is available
		int exitStatus = WEXITSTATUS(status); // extract exit status of child process

		if (exitStatus == 0){ //if left run successfully
			pid_t pid2 = fork(); 
			if (pid2 == 0){
				execute_switch(c->u.command[1]);
				_exit(c->u.command[1]->status);
			}
			
			if (pid2 > 0) {
				waitpid(pid2, &status, 0); 
				exitStatus = WEXITSTATUS(status);
			}
		}
			
		c->status = exitStatus;
		return;
	}
}


void executingOr(command_t c)
{
	pid_t pid = fork();
    int status;
    
   	if (pid < 0)
    {
		error(1, errno, "fork was unsuccessful");
    }
	
	if (pid == 0){
		execute_switch(c->u.command[0]);
		_exit(c->u.command[0]->status);
	}
	
	if (pid > 0) {
		waitpid(pid, &status, 0); // wait until child status is available
		int exitStatus = WEXITSTATUS(status); // extract exit status of child process

		if (exitStatus != 0){ //if left fails to run
			pid_t pid2 = fork(); 
			if (pid2 == 0){
				execute_switch(c->u.command[1]);
				_exit(c->u.command[1]->status);
			}
			
			if (pid2 > 0) {
				waitpid(pid2, &status, 0); 
				exitStatus = WEXITSTATUS(status);
			}
		}
			
		c->status = exitStatus;
		return;
	}
}

void executingSequence(command_t c)
{
	pid_t pid = fork();
    int status;
    
   	if (pid < 0)
    {
		error(1, errno, "fork was unsuccessful");
    }
	
	if (pid == 0){
		execute_switch(c->u.command[0]);
		_exit(c->u.command[0]->status);
	}
	
	if (pid > 0) {
		waitpid(pid, &status, 0); // wait until child status is available

		pid_t pid2 = fork(); 
		if (pid2 == 0){
			execute_switch(c->u.command[1]);
			_exit(c->u.command[1]->status);
		}
		
		if (pid2 > 0) {
			waitpid(pid2, &status, 0); 
			int exitStatus= WEXITSTATUS(status); // extract exit status of child process
			c->status = exitStatus;
			return;
		}
	}
}

void executingPipe(command_t c)
{
	pid_t returnedPid;
	pid_t firstPid;
	pid_t secondPid;
	int buffer[2];
	int eStatus;

	if ( pipe(buffer) < 0 )
	{
		error (1, errno, "pipe was not created");
	}

	firstPid = fork();
	if (firstPid < 0)
    {
		error(1, errno, "fork was unsuccessful");
    }
	else if (firstPid == 0) //child executes command on the right of the pipe
	{
		close(buffer[1]); //close unused write end

        //redirect standard input to the read end of the pipe
        //so that input of the command (on the right of the pipe)
        //comes from the pipe
		if ( dup2(buffer[0], 0) < 0 )
		{
			error(1, errno, "error with dup2");
		}
		execute_switch(c->u.command[1]);
		_exit(c->u.command[1]->status);
	}
	else 
	{
		// Parent process
		secondPid = fork(); //fork another child process
                            //have that child process executes command on the left of the pipe
		if (secondPid < 0)
		{
			error(1, 0, "fork was unsuccessful");
		}
        else if (secondPid == 0)
		{
			close(buffer[0]); //close unused read end
			if(dup2(buffer[1], 1) < 0) //redirect standard output to write end of the pipe
            {
				error (1, errno, "error with dup2");
            }
			execute_switch(c->u.command[0]);
			_exit(c->u.command[0]->status);
		}
		else
		{
			// Finishing processes
			returnedPid = waitpid(-1, &eStatus, 0); //this is equivalent to wait(&eStatus);
                        //we now have 2 children. This waitpid will suspend the execution of
                        //the calling process until one of its children terminates
                        //(the other may not terminate yet)

			//Close pipe
			close(buffer[0]);
			close(buffer[1]);

			if (secondPid == returnedPid )
			{
			    //wait for the remaining child process to terminate
				waitpid(firstPid, &eStatus, 0); 
				c->status = WEXITSTATUS(eStatus);
				return;
			}
			
			if (firstPid == returnedPid)
			{
			    //wait for the remaining child process to terminate
   				waitpid(secondPid, &eStatus, 0);
				c->status = WEXITSTATUS(eStatus);
				return;
			}
		}
	}	
}

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, bool time_travel)
{
   /*FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  
    */

	if (time_travel == false)
	{
	    execute_switch(c);
	}
}
