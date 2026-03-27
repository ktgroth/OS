
#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"GNU UEFI loader started.\r\n");
    Print(L"kernel.elf should be at \\\\kernel.elf on ESP.\r\n");

    for (;;)
        __asm__ __volatile__("hlt");

    return EFI_SUCCESS;
}
