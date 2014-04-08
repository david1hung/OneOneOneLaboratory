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
        CLOSE_SUBSHELL_STATE, MULTI_NEWLINE_STATE, NULL_STATE
} state;

struct command_stream {
    struct node *head;
} stream;

struct node {
    command_t command;
    struct node *next;
};

static
void init_queue(struct command_stream *q)
{
    q->head = NULL;
}

static
void enqueue(struct command_stream *q, command_t c)
{
    if(q == NULL)
        return;

    struct node *n = (struct node *)malloc(sizeof(struct node));
    n->command = c;
    n->next = NULL;
    
    if(q->head == NULL)
    {
        q->head = n;
        return;
    }

    struct node *i = q->head;
    while(i->next != NULL)
    {
        i=i->next;
    }

    i->next = n;
}

static
command_t dequeue(struct command_stream *q)
{
    if(q == NULL)
        return;

    if(q->head == NULL)
        return NULL;

    struct node *n = q->head;
    q->head = q->head->next;

    command_t c = n->command;
    free(n);
    return c;
}


struct stack {
    struct node *head;
};

void debug_tokens(char **tokens, long int ntokens);

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

    printf("PRINTING STACK\n",count);

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
            for(i = 0; n->command->u.word[i]!= NULL; i++)
                printf("        %s\n",n->command->u.word[i]);
            
        }
        else
        {
            printf("    Type: %u.\n",n->command->type);
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
void get_extended_state(state *s, char *token)
{
    long int len = get_token_length(token);
    get_state(s, token[0]);

    if(*s == NEWLINE_STATE)
        if(len > 1)
            *s = MULTI_NEWLINE_STATE;

    if(*s == OR_STATE)
        if(len == 1)
            *s = PIPE_STATE;
}

// Returns 0-3 depending on token. Returns -1 if error.
static
int get_precedence_level(enum command_type t)
{
    switch(t)
    {
        case SEQUENCE_COMMAND:
            return 0;
            break;
        case AND_COMMAND:
        case OR_COMMAND:
            return 1;
            break;
        case PIPE_COMMAND:
            return 2;
            break;
        case SUBSHELL_COMMAND:
            return 3;
            break;
    }

    return -1;
}

static
enum command_type state_to_command(state s)
{
    switch(s)
    {
        case AND_STATE:
            return AND_COMMAND;
            break;
        case SEQUENCE_STATE:
        case NEWLINE_STATE:
            return SEQUENCE_COMMAND;
            break;
        case OR_STATE:
            return OR_COMMAND;
            break;
        case PIPE_STATE:
            return PIPE_COMMAND;
            break;
        case SIMPLE_STATE:
            return SIMPLE_COMMAND;
            break;
        case OPEN_SUBSHELL_STATE:
            return SUBSHELL_COMMAND;
            break;
        default:
            return -1;
            break;
    }
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

    bool encountered_comment = false;
    while((c = get_next_byte(get_next_byte_argument)) > 0)
    {   
        if(c == '#')
        {
            encountered_comment = true;
            continue;
        }

        if(encountered_comment)
        {
            if(c == '\n')
                encountered_comment = false;
            else
                continue;
        }

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
            buf[buf_position] = NULL;
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
        if(s > 0 && s != CLOSE_SUBSHELL_STATE && s != NEWLINE_STATE)
        {
            fprintf(stderr, "%lu: syntax error\n", line_number);
            exit(1);
        }

        buf_position++;

        char **tmp = (char **)malloc(sizeof(char *)*buf_position);
        long int j;
        for(j = 0; j < buf_position-1; j++)
            tmp[j] = buf[j];
        
        tmp[j] = NULL;

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

// Here, let line_number be the line number that this line begins on.
//  This will work so long as you always cycle through the lines
//  sequentially. This allows us to abstract the stack-populating process
//  to individual lines.

static
command_t generate_command_tree(char **line, long int *line_number)
{
    struct stack* op = (struct stack *)malloc(sizeof(struct stack));
    init_stack(op);

    struct stack* cmd = (struct stack *)malloc(sizeof(struct stack));
    init_stack(cmd);

    command_t c;

    bool processing_simple_command = false;
    long int word_size = 10;
    long int word_position = 0;

    state s = NULL_STATE;
    long int len = 0;

    long int i;
    for(i = 0; line[i] != NULL; i++)
    {
        get_extended_state(&s, line[i]);

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

                    if(line[i] != NULL)
                    {
                        get_extended_state(&s, line[i]);
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

                    if(line[i] != NULL)
                    {
                        get_extended_state(&s, line[i]);

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
            case MULTI_NEWLINE_STATE: // Must be preceded by an operand
            {
                free(line[i]);
                continue;
            } break;
            case NEWLINE_STATE:
            {
                if(!processing_simple_command || line[i+1] == NULL)
                {
                    free(line[i]);
                    continue;
                }
            }
            default:
            {
                // Start with SIMPLE_COMMAND completion logic
                if(!processing_simple_command && s != OPEN_SUBSHELL_STATE)
                {
                    printf("(7) broke here: %i\n", s);
                    fprintf(stderr, "7. Syntax error.\n");
                    exit(1);
                }

                if(word_position > 0)
                {
                    if(word_size == word_position)
                    {
                        word_size *= 2;
                        c->u.word = (char **)realloc((void *)c->u.word,
                            sizeof(char *)*word_size);
                    }

                    c->u.word[word_position] = NULL;
                    word_position = 0;
                }


                if(s == CLOSE_SUBSHELL_STATE)
                {
                    while(top(op) != NULL && top(op)->type != SUBSHELL_COMMAND)
                    {
                        command_t tmp = pop(op);
                        command_t cmd_b = pop(cmd);
                        command_t cmd_a = pop(cmd);
                        tmp->u.command[0] = cmd_a;
                        tmp->u.command[1] = cmd_b;
                        push(cmd, tmp);
                    }

                    if(top(op) == NULL)
                    {
                        fprintf(stderr, "Missing '('\n");
                        exit(1);
                    }

                    if(top(op)->type == SUBSHELL_COMMAND)
                    {
                        command_t tmp = pop(op);
                        command_t subshell = pop(cmd);
                        tmp->u.subshell_command = subshell;
                        push(cmd, tmp);
                        continue;
                    }
                }

                processing_simple_command = false;

                // Then create the relevant command struct
                c = (command_t)malloc(sizeof(struct command));
                c->type = state_to_command(s);
                c->input = NULL;
                c->output = NULL;
                free(line[i]);

                int precedence = get_precedence_level(state_to_command(s));

                while(top(op) != NULL)
                {
                    if(precedence > get_precedence_level(top(op)->type) ||
                            top(op)->type == SUBSHELL_COMMAND)
                        break;

                    command_t tmp = pop(op);
                    command_t cmd_b = pop(cmd);
                    command_t cmd_a = pop(cmd);
                    tmp->u.command[0] = cmd_a;
                    tmp->u.command[1] = cmd_b;
                    push(cmd, tmp);

                }
                push(op, c);

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

        c->u.word[word_position] = NULL;
    }

    // parse_stack(cmd);
    // parse_stack(op);

    while(top(op) != NULL)
    {
        command_t tmp = pop(op);
        command_t cmd_b = pop(cmd);
        command_t cmd_a = pop(cmd);
        tmp->u.command[0] = cmd_a;
        tmp->u.command[1] = cmd_b;
        push(cmd, tmp);
    }

    command_t tmp = pop(cmd);

    free(op);
    free(cmd);

    return tmp;
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

    // debug_tokens(tokens, ntokens);

    long int nlines;
    char ***lines = split_command_lines(tokens, ntokens, &nlines);
    // free(tokens);

    
    // debug_lines(lines, nlines);


    long int line_number = 1;

    command_stream_t cstream =
        (command_stream_t)malloc(sizeof(struct command_stream));
    init_queue(cstream);

    long int j;
    for(j = 0; j < nlines; j++)
    {
        enqueue(cstream, generate_command_tree(lines[j], &line_number));
    }

    // free(lines);

    return cstream;
}

command_t
read_command_stream (command_stream_t s)
{
    return dequeue(s);
    // return 0;
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
        for(k = 0; lines[j][k] != NULL; k++)
        {
            printf("(%lu, %lu): %s\n", j, k, lines[j][k]);
            long int l;
            for(l = 0; lines[j][k] != NULL; l++)
                necessary_counter++;
            necessary_counter+=9;
        }
        necessary_counter+=9;
    }

    printf("Allocations counter (bytes): %lu\n",necessary_counter);

    return necessary_counter;
}
