// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <ctype.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: You may need to add #include directives, macro definitions,
 static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
 complete the incomplete type declaration in command.h.  */

struct command_stream
{
    command_t* s;
} stream;

int ncommands = 0;
int ncapacity = 128;


/* Returns true or false depending on whether t has changed. Additional
    logic is required for OR/PIPE distinctions because |, ||. We use this
    function to break between commands. */

static
bool set_command_type(enum command_type* t, char c)
{
    if(isalpha(c) && *t != SIMPLE_COMMAND)
        *t = SIMPLE_COMMAND;
    else if(c == '&' && *t != AND_COMMAND)
        *t = AND_COMMAND;
    else if(c == ';' && *t != SEQUENCE_COMMAND)
        *t = SEQUENCE_COMMAND;
    else if(c == '|' && *t != OR_COMMAND)
        *t = OR_COMMAND;
    else if((c == '(' || c == ')') && *t != SUBSHELL_COMMAND)
        *t = SUBSHELL_COMMAND;
    else
        return false;

    return true;
}

static
command_t generate_command(char* buf, int size)
{
    command_t temp;
    long int k;
    for(k=0;k<size;k++)
        printf("%c",buf[k]);
    printf("\n");
    return temp;
}

/* Create a command stream from GETBYTE and ARG.  A reader of
 the command stream will invoke GETBYTE (ARG) to get the next byte.
 GETBYTE will return the next input byte, or a negative number
 (setting errno) on failure.  */
 command_stream_t
 make_command_stream (int (*get_next_byte) (void *),
   void *get_next_byte_argument)
 {
    /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    stream.s = (command_t *)malloc(sizeof(command_t)*ncommands);

    char* buf = (char *)malloc(216); // Buffer to store the chars-in-progress
    long int buf_size = 216;

    char c;
    long int k = 0;
    enum command_type t;

    while((c = (char)get_next_byte(get_next_byte_argument)) > 0)
    {    
        // This block is triggered if an only if the command_type
        //  has changed, i.e. there is a distinctly different
        //  type of operand being dealt with.
        if(set_command_type(&t,c) || c == '\n')
        {
            command_t cmd = generate_command(buf,k);
            
            if(ncommands == ncapacity)
            {
                stream.s = (command_t *)realloc((void *)stream.s,
                    sizeof(command_t)*ncommands*2);

                ncommands *= 2;
            }
            
            stream.s[ncommands] = cmd;
            ncommands++;
            k=0;

            if(c == '\n')
                continue;
        }

        // Realloc logic in the case that the chars-in-progress
        //  exceed the size of the buffer array.
        if(k == buf_size)
        {
            buf = (char *)realloc((void *)buf, buf_size*2);
            buf_size *=2;
        }

        buf[k] = c;
        k++;
    }
    
    error (1, 0, "command reading not yet implemented");
    return 0;
}

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
 an error, report the error and exit instead of returning.  */
command_t
read_command_stream (command_stream_t s)
{
    /* FIXME: Replace this with your implementation too.  */
    error (1, 0, "command reading not yet implemented");
    return 0;
}
