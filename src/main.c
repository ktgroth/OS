
#include "include/efi/efi.h"

efi_status EFIAPI efi_main(efi_handle image_handle, efi_system_table *system_table) {
    (void)image_handle;

    if (!system_table || !system_table->con_out || !system_table->con_out->output_string)
        return 1;

    system_table->con_out->output_string(
        system_table->con_out,
        L"UEFI x86_64 boot works.\r\n"
    );

    for (;;)
        __asm__ __volatile__("hlt");

    return EFI_SUCCESS;
}
