// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

struct command_stream
{
  command* stream;
} cstream;

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
    
    // david|
    // daviide<

     char c;
     long int k=0;
     command_type t;
     while((c = get_next_byte(get_next_byte_argument)) > 0)
     {


        // 1. Process commands
        // 2. Processing string
        // 3. Process subshell
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
