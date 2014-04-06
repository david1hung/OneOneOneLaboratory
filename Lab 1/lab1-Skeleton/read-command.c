// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <ctype.h>
// #include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

typedef enum {
    SIMPLE_STATE, INPUT_STATE, OUTPUT_STATE, AND_STATE, OR_STATE,
        OPEN_SUBSHELL_STATE, CLOSE_SUBSHELL_STATE, SEQUENCE_STATE,
        NEWLINE_STATE, COMMENT_STATE, NULL_STATE
} state;

struct command_stream {
    command_t head;
} stream;

struct node {
    command_t command;
    char **payload;
    struct node *next;
};

struct stack {
    struct node *head;
};

static
void init_stack(struct stack *s)
{
    s->head = NULL;
}

static
void push(struct stack *s, command_t c)
{
    struct node *n = (struct node *)malloc(sizeof(struct node));
    n->command = c;
    n->next = s->head;

    if(s->head == NULL)
        s->head = n;
    else
        s->head = n;
}


static
command_t pop(struct stack *s)
{
    if(s->head == NULL)
        return NULL;

    struct node *n = s->head;
    s->head = n->next;

    command_t c = n->command;
    free(n);
    return c;
}

static
bool needs_splitting(state *s, char c)
{
    if(c == ' ')
    {
        *s = NULL_STATE;
        return true;
    }
    else if (c == '<')
    {
        *s = INPUT_STATE;
        return true;
    }
    else if (c == '>')
    {
        *s = OUTPUT_STATE;
        return true;
    }    
    else if (c == '(')
    {
        *s = OPEN_SUBSHELL_STATE;
        return true;
    }
    else if (c == ')')
    {
        *s = CLOSE_SUBSHELL_STATE;
        return true;
    }
    else if (c == ';')
    {
        *s = SEQUENCE_STATE;
        return true;
    }
    else if (c == '#')
    {
        *s = COMMENT_STATE;
        return true;
    }
    else if((isalnum(c) || c == '!' || c == '%' || c == '+' ||
                c == ',' || c == '-' || c == '.' || c == '/' ||
                c == ':' || c == '@' || c == '^' || c == '_') && 
                *s != SIMPLE_STATE)
    {
        *s = SIMPLE_STATE;
        return true;
    }
    else if(c == '&' && *s != AND_STATE)
    {
        *s = AND_STATE;
        return true;
    }
    else if(c == '|' && *s != OR_STATE)
    {
        *s = OR_STATE;
        return true;
    }
    else if(c == '\n' && *s != NEWLINE_STATE)
    {
        *s = NEWLINE_STATE;
        return true;
    }
    else
        return false;
}

/* Returns 0 if cmd->cmd, 1 if cmd->op, -1 if op->cmd */

static
int get_state(state *s, char c)
{
    state s_old = *s;
    needs_splitting(s, c);

    int a = s_old/3;
    int b = (*s)/3;

    if(a == b)
        return 0;
    else if(a < b)
        return 1;
    else
        return -1;
}

static
long int get_token_length(char *t)
{
    long int k;
    for(k = 0; t[k] != '\0'; k++) {}
    return k;
}


static
long int validate_token(char *token)
{
    state s;
    char c0 = token[0];
    needs_splitting(&s, c0);

    if(s == AND_STATE || s == OR_STATE || s == NEWLINE_STATE)
    {
        long int k;
        for(k = 0; token[k] != '\0'; k++)
        {
            if(k == 2 && s != NEWLINE_STATE)
            {
                fprintf(stderr, "Syntax error: {|...|, &...&}\n");
                exit(1);
            }
        }

        if(s == AND_STATE)
        {
            if(k != 2)
            {
                fprintf(stderr, "Syntax error: {&}\n");
                exit(1);
            }
        }

        return k;
    }
}


static
char **get_tokens(int (*get_next_byte) (void *), 
                    void *get_next_byte_argument, long int *ntokens)
{
    char c;
    long int tokens_position = 0;
    long int tokens_size = 1024;
    char **tokens = (char **)malloc(sizeof(char *)*tokens_size);
    state s = NULL_STATE;

    long int buf_position = 0;
    long int buf_size = 216;
    char *buf = (char *)malloc(sizeof(char)*buf_size);

    while((c = get_next_byte(get_next_byte_argument)) > 0)
    {
        if(!needs_splitting(&s, c))
        {
            if(buf_position == buf_size)
            {
                buf_size *= 2;
                buf = (char *)realloc((void *)buf, sizeof(char)*buf_size);
            }

            buf[buf_position] = c;
            buf_position++;
        }
        else
        {
            if(buf_position > 0)
            {
                char *tmp = (char *)malloc(sizeof(char)*buf_position);
                long int i;
                for(i = 0; i < buf_position; i++)
                    tmp[i] = buf[i];
                tmp[i] = '\0';

                validate_token(tmp);

                if(tokens_position == tokens_size)
                {
                    tokens_size *= 2;
                    tokens = (char **)realloc((void *)tokens,
                        sizeof(char *)*tokens_size);
                }

                tokens[tokens_position] = tmp;
                tokens_position++;
            }

            buf_position = 0;

            if(c != ' ')
            {
                buf[buf_position] = c;
                buf_position++;
            }
        }
    }

    if(buf_position > 0)
    {
        char *tmp = (char *)malloc(sizeof(char)*buf_position);
        long int i;
        for(i = 0; i < buf_position; i++)
            tmp[i] = buf[i];
        tmp[i] = '\0';

        validate_token(tmp);

        if(tokens_position == tokens_size)
        {
            tokens_size *= 2;
            tokens = (char **)realloc((void *)tokens,
                sizeof(char *)*tokens_size);
        }

        tokens[tokens_position] = tmp;
        tokens_position++;
    }

    buf_position = 0;

    if(c != ' ')
    {
        buf[buf_position] = c;
        buf_position++;
    }

    *ntokens = tokens_position;
    return tokens;
}


/* We're going to skimp out on having two separate stack classes here:
    a command with command_type SUBSHELL_COMMAND can fall under two categories.
        1. status = 0 -> '('
        2. status = 1 -> ')' */

/*  SIMPLE_STATE, INPUT_STATE, OUTPUT_STATE, AND_STATE, OR_STATE,
    OPEN_SUBSHELL_STATE, CLOSE_SUBSHELL_STATE, SEQUENCE_STATE,
    NEWLINE_STATE, COMMENT_STATE, NULL_STATE */

static
void populate_stacks(char **tokens, long int ntokens,
    struct stack *op, struct stack *cmd)
{
    /* ... */
}

static
command_t *split_line(char **line, long int ntokens)
{
    state s = NULL_STATE;
    command_t c = malloc(sizeof(struct command));

    long int i;
    for(i = 0; i < ntokens; i++)
    {
        state old_s = s;
        int delta = get_state(&s, line[i][0]);
        
        if(delta == 0)
        {
            // i++;
            // check whether i is still within the bounds
            // take the token

            // if we *just* enter a simple state, we need to note
            // that we are in the simple state for the next iteration
            // NEED: SIMPLE_COMMAND COUNTER THAT RESETS UPON COMPLETION

        }

    }
    return NULL;
}


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
             void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  // error (1, 0, "command reading not yet implemented");
    long int ntokens;
    char **tokens = get_tokens(get_next_byte,
        get_next_byte_argument, &ntokens);

    struct stack op;
    init_stack(&op);

    struct stack cmd;
    init_stack(&cmd);

    /*
    populate_stacks(tokens, ntokens, &op, &cmd);
    
    char hello[] = "helloes\n";
    command_t c = (command_t)malloc(sizeof(struct command));
    c->input = hello;

    push(&op, c);
    command_t d = pop(&op);
    printf("%s", d->input);
    */

    
    long int j;
    for(j = 0; j < ntokens; j++)
    {
        printf("%lu: %s\n", j, tokens[j]);
    }
    

  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  // error (1, 0, "command reading not yet implemented");
  return 0;
}
