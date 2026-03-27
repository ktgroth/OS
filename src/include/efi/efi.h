#ifndef __EFI_EFI
#define __EFI_EFI

#include "../types.h"

#define EFIAPI      __attribute__((ms_abi))
#define EFI_SUCCESS 0

typedef void *efi_handle;
typedef uint64_t uintn;
typedef uint64_t efi_status;
typedef uint16_t char16;

typedef struct {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
} efi_table_header;

struct efi_simple_text_output_protocol;
typedef efi_status(EFIAPI *efi_text_string)(
    struct efi_simple_text_output_protocol *this,
    char16 *string
);

typedef struct efi_simple_text_output_protocol {
    void *reset;
    efi_text_string output_string;
    void *test_string;
    void *query_mode;
    void *set_mode;
    void *set_attribute;
    void *clear_screen;
    void *set_cursor_position;
    void *enable_cursor;
    void *mode;
} efi_simple_text_output_protocol;

typedef struct {
    efi_table_header hdr;
    char16 *firmware_vendor;
    uint32_t firmware_revision;
    efi_handle console_in_handle;
    void *con_in;
    efi_handle console_out_handle;
    efi_simple_text_output_protocol *con_out;
    efi_handle standard_error_handle;
    efi_simple_text_output_protocol *stderr;
    void *runtime_services;
    void *boot_services;
    uintn number_of_table_entries;
    void *configuration_table;
} efi_system_table;

#endif
