
#include "../include/driver/storage.h"
#include "../include/libc/memory.h"

extern directory_t *root;
extern directory_t *cwd;
static directory_t *home;


char *getcwd() {
    char *cwd = kmalloc(12 * sizeof(char));
}

