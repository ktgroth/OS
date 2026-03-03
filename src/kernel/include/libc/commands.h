#ifndef __LIBC_COMMANDS
#define __LIBC_COMMANDS

typedef void (*cmd_fn)(int argc, char **argv);

typedef struct {
    const char *name;
    cmd_fn fn;
    const char *help;
} command_t;


void user_input(char *input);

#endif

