
#include "../include/cpu/ports.h"
#include "../include/driver/vga.h"
#include "../include/driver/clock.h"
#include "../include/driver/fat32.h"
#include "../include/driver/storage.h"
#include "../include/libc/printf.h"
#include "../include/libc/commands.h"
#include "../include/libc/memory.h"
#include "../include/libc/string.h"
#include "../include/libc/app.h"
#include "../include/user_mode/process.h"
#include "../include/user_mode/scheduler.h"

#define MAX_INPUT 4096
#define MAX_ARGS 16

static int tokenize(char *s, char *argv[], int max_args) {
    int argc = 0;

    while (*s && argc < max_args) {
        while (*s == ' ') s++;
        if (!*s) break;
        argv[argc++] = s;
        
        while (*s && *s != ' ') s++;
        if (*s) *s++ = '\0';
    }

    return argc;
}

static void cmd_help(int argc, char **argv);
static void cmd_clear(int argc, char **argv);
static void cmd_time(int argc, char **argv);
static void cmd_pwd(int argc, char **argv);
static void cmd_cd(int argc, char **argv);
static void cmd_ls(int argc, char **argv);
static void cmd_alloc(int argc, char **argv);
static void cmd_shutdown(int argc, char **argv);
static void cmd_read(int argc, char **argv);
static void cmd_write(int argc, char **argv);
static void cmd_print(int argc, char **argv);
static void cmd_run(int argc, char **argv);

static command_t commands[] = {
    { "HELP", cmd_help, "List commands" },
    { "CLEAR", cmd_clear, "Clear the screen" },
    { "TIME", cmd_time, "Show RTC time" },
    { "PWD", cmd_pwd, "Print working directory" },
    { "CD" , cmd_cd, "Change directory" },
    { "LS", cmd_ls, "List directory" },
    { "ALLOC", cmd_alloc, "Test allocation" },
    { "SHUTDOWN", cmd_shutdown, "Power off" },
    { "READ", cmd_read, "Read file: READ <PATH> "},
    { "WRITE", cmd_write, "Write file: WRITE <PATH> <DATA>" },
    { "PRINT", cmd_print, "Print memory: PRINT <HEX_ADDR> <WIDTH_BYTES>" },
    { "RUN", cmd_run, "Run file: RUN <PATH> ..ARGS" }
};
static const int command_count = sizeof(commands)/sizeof(commands[0]);


static void dispatch(int argc, char **argv) {
    if (argc == 0) return;

    for (uint64_t i = 0; i < command_count; ++i) {
        if (!strcmp(argv[0], commands[i].name)) {
            commands[i].fn(argc, argv);
            return;
        }
    }

    putstr("Unknown command. Type HELP\n> ", COLOR_WHT, COLOR_BLK);
}

void user_input(char *input) {
    printf("INPUT: %s\n", input);
    char *argv[MAX_ARGS];
    int argc = tokenize(input, argv, MAX_ARGS);
    dispatch(argc, argv);
}

static void cmd_help(int argc, char **argv) {
    for (int64_t i = 0; i < command_count; ++i)
        printf("%s: %s\n", commands[i].name, commands[i].help);

    putstr("> ", COLOR_WHT, COLOR_BLK);
}

static void cmd_clear(int argc, char **argv) {
    set_cursor_pos(0, 0);
    clearwin(COLOR_WHT, COLOR_BLK);
    printf("Type something, it will go through the kernel\n"
           "Type SHUTDOWN to Shutdown CPU\n> ");
}

static void cmd_time(int argc, char **argv) {
    print_current_time();
    putstr("> ", COLOR_WHT, COLOR_BLK);
}

static void cmd_pwd(int argc, char **argv) {
    putstr("> ", COLOR_WHT, COLOR_BLK);

}

static void cmd_cd(int argc, char **argv) {
    putstr("> ", COLOR_WHT, COLOR_BLK);
}

static void cmd_ls(int argc, char **argv) {
    putstr("> ", COLOR_WHT, COLOR_BLK);

}

static void cmd_alloc(int argc, char **argv) {
    mblock_t *block = kmalloc(0x1000);
    void *vaddr = &block->addr;
    void *paddr = (uint64_t *)get_paddr(vaddr);
    
    printf("PADDR: %x\nVADDR: %x\n> ", paddr, vaddr);
}

static void cmd_shutdown(int argc, char **argv) {
    outw(0x604, 0x2000);
}

static void cmd_read(int argc, char **argv) {
    putstr("> ", COLOR_WHT, COLOR_BLK);

}

static void cmd_write(int argc, char **argv) {
    putstr("> ", COLOR_WHT, COLOR_BLK);

}


#define APP_LOAD_MIN    ((uint8_t *)0x200000)
#define APP_ALIGN       0x1000
#define APP_FILE_MAX    (0x400 * 0x400)

static uint8_t *next_app_addr = APP_LOAD_MIN;


static int to_name(const char *in, char out[11]) {
    uint64_t i = 0, j = 0;
    for (i = 0; i < 11; ++i)
        out[i] = ' ';

    i = 0;
    while (in[i] && in[i] != '.' && j < 8) {
        char c = in[i++];
        if (c >= 'a' && c <= 'z') c -= 32;
        out[j++] = c;
    }

    if (in[i] == '.') ++i;
    j = 8;
    while (in[i] && j < 11) {
        char c = in[i++];
        if (c >= 'a' && c <= 'z') c -= 32;
        out[j++] = c;
    }

    return 1;
}


static int parse_hex_u64(const char *s, uint64_t *out) {
    if (!s || !*s || !out)
        return 0;

    uint64_t i = 0;
    uint64_t v = 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        i = 2;

    if (!s[i])
        return 0;

    for (; s[i]; ++i) {
        char c = s[i];
        uint8_t d;
        if (c >= '0' && c <= '9')
            d = (uint8_t)(c - '0');
        else if (c >= 'A' && c <= 'F')
            d = (uint8_t)(c - 'A' + 10);
        else
            return 0;
    
        v = (v << 4) | d;
    }

    *out = v;
    return 1;
}

static void cmd_print(int argc, char **argv) {
    if (argc < 3) {
        putstr("Usage: PRINT <HEX_ADDR> <WIDTH_BYTES>\n> ", COLOR_WHT, COLOR_BLK);
        return;
    }

    uint64_t addr = 0;
    if (!parse_hex_u64(argv[1], &addr)) {
        putstr("PRINT: invalid hex address\n> ", COLOR_WHT, COLOR_BLK);
        return;
    }

    uint64_t width = ascii_to_int(argv[2]);
    if (width == 0 || width > 256) {
        putstr("PRINT: width must be 1..256\n> ", COLOR_WHT, COLOR_BLK);
        return;
    }

    for (uint64_t i = 0; i < width; ++i) {
        __volatile__ uint8_t *p = (__volatile__ uint8_t *)(addr + i);
        if (!get_paddr((void *)p)) {
            printf("PRINT: unmapped at %p\n> ", (void *)p);
            return;
        }

        if ((i % 16) == 0)
            printf("\n%p: ", (void *)p);

        printf("%02x ", (uint64_t)(*p));
    }

    putstr("\n\n> ", COLOR_WHT, COLOR_BLK);
}

static uint8_t *align_up_ptr(uint8_t *p, uint64_t align) {
    uint64_t v = (uint64_t)p;
    v = (v + align - 1) & ~(align - 1);
    return (uint8_t *)v;
}

static uint8_t *alloc_app_region(uint64_t total_size) {
    mblock_t *block = kmalloc(total_size); 
    uint8_t *base = (uint8_t *)&block->addr;
    uint8_t *end = base + block->size;

    next_app_addr = end;
    return base;
}


static void cmd_run(int argc, char **argv) {
    if (argc < 2) {
        putstr("Usage: RUN <NAME.EXT>\n> ", COLOR_WHT, COLOR_BLK);
        return;
    }
   
    uint64_t queued = 0;

    for (uint64_t i = 1; i < argc; ++i) {
        char name[11];
        directory_t ent;

        to_name(argv[i], name);

        if (!find_root_entry(name, &ent)) {
            putstr("RUN: file not found\n> ", COLOR_WHT, COLOR_BLK);
            return;
        }

        if (ent.flags & DIRECTORY) {
            putstr("RUN: is directory\n> ", COLOR_WHT, COLOR_BLK);
            return;
        }

        if (ent.bytes == 0 || ent.bytes > APP_FILE_MAX) {
            putstr("RUN: bad file size\n> ", COLOR_WHT, COLOR_BLK);
            return;
        }

        uint8_t *file_base = alloc_app_region(ent.bytes);
        if (!file_base) {
            putstr("RUN: out of app memory\n> ", COLOR_WHT, COLOR_BLK);
            return;
        }

        uint32_t start_cluster = ((uint32_t)ent.fc_hi << 16) | ent.fc_lo;
        if (!read_file_chain(start_cluster, file_base, ent.bytes)) {
            putstr("RUN: read failed\n> ", COLOR_WHT, COLOR_BLK);
            return;
        }

        app_header_t *hdr = (app_header_t *)file_base;
        if (hdr->magic != APP_MAGIC || hdr->version != APP_VERSION) {
            putstr("RUN: bad app header\n> ", COLOR_WHT, COLOR_BLK);
            return;
        } if (!(hdr->flags & APP_FLAG_PIE)) {
            putstr("RUN: app not PIE\n> ", COLOR_WHT, COLOR_BLK);
            return;
        }

        uint8_t *payload = file_base + sizeof(app_header_t);
        if (hdr->image_size != (uint64_t)(ent.bytes - sizeof(app_header_t))) {
            printf("Expected: %lu\nRead: %lu\n", ent.bytes - sizeof(app_header_t), hdr->image_size);
            putstr("RUN: size mismatch\n> ", COLOR_WHT, COLOR_BLK);
            return;
        } if (hdr->entry_off >= hdr->image_size) {
            putstr("RUN: bad entry\n> ", COLOR_WHT, COLOR_BLK);
            return;
        }

        process_t *p = create_user_process_from_image(
            payload,
            hdr->image_size,
            hdr->entry_off,
            argv[i]
        );

        if (!p) {
            printf("RUN: create process failed: %s\n", argv[i]);
            continue;
        }
        
        scheduler_enqueue(p);
        printf("RUN: queued pid=%lu %s\n", p->pid, argv[i]);
        printf("rip=%lx rsp=%lx\n", p->regs.rip, p->regs.rsp);
        queued++;
    }

    printf("RUN: queued %lu process(es)\n> ", queued);
    scheduler_start_if_idle();
}

