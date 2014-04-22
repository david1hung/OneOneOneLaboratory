
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

/* NOTE: Lab 1C implementation begins here. */

//  Dependency node for the dependency list. I don't think we need to touch any
//      of it directly, you can interface with dependency-removal using the
//      remove_dependency_node function.

struct dependency_node {
    //int nid;
    pid_t pid;
    command_t command;

    char *input[NODE_MAX];
    int ninput;
    char *output[NODE_MAX];
    int noutput;

    //int *dependencies;
    struct dependency_node *before_list[NODE_MAX]; //**
    int list_size;
    
    struct dependency_node *next;
};

//  Dependency list. Singly linked list. After creating one, initialize the two
//      member variables using init_dependency_list. To add a node, call
//      add_dependency_node, and to remove a node (and traces of its 
//      dependencies), call remove_dependency_node with the relevant nid.
struct dependency_list {
    struct dependency_node *head;
    long int size;
};

//  Initializes the dependency list.
void init_dependency_list(struct dependency_list *l)
{
    l->head = NULL;
    l->size = 0;
}

//  Function declaration: refer below for better description.
void populate_input_output(struct dependency_node *n, command_t c);

//  add_dependency_node uses barebones enumeration to give each node an nid:
//      the nid is exactly the list's size. We assume that once any form of
//      node removal occurs, there will no longer be any add_dependency_node
//      function calls.
void add_dependency_node(struct dependency_list *l, command_t c)
{
    // Here, we start by generating and initializing the dependency node.
    struct dependency_node *node =
            (struct dependency_node *)malloc(sizeof(struct dependency_node));

    //node->nid = l->size;
    node->command = c;
    node->ninput = 0;
    node->noutput = 0;
    node->next = NULL;

    node->pid = -1; //**
    node->list_size = 0; //**
    
    populate_input_output(node, c);
    
    // Here, we cap each of the input and output lists.
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

//  add_dependency **
void add_dependency(struct dependency_node *cur, struct dependency_node *toAdd)
{    
    int size = cur->list_size;
    cur->before_list[size] = toAdd;
    (cur->list_size)++;
    return;
}

//  populate_input_output is a helper function for add_dependency_node. The
//      reason why we place it in a separate function is because it is called
//      recursively: it traverses, in post-order, through the command passed
//      to it and populates the input and output arrays. As noted earlier, these
//      two arrays are "capped" with a NULL pointer in the add_dependency_node
//      function.
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
            //Remark, we could add a check for '-' at the start of the word which would be the options added.
            //But for echo -n, it prints -n anyways... 
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

//  Function declaration: refer below for better description.
bool probe_sequence_command(command_t c);

//  populate_dependency_list, although ambiguously named, serves the important
//      purpose of populating the dependency_list struct, not the array of
//      dependencies within the dependency_node. It uses probe_sequence_command
//      as a helper function (purpose elaborated later) to divide the root
//      tree into the simplest subtrees divided by SEQUENCE_COMMAND nodes.
//      It works recursively by once again iterating over the tree in post-order
//      and adding to the dependency_list subtrees that don't contain
//      SEQUENCE_COMMANDs. Just as a quick example, suppose we have the command
//          echo a && echo b ; echo c ; echo d
//      The output looks approximately like:
//                echo a \
//              &&
//                echo b \
//            ;
//              echo c \
//            ;
//              echo d
//  In this case, we are interested in breaking apart the tree such that we 
//      divide at each of the SEQUENCE_COMMANDs, at the semicolons. 
void populate_dependency_list(struct dependency_list *l, command_t c)
{
    if(c == NULL)
        return;
    else
    {
        //  Note: probe_sequence_command abstractly serves to return a boolean
        //      value depending on whether there is a SEQUENCE_COMMAND in the
        //      current or descendent nodes.
        if(!probe_sequence_command(c)) // If there aren't any for this node
        {                              //     then this node is some form of
            add_dependency_node(l, c); //     operand.
            return;
        }
        
        // We probe left then right subtrees.
        bool left_sequence = probe_sequence_command(c->u.command[0]);
        bool right_sequence = probe_sequence_command(c->u.command[1]);
        
        //  We recursively call the function or add the subtree to the list in
        //      post-order sequence.
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

//  probe_sequence_command basically runs from the current command through to
//      the terminating cases: SIMPLE_COMMAND, SUBSHELL_COMMAND, or supposedly
//      NULL (which should never occur) and checks for any SEQUENCE_COMMANDS.
bool probe_sequence_command(command_t c)
{
    if(c == NULL || c->type == SIMPLE_COMMAND || c->type == SUBSHELL_COMMAND)
        return false;
    else if(c->type == SEQUENCE_COMMAND)
        return true;
    
    //  At this point, we have a non-SIMPLE_COMMAND/SUBSHELL_COMMAND so it's o-k
    //      to probe both subtrees.
    return probe_sequence_command(c->u.command[0]) ||
         probe_sequence_command(c->u.command[1]);
}

//  Function declaration: refer below for better description.
void populate_dependencies(struct dependency_list *list);

//  get_dependency_list basically creates the list (using all of the above 
//      functions) and returns it.
struct dependency_list *get_dependency_list(command_t root)
{
    struct dependency_list *list =
        (struct dependency_list*)malloc(sizeof(struct dependency_list));
    
    init_dependency_list(list);
    populate_dependency_list(list, root);
    populate_dependencies(list);
    
    return list;
}


//  populate_dependencies populates the dependencies by following a pseudocode
//      of the form:
//      for each node
//          for each output
//              for each subsequent node's inputs
//                  if(output == input)
//                      add the node to the dependency list                
//  Which is the main reason for its seemingly daunting for loops. Otherwise, it
//      is a fairly unexceptional function involving two layers of iterators.
//  Similarily code exist for (Read then Write and Write then write)   
void populate_dependencies(struct dependency_list *list)
{
    struct dependency_node *node = list->head;
    
    node = list->head;
    while(node != NULL)
    {
        long int i;

        //check Write And Read
        for(i=0; node->output[i] != NULL; i++)
        {
            struct dependency_node *iter = node->next;
            while(iter != NULL)
            {
                long int j;
                for(j=0; iter->input[j] != NULL; j++)
                {
                    if(strcmp(node->output[i], iter->input[j]) == 0)
                    {
                        add_dependency(iter, node);
                    }
                }
                iter = iter->next;
            }
        }
        
        //Check Write and Write
        for(i=0; node->output[i] != NULL; i++)
        {
            struct dependency_node *iter = node->next;
            while(iter != NULL)
            {
                long int j;
                for(j=0; iter->output[j] != NULL; j++)
                {
                    if(strcmp(node->output[i], iter->output[j]) == 0)
                    {
                        add_dependency(iter, node);
                    }
                }
                iter = iter->next;
            }
        }  

        //Check Read and Write 
        for(i=0; node->input[i] != NULL; i++)
        {
            struct dependency_node *iter = node->next;
            while(iter != NULL)
            {
                long int j;
                for(j=0; iter->output[j] != NULL; j++)
                {
                    if(strcmp(node->input[i], iter->output[j]) == 0)
                    {
                        add_dependency(iter, node);
                    }
                }
                iter = iter->next;
            }
        }     
        node = node->next;
    }
}

void complete_sequence_tree(command_t c)
{
    if(c == NULL)
        return;
    else
    {
        if(!probe_sequence_command(c))
            return;
        
        // We probe left then right subtrees.
        bool left_sequence = probe_sequence_command(c->u.command[0]);
        bool right_sequence = probe_sequence_command(c->u.command[1]);
        
        //  We recursively call the function or add the subtree to the list in
        //      post-order sequence.
        if(left_sequence)
            complete_sequence_tree(c->u.command[0]);
        
        if(right_sequence)
            complete_sequence_tree(c->u.command[1]);
        
        // We assume that we only had splits at SEQUENCE_COMMANDs
        if(c->type == SEQUENCE_COMMAND)
            c->status = c->u.command[1]->status;
    }
}


// Use this to print the list!
void debug_list(struct dependency_list *list)
{
    struct dependency_node *node = list->head;
    while(node != NULL)
    {
        long int i;
        //printf("nid:     %d\n",node->nid);
        printf("input:   ");
        for(i=0; node->input[i] != NULL; i++)
            printf("%s ", node->input[i]);
        printf("\noutput:  ");
        for(i=0; node->output[i] != NULL; i++)
            printf("%s ", node->output[i]);
        printf("\ndep:     ");
        //for(i=0; node->dependencies[i] != -1; i++)
        //    printf("%d ", node->dependencies[i]);
        //printf("\n\n");
        node = node->next;
    }
}

//Destructor to clean up all and deallocate all the nodes
void 
clean_up_nodes(struct dependency_list *l)
{
    struct dependency_node *cur = l->head;
    struct dependency_node *next = NULL;
    while (cur != NULL)
    {
        next = cur->next;
        free(cur);
        cur = next;
    }
    l->head = NULL;
    l->size = 0;
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
    //debug_list(list);

    // Supposely the code to execute command in parallel
    
    struct dependency_node *node = list->head;

    int status;
    //Run process with satisfied dependencies
    while (node != NULL)
    {   
        //Wait for dependencies to complete
        int i; 
        
        int dependencySize = node->list_size;
        //wait for all dependencies to finish
        loop_label: 
            for (i = 0;i < dependencySize; i++)
            {                
                if (node->before_list[i]->pid == -1) 
                    goto loop_label;
            }

        //Reaps of the dependencet processes that have become zombie
        for (i = 0;i < dependencySize; i++)
        {                
            pid_t checkpid = node->before_list[i]->pid;
                waitpid(checkpid, &status, 0);
        }
        

        pid_t pid = fork();
        if (pid == 0) {
            execute_switch(node->command);
            _exit(node->command->status);
        }
        if (pid > 0)
        {
            node->pid = pid;
        }

        node = node->next;
    }
    while(wait(&status)>0)
      continue;

    complete_sequence_tree(c);
    clean_up_nodes(list);

}
