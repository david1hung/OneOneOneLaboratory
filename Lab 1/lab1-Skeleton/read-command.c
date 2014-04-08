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
    SIMPLE_STATE, INPUT_STATE, OUTPUT_STATE, NEWLINE_STATE, SEQUENCE_STATE,
        AND_STATE, OR_STATE, PIPE_STATE, OPEN_SUBSHELL_STATE,
        CLOSE_SUBSHELL_STATE, MULTI_NEWLINE_STATE, COMMENT_STATE, NULL_STATE
} state;

struct command_stream {
    command_t head;
} stream;

struct node {
    command_t command;
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
    if(s == NULL)
        return;

    struct node *n = (struct node *)malloc(sizeof(struct node));
    n->command = c;
    n->next = s->head;
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
command_t top(struct stack *s)
{
    if(s->head == NULL)
        return NULL;

    return s->head->command;
}

static
void parse_stack(struct stack *s)
{
    if(s->head == NULL)
    {
        fprintf(stderr, "Parse failed: empty stack.\n");
        return;
    }

    struct node *n = s->head;
    long int count = 0;

    while(n != NULL)
    {
        printf("Node %lu\n",count);
        if(n->command->type == SIMPLE_COMMAND)
        {
            if(n->command->input != NULL)
                printf("    Input: %s\n",n->command->input);
            if(n->command->output != NULL)
                printf("    Output: %s\n",n->command->output);

            printf("    Command and arguments are: \n",n->command->input);

            long int i;
            for(i = 0; n->command->u.word[i][0] != '\0'; i++)
                printf("        %s\n",n->command->u.word[i]);
            
        }
        else
        {
            printf("    Hello! I am COMMAND of type %u.\n",n->command->type);
        }
        count++;
        n = n->next;
    }
}

static
bool needs_splitting(state *s, char c)
{
    if(s == NULL)
        return false;

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

static
void get_state(state *s, char c)
{
    if(s == NULL)
        return;

    state s_old = *s;
    needs_splitting(s, c);
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
    state s = NULL_STATE;
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
                buf_position++;
                char *tmp = (char *)malloc(sizeof(char)*buf_position);
                long int i;
                for(i = 0; i < buf_position-1; i++)
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
        buf_position++;
        char *tmp = (char *)malloc(sizeof(char)*buf_position);
        long int i;
        for(i = 0; i < buf_position-1; i++)
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

    free(buf);
    *ntokens = tokens_position;
    return tokens;
}


/* NOTE: each line ends with an '\0' rather than '\n\n\0' */
static
char ***split_command_lines(char **tokens, long int ntokens, long int *nlines)
{
    long int lines_position = 0;
    long int lines_size = 64;
    char ***lines = (char ***)malloc(sizeof(char **)*lines_size);

    long int buf_position = 0;
    long int buf_size = 128;
    char **buf = (char **)malloc(sizeof(char *)*buf_size);

    state s = NULL_STATE;
    long int l;

    long int line_number = 1;

    long int i;
    for(i = 0; i < ntokens; i++)
    {
        if(buf_position == buf_size)
        {
            buf_size *= 2;
            buf = (char **)realloc((void *)buf,
                sizeof(char *)*buf_size);
        }
      
        get_state(&s, tokens[i][0]);
        l = get_token_length(tokens[i]);
        
        if(s == NEWLINE_STATE)
        {
            line_number += l;
        }


        if(s == NEWLINE_STATE && l > 1 && i > 0)
        {
            state s_prev = NULL_STATE;
            get_state(&s_prev, tokens[i-1][0]);

            if(s_prev/3 > 0)
            {
                buf[buf_position] = tokens[i];
                buf_position++;
                continue;
            }

            free(tokens[i]);
            buf[buf_position] = (char *)malloc(sizeof(char));
            buf[buf_position][0] = '\0';
            buf_position++;

            char **tmp = (char **)malloc(sizeof(char *)*buf_position);
            long int j;
            for(j = 0; j < buf_position; j++)
                tmp[j] = buf[j];

            if(lines_position == lines_size)
            {
                lines_size *= 2;
                lines = (char ***)realloc((void *)lines,
                    sizeof(char **)*lines_size);
            }

            lines[lines_position] = tmp;
            lines_position++;

            buf_position = 0;
        }
        else
        {
            buf[buf_position] = tokens[i];
            buf_position++;
        }
    }

    if(buf_position > 0)
    {
        if(s > 0)
        {
            fprintf(stderr, "%lu: syntax error\n", line_number);
            exit(1);
        }

        buf_position++;

        char **tmp = (char **)malloc(sizeof(char *)*buf_position);
        long int j;
        for(j = 0; j < buf_position-1; j++)
            tmp[j] = buf[j];
        
        tmp[j] = (char *)malloc(sizeof(char));
        tmp[j][0] = '\0';

        if(lines_position == lines_size)
        {
            lines_size *= 2;
            lines = (char ***)realloc((void *)lines,
                sizeof(char **)*lines_size);
        }

        lines[lines_position] = tmp;
        lines_position++;
    }

    free(buf);
    *nlines = lines_position;
    return lines;
}

/*  SIMPLE_STATE, INPUT_STATE, OUTPUT_STATE, AND_STATE, OR_STATE,
    OPEN_SUBSHELL_STATE, CLOSE_SUBSHELL_STATE, SEQUENCE_STATE,
    NEWLINE_STATE, COMMENT_STATE, NULL_STATE */


// Here, let line_number be the line number that this line begins on.
//  This will work so long as you always cycle through the lines
//  sequentially. This allows us to abstract the stack-populating process
//  to individual lines.

static
void populate_stacks(char **line, long int *line_number, struct stack *op,
                        struct stack *cmd)
{
    if(op == NULL)
    {
        op = (struct stack *)malloc(sizeof(struct stack));
        init_stack(op);
    }

    if(cmd == NULL)
    {
        cmd = (struct stack *)malloc(sizeof(struct stack));
        init_stack(cmd);
    }

    command_t c;

    bool processing_simple_command = false;
    long int word_size = 10;
    long int word_position = 0;

    state s = NULL_STATE;
    long int len = 0;

    long int i;
    for(i = 0; line[i][0] != '\0'; i++)
    {
        get_state(&s, line[i][0]);

        switch(s)
        {
            case SIMPLE_STATE:
            {
                if(!processing_simple_command)
                {
                    c = (command_t)malloc(sizeof(struct command));
                    c->input = NULL;
                    c->output = NULL;
                    c->type = SIMPLE_COMMAND;
                    c->u.word = (char **)malloc(sizeof(char *)*word_size);
                    
                    c->u.word[word_position] = line[i];
                    word_position++;

                    push(cmd, c);
                    processing_simple_command = true;

                }
                else
                {
                    c = pop(cmd);

                    if(word_size == word_position)
                    {
                        word_size *= 2;
                        c->u.word = (char **)realloc((void *)c->u.word,
                            sizeof(char *)*word_size);
                    }

                    c->u.word[word_position] = line[i];
                    push(cmd, c);

                    word_position++;
                }
            } break;
            case INPUT_STATE:
            {
                if(!processing_simple_command)
                {
                    fprintf(stderr, "1. Syntax error.\n");
                    exit(1);
                }
                else
                {
                    free(line[i]);
                    i++;

                    // I could add '\0' to NULL_STATE but I want to make
                    //  it explicit that this is the end-of-line token.

                    if(line[i][0] != '\0')
                    {
                        get_state(&s, line[i][0]);
                        if(s == SIMPLE_STATE)
                        {
                            c = pop(cmd);
                            c->input = line[i];
                            push(cmd, c);
                        }
                        else
                        {
                            fprintf(stderr, "2. Syntax error.\n");
                            exit(1);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "3. Syntax error.\n");
                        exit(1);
                    }
                }
            } break;
            case OUTPUT_STATE:
            {
                if(!processing_simple_command)
                {
                    fprintf(stderr, "4. Syntax error.\n");
                    exit(1);
                }
                else
                {
                    free(line[i]);
                    i++;

                    // I could add '\0' to NULL_STATE but I want to make
                    //  it explicit that this is the end-of-line token.

                    if(line[i][0] != '\0')
                    {
                        get_state(&s, line[i][0]);

                        if(s == SIMPLE_STATE)
                        {
                            c = pop(cmd);
                            c->output = line[i];
                            push(cmd, c);
                        }
                        else
                        {
                            fprintf(stderr, "5. Syntax error.\n");
                            exit(1);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "6. Syntax error.\n");
                        exit(1);
                    }
                }
            } break;
            case NEWLINE_STATE:
            case SEQUENCE_STATE:
            {
                len = get_token_length(line[i]);
                if(len == 1)
                {
                    if(!processing_simple_command)
                    {
                        fprintf(stderr, "7. Syntax error.\n");
                        exit(1);
                    }
                    
                    processing_simple_command = false;

                    if(word_size == word_position)
                    {
                        word_size *= 2;
                        c->u.word = (char **)realloc((void *)c->u.word,
                            sizeof(char *)*word_size);
                    }

                    c->u.word[word_position] = (char *)malloc(sizeof(char));
                    c->u.word[word_position][0] = '\0';
                    word_position = 0;


                    c = (command_t)malloc(sizeof(struct command));
                    c->type = SEQUENCE_COMMAND;
                    c->input = NULL;
                    c->output = NULL;

                    free(line[i]);
                    push(op, c);
                }
                else if(len > 1)
                {
                    free(line[i]);
                    continue;
                }
            } break;

        }
    }

    if(processing_simple_command)
    {
        if(word_size == word_position)
        {
            word_size *= 2;
            c->u.word = (char **)realloc((void *)c->u.word,
                sizeof(char *)*word_size);
        }

        c->u.word[word_position] = (char *)malloc(sizeof(char));
        c->u.word[word_position][0] = '\0';
    }

}


static
command_t *generate_command_tree(struct stack *op, struct stack *cmd)
{
    state s = NULL_STATE;
    command_t c = malloc(sizeof(struct command));
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

    long int nlines;
    char ***lines = split_command_lines(tokens, ntokens, &nlines);
    free(tokens);

    debug_lines(lines, nlines);

    struct stack op;
    init_stack(&op);

    struct stack cmd;
    init_stack(&cmd);

    long int line_number = 1;

    long int j;
    for(j = 0; j < nlines; j++)
    {

        populate_stacks(lines[j], &line_number, &op, &cmd);

        printf("Command line %lu\n", j);
    }

    parse_stack(&cmd);
    parse_stack(&op);

    return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  // error (1, 0, "command reading not yet implemented");
  return 0;
}

// DEBUG FUNCTIONS
void debug_tokens(char **tokens, long int ntokens)
{
    long int j;
    for(j = 0; j < ntokens; j++)
    {
        printf("%lu: %s\n", j, tokens[j]);
    }
}

int debug_lines(char ***lines, long int nlines)
{
    long int j;
    
    long int necessary_counter = 0;
    
    // split_command_lines debug
    for(j = 0; j < nlines; j++)
    {
        long int k;
        state s;
        for(k = 0; lines[j][k][0] != '\0'; k++)
        {
            printf("(%lu, %lu): %s\n", j, k, lines[j][k]);
            long int l;
            for(l = 0; lines[j][k][l] != '\0'; l++)
                necessary_counter++;
            necessary_counter+=9;
        }
        necessary_counter+=9;
    }

    printf("Allocations counter (bytes): %lu\n",necessary_counter);

    return necessary_counter;
}
