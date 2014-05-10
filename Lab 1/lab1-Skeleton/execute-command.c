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
void executingSubshell(command_t c, bool xtrace);
void executingAnd(command_t c, bool xtrace);
void executingOr(command_t c, bool xtrace);
void executingSequence(command_t c, bool xtrace);
void executingPipe(command_t c, bool xtrace);

void execute_switch(command_t c, bool xtrace)
{
    c->status = -1;
    
	switch(c->type)
	{
	case SIMPLE_COMMAND:
    if(xtrace)
    {
        printf("+ ");
        print_line(c);
        printf("\n");
    }
		executingSimple(c);
		break;
	case SUBSHELL_COMMAND:
		executingSubshell(c, xtrace);
		break;
	case AND_COMMAND:
		executingAnd(c, xtrace);
		break;
	case OR_COMMAND:
		executingOr(c, xtrace);
		break;
	case SEQUENCE_COMMAND:
		executingSequence(c, xtrace);
		break;
	case PIPE_COMMAND:
		executingPipe(c, xtrace);
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

void executingSubshell(command_t c, bool xtrace)
{
	pid_t pid = fork();
    int status;
    
   	if (pid < 0)
    {
		error(1, errno, "fork was unsuccessful");
    }
    
	if (pid == 0){
		execute_switch(c->u.subshell_command, xtrace);
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

void executingAnd(command_t c, bool xtrace)
{
	pid_t pid = fork();
    int status;
    
   	if (pid < 0)
    {
		error(1, errno, "fork was unsuccessful");
    }
	
	if (pid == 0){
		execute_switch(c->u.command[0], xtrace);
		_exit(c->u.command[0]->status);
	}
	
	if (pid > 0) {
		waitpid(pid, &status, 0); // wait until child status is available
		int exitStatus = WEXITSTATUS(status); // extract exit status of child process

		if (exitStatus == 0){ //if left run successfully
			pid_t pid2 = fork(); 
			if (pid2 == 0){
				execute_switch(c->u.command[1], xtrace);
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


void executingOr(command_t c, bool xtrace)
{
	pid_t pid = fork();
    int status;
    
   	if (pid < 0)
    {
		error(1, errno, "fork was unsuccessful");
    }
	
	if (pid == 0){
		execute_switch(c->u.command[0], xtrace);
		_exit(c->u.command[0]->status);
	}
	
	if (pid > 0) {
		waitpid(pid, &status, 0); // wait until child status is available
		int exitStatus = WEXITSTATUS(status); // extract exit status of child process

		if (exitStatus == 1){ //if left fails to run
			pid_t pid2 = fork(); 
			if (pid2 == 0){
				execute_switch(c->u.command[1], xtrace);
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

void executingSequence(command_t c, bool xtrace)
{
	pid_t pid = fork();
    int status;
    
   	if (pid < 0)
    {
		error(1, errno, "fork was unsuccessful");
    }
	
	if (pid == 0){
		execute_switch(c->u.command[0], xtrace);
		_exit(c->u.command[0]->status);
	}
	
	if (pid > 0) {
		waitpid(pid, &status, 0); // wait until child status is available

		pid_t pid2 = fork(); 
		if (pid2 == 0){
			execute_switch(c->u.command[1], xtrace);
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

void executingPipe(command_t c, bool xtrace)
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
		execute_switch(c->u.command[1], xtrace);
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
			execute_switch(c->u.command[0], xtrace);
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
    int nid;
    command_t command;

    char *input[NODE_MAX];
    int ninput;
    char *output[NODE_MAX];
    int noutput;

    int *dependencies;
    int status; // 0 not yet run, 1 running.
    pid_t pid;
    
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

    node->nid = l->size;
    node->command = c;
    node->ninput = 0;
    node->noutput = 0;
    node->status = 0;
    node->next = NULL;
    
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

//  remove_dependency_node is a two part process. It begins by deleting a node
//      according to nid, and subsequently removes itself from the dependency
//      lists of all subsequent nodes. Checking succeeding nodes is sufficient
//      on the basis that you cannot depend on something you precede.
struct dependency_node *remove_dependency_node(struct dependency_list *l, int nid)
{
    if(l->head == NULL) // In the case that the list is empty.
        return NULL;
    else // Otherwise ...
    {
        struct dependency_node *prev = NULL;
        struct dependency_node *traversal = l->head;
        while(traversal != NULL)
        {
            if(traversal->nid == nid)
            {   
                
                struct dependency_node *tmp;
                if(prev == NULL) // If it's the first node!
                {
                    l->head = traversal->next;
                    tmp = traversal->next;
                    free(traversal->dependencies);
                    free(traversal);
                }
                else
                {
                    prev->next = traversal->next;
                    tmp = traversal->next;
                    free(traversal->dependencies);
                    free(traversal);
                }
                
                struct dependency_node *iter = tmp;
                while(iter != NULL)
                {
                    if(iter->dependencies[nid] == 1)
                        iter->dependencies[nid] = 0;
                    iter = iter->next;
                }
                
                (l->size)--;
                return tmp;
            }
            
            prev = traversal;
            traversal = traversal->next;
        }
        
        return prev->next;
    }
    return NULL;
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
            if(c->u.word[i][0] != '-')
            {
                n->input[n->ninput] = c->u.word[i];
                (n->ninput)++;
            }
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
        if(!probe_sequence_command(c)) // SIMPLE, SUBSHELL_COMMAND
        {
            add_dependency_node(l, c);
            return;
        }
        
        //  Here, we can reasonably assume top-level operands are SEQUENCEs.
        //      We probe left then right subtrees.
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
    while(node != NULL)
    {
        //  We need this array to be list->size + 1 so that we can fit the
        //      capping -1 for later iterating.
        node->dependencies = (int *)malloc(sizeof(int)*(list->size + 1));
        long int i;
        for(i=0; i<list->size; i++)
            node->dependencies[i] = 0;
        node->dependencies[i] = -1;
        node = node->next;
    }
    
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
                if(iter->dependencies[node->nid] == 0)
                {
                    long int j;
                    for(j=0; iter->input[j] != NULL; j++)
                    {
                        if(strcmp(node->output[i], iter->input[j]) == 0)
                        {
                            iter->dependencies[node->nid] = 1;
                        }
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
                if(iter->dependencies[node->nid] == 0)
                {
                    long int j;
                    for(j=0; iter->output[j] != NULL; j++)
                    {
                        if(strcmp(node->output[i], iter->output[j]) == 0)
                        {
                            iter->dependencies[node->nid] = 1;
                        }
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
                if(iter->dependencies[node->nid] == 0)
                {
                    long int j;
                    for(j=0; iter->output[j] != NULL; j++)
                    {
                        if(strcmp(node->input[i], iter->output[j]) == 0)
                        {
                            iter->dependencies[node->nid] = 1;
                        }
                    }
                }
                iter = iter->next;
            }
        }     
        node = node->next;
    }
}

//  Returns true if the dependency_list is empty.
bool dependency_list_empty(struct dependency_list *list)
{
    return list->head == NULL;
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

bool has_dependencies(struct dependency_node *node)
{
    int i;
    for(i=0; i < node->nid; i++)
        if(node->dependencies[i] == 1)
            return true;
    
    return false;
}

// Use this to print the list!
void debug_list(struct dependency_list *list)
{
    struct dependency_node *node = list->head;
    while(node != NULL)
    {
        long int i;
        printf("nid:     %d\n",node->nid);
        printf("input:   ");
        for(i=0; node->input[i] != NULL; i++)
            printf("%s ", node->input[i]);
        printf("\noutput:  ");
        for(i=0; node->output[i] != NULL; i++)
            printf("%s ", node->output[i]);
        printf("\ndep:     ");
        for(i=0; node->dependencies[i] != -1; i++)
            printf("%d ", node->dependencies[i]);
        printf("\n\n");
        node = node->next;
    }
}

void
print_line (command_t c)
{
    switch (c->type)
      {
      case AND_COMMAND:
      case SEQUENCE_COMMAND:
      case OR_COMMAND:
      case PIPE_COMMAND:
        {
  	print_line (c->u.command[0]);
  	static char const command_label[][3] = { "&&", ";", "||", "|" };
  	printf ("%s ", command_label[c->type]);
  	print_line (c->u.command[1]);
  	break;
        }

      case SIMPLE_COMMAND:
        {
  	char **w = c->u.word;
  	printf ("%s ", *w);
  	while (*++w)
  	  printf ("%s ", *w);
  	break;
        }

      case SUBSHELL_COMMAND:
      printf("( ");
        print_line (c->u.subshell_command);
      printf(") ");
        break;

      default:
        return;
      }

    if (c->input)
      printf ("<%s ", c->input);
    if (c->output)
      printf (">%s ", c->output);
}

void
execute_command (command_t c, bool time_travel, bool xtrace,
                        bool verbose)
{
   /*FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  
    */
    
    //  Start 1C time_travel implementation
    //  Let's start with populating an array in post-order from 'c' such that we
    //      can get a dependency list going.
    struct dependency_list *list = get_dependency_list(c);
    // debug_list(list);
    
    if(time_travel == false)
    {
        struct dependency_node *iter = list->head;
        while(iter != NULL)
        {
            if(verbose)
            {
                print_line(iter->command);
                printf("\n");
            }
            
            int status;
            execute_switch(iter->command, xtrace);
            iter = remove_dependency_node(list, iter->nid);
            
        }
        
        complete_sequence_tree(c);
        return;
    }


    if(verbose)
    {
        print_line(c);
        printf("\n");
    }

    while (!dependency_list_empty(list))
    {
        struct dependency_node *iter = list->head;
        while(iter != NULL)
        {
            int status;
            if(iter->status == 1)
                if(waitpid(iter->pid, &status, WNOHANG) == iter->pid)
                {
                    // printf("(W) The dragon [%d] has been slain! Clearing up the dependencies ...\n", iter->nid);
                    int exit_status = WEXITSTATUS(status);
                    iter->command->status = exit_status;
                    iter = remove_dependency_node(list, iter->nid);
                    continue;
                }
            
            if(iter != NULL)
                iter = iter->next;
        }

        for(iter = list->head; iter != NULL; iter = iter->next)
        {
            if(iter->status == 1)
                continue;
            
            if(has_dependencies(iter))
                continue;
            
            // printf("(F) The hero   [%d] is born!\n", iter->nid);
            
            pid_t pid = fork();
            if (pid == 0) {
                execute_switch(iter->command, xtrace);
                _exit(iter->command->status);
            }
            if (pid > 0)
            {
                iter->pid = pid;
                iter->status = 1;
            }
        }
    }
    
    complete_sequence_tree(c);

    //debug_list(list);

}
