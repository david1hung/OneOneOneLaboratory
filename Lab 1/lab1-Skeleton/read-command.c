// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <ctype.h>
#include <error.h>
#include <string.h>

/* FIXME: You may need to add #include directives, macro definitions,
 static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
 complete the incomplete type declaration in command.h.  */

struct command_stream
{
    command* s;
} stream;


/* Returns true or false depending on whether t has changed. Additional
    logic is required for OR/PIPE distinctions because |, ||. We use this
    function to break between commands. */

static
bool set_command_type(command_type_t* t, char c)
{
    if(ischar(c) && *t != SIMPLE_COMMAND)
        *t = SIMPLE_COMMAND;
    else if(c == '&' && *t != AND_COMMAND)
        *t = AND_COMMAND;
    else if(c == ';' && *t != SEQUENCE_COMMAND)
        *t = SEQUENCE_COMMAND;
    else if(c == '|' && *t != OR_COMMAND)
        *t = OR_COMMAND;
    else if(c == '(' && *t != SUBSHELL_COMMAND)
        *t = SUBSHELL_COMMAND;
    else
        return false;

    return true;
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

    char c;
    long int k=0;
    command_type_t t;
    while((c = get_next_byte(get_next_byte_argument)) > 0)
    {
/*      
*/
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
