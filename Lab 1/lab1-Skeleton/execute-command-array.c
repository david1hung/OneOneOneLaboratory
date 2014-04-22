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
#include <string.h>

#define NODE_MAX 100

void executingSimple(command_t c);
void executingSubshell(command_t c);
void executingAnd(command_t c);
void executingOr(command_t c);
void executingSequence(command_t c);
void executingPipe(command_t c);

void execute_switch(command_t c)
{
    c->status = -1;
    
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
            {
                fprintf(stderr,"Redirect error!\n",c->input);
                _exit(1);
            }
            
            close(inputRedir);
        }
    
        int outputRedir;
        if(c->output != NULL)
        {
            outputRedir = open(c->output, O_WRONLY|O_CREAT|O_TRUNC, 0644);
            if(outputRedir < 0)
            {
                fprintf(stderr,"Output error.\n");
                _exit(1);
            }
        
            if(dup2(outputRedir,1) < 0)
            {
                fprintf(stderr,"Redirect error!\n",c->input);
                _exit(1);
            }
            
            close(outputRedir);
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

		if (exitStatus == 1){ //if left fails to run
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

struct dependency_node {
    
    command_t command;
    char *input[NODE_MAX];
    int ninput;
    char *output[NODE_MAX];
    int noutput;
    
    //  In practice, we would be wasting the status
    //     member variable if we were to have an
    //     array of pids. Also, it gives us the
    //     flexibility to pop nodes once they're
    //     complete.
    command_t dependencies[NODE_MAX];
    int ndependencies;
    
    struct dependency_node *next;
};

struct dependency_list {
    struct dependency_node *head;
    long int size;
};

void init_dependency_list(struct dependency_list *l) {
    l->head = NULL;
    l->size = 0;
}

void populate_input_output(struct dependency_node *n, command_t c)
{
    if(c == NULL)
        return;
    if(c->type == SIMPLE_COMMAND)
    {
        if(c->input != NULL)
        {
            n->input[n->ninput] = c->input;
            (n->ninput)++;
        }
        
        if(c->output != NULL)
        {
            n->output[n->noutput] = c->output;
            (n->noutput)++;
        }
        
        long int i;
        for(i=1; c->u.word[i] != NULL; i++)
        {
            n->input[n->ninput] = c->u.word[i];
            (n->ninput)++;
        }

    }
    else if(c->type == SUBSHELL_COMMAND)
    {
        populate_input_output(n, c->u.subshell_command);
    }
    else
    {
        populate_input_output(n, c->u.command[0]);
        populate_input_output(n, c->u.command[1]);
    }
}

void add_dependency_node(struct dependency_list *l, command_t c) {
    // Here, we start by generating and initializing the dependency node.
    struct dependency_node *node =
            (struct dependency_node *)malloc(sizeof(struct dependency_node));
    node->command = c;
    node->ninput = 0;
    node->noutput = 0;
    node->ndependencies = 0;
    node->next = NULL;
    
    populate_input_output(node, c);
    node->input[node->ninput] = NULL;
    node->output[node->noutput] = NULL;
    
    // Then we perform the relevant node-addition actions.
    if(l->head == NULL) // In the case that the list is empty.
        l->head = node;
    else // Otherwise ...
    {
        struct dependency_node *traversal = l->head;
        while(traversal->next != NULL)
            traversal = traversal->next;
        traversal->next = node;
    }
    
    (l->size)++;
    return;
}

/*
bool remove_dependency_node(struct dependency_list *l, command_t c) {
    if(l->head == NULL) // In the case that the list is empty.
        return false;
    else // Otherwise ...
    {
        struct dependency_node *traversal = l->head;
        while(traversal->next != NULL)
        {
            if(traversal->next->command == c)
                break;
            traversal = traversal->next;
        }
    }
    
    return node;
}
*/

// Probes the command tree starting from c for any sequence commands. If none
//     are encountered, the function returns false.
bool probe_sequence_command(command_t c)
{
    if(c == NULL || c->type == SIMPLE_COMMAND || c->type == SUBSHELL_COMMAND)
        return false;
    else if(c->type == SEQUENCE_COMMAND)
        return true;
    
    return probe_sequence_command(c->u.command[0]) ||
         probe_sequence_command(c->u.command[1]);
}

void populate_dependency_list(struct dependency_list *l, command_t c)
{
    if(c == NULL)
        return;
    else
    {
        if(!probe_sequence_command(c))
        {
            add_dependency_node(l, c);
            return;
        }
        
        bool left_sequence = probe_sequence_command(c->u.command[0]);
        bool right_sequence = probe_sequence_command(c->u.command[1]);
        
        if(left_sequence)
            populate_dependency_list(l, c->u.command[0]);
        else
            add_dependency_node(l, c->u.command[0]);
        
        if(right_sequence)
            populate_dependency_list(l, c->u.command[1]);      
        else
            add_dependency_node(l, c->u.command[1]);  
    }
}

struct dependency_list *get_dependency_list(command_t root)
{
    struct dependency_list *list =
        (struct dependency_list*)malloc(sizeof(struct dependency_list));
    
    populate_dependency_list(list, root);
    return list;
}

void populate_dependencies(struct dependency_list *list)
{
    struct dependency_node *node = list->head;
    while(node != NULL)
    {
        long int i;
        for(i=0; node->output[i] != NULL; i++)
        {
            struct dependency_node *iter = node->next;
            while(iter != NULL)
            {
                long int j;
                for(j=0; iter->input[j] != NULL; j++)
                {
                    printf("(%s, %s)\n",node->output[i],iter->input[j]);
                    if(strcmp(node->output[i], iter->input[j]) == 0)
                    {
                        iter->dependencies[iter->ndependencies] = node->command;
                        (iter->ndependencies)++;
                    }
                }
                iter = iter->next;
            }
        }
        node->dependencies[node->ndependencies] = NULL;
        node = node->next;
    }
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
        return;
	}
    
    //  Start 1C time_travel implementation
    //  Let's start with populating an array in post-order from 'c' such that we
    //      can get a dependency list going.
    struct dependency_list *list = get_dependency_list(c);
    populate_dependencies(list);
    
    struct dependency_node *node = list->head;
    while(node != NULL)
    {
        long int i;
        printf("input: ");
        for(i=0; node->input[i] != NULL; i++)
            printf("%s / ", node->input[i]);
        printf("\noutput: ");
        for(i=0; node->output[i] != NULL; i++)
            printf("%s / ", node->output[i]);
        printf("\ndependencies: ");
        for(i=0; node->dependencies[i] != NULL; i++)
            printf("%x / ", node->dependencies[i]);
        printf("\n\n");
        node = node->next;
    }
    
    
}
