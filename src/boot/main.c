
#include <efi.h>
#include <efilib.h>

#include "../boot_info.h"


#define BOOT_INFO_MAGIC     0x424F4F54494E464FULL
#define BOOT_INFO_VERSION   1U

#define EI_NIDENT           16
#define PT_LOAD             1

#define ELFCLASS64          2
#define ELFDATA2LSB         1
#define EV_CURRENT          1
#define ET_EXEC             2
#define EM_X86_64           62

typedef struct {
    UINT8   e_ident[EI_NIDENT];
    UINT16  e_type;
    UINT16  e_machine;
    UINT32  e_version;
    UINT64  e_entry;
    UINT64  e_phoff;
    UINT64  e_shoff;
    UINT32  e_flags;
    UINT16  e_ehsize;
    UINT16  e_phentsize;
    UINT16  e_phnum;
    UINT16  e_shentsize;
    UINT16  e_shnum;
    UINT16  e_shstrndx;
} elf64_ehdr_t;

typedef struct {
    UINT32 p_type;
    UINT32 p_flags;
    UINT64 p_offset;
    UINT64 p_vaddr;
    UINT64 p_paddr;
    UINT64 p_filesz;
    UINT64 p_memsz;
    UINT64 p_align;
} elf64_phdr_t;


static VOID halt_forever(VOID) {
    for (;;)
        __asm__ __volatile("hlt");
}

static VOID copy_bytes(VOID *dst, const VOID *src, UINTN size) {
    UINT8 *d = (UINT8 *)dst;
    const UINT8 *s = (const UINT8 *)src;
    UINTN i;

    for (i = 0; i < size; ++i)
        d[i] = s[i];
}

static VOID zero_bytes(VOID *dst, UINTN size) {
    UINT8 *d = (UINT8 *)dst;
    UINTN i;

    for (i = 0; i < size; ++i)
        d[i] = 0;
}

static BOOLEAN guid_equal(const EFI_GUID *a, const EFI_GUID *b) {
    const UINT8 *pa = (const UINT8 *)a;
    const UINT8 *pb = (const UINT8 *)b;
    UINTN i;

    for (i = 0; i < sizeof(EFI_GUID); ++i)
        if (pa[i] != pb[i])
            return FALSE;

    return TRUE;
}

static EFI_STATUS open_kernel_file(EFI_HANDLE image_handle, EFI_FILE_HANDLE *out_file) {
    EFI_STATUS status;
    EFI_LOADED_IMAGE *loaded_image = NULL;
    EFI_FILE_HANDLE root;
    EFI_FILE_HANDLE file;
    CHAR16 *paths[] = {
        L"\\kernel.elf",
        L"\\EFI\\BOOT\\kernel.elf"
    };
    UINTN i;

    status = uefi_call_wrapper(BS->HandleProtocol, 3,
                               image_handle,
                               &LoadedImageProtocol,
                               (VOID **)&loaded_image);
    if (EFI_ERROR(status))
        return status;

    root = LibOpenRoot(loaded_image->DeviceHandle);
    if (root == NULL)
        return EFI_NOT_FOUND;

    for (i = 0; i < (sizeof(paths) / sizeof(paths[0])); ++i) {
        status = uefi_call_wrapper(root->Open, 5,
                                   root,
                                   &file,
                                   paths[i],
                                   EFI_FILE_MODE_READ,
                                   0);
        if (!EFI_ERROR(status)) {
            uefi_call_wrapper(root->Close, 1, root);
            *out_file = file;
            return EFI_SUCCESS;
        }
    }

    uefi_call_wrapper(root->Close, 1, root);
    return EFI_NOT_FOUND;
}

static EFI_STATUS read_entire_file(EFI_FILE_HANDLE file, VOID **out_buf, UINTN *out_size) {
    EFI_STATUS status;
    EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
    EFI_FILE_INFO *info;
    UINTN info_size;
    UINTN read_size;
    VOID *buf;

    info_size = SIZE_OF_EFI_FILE_INFO + 256;
    info = AllocatePool(info_size);
    if (info == NULL)
        return EFI_OUT_OF_RESOURCES;

    status = uefi_call_wrapper(file->GetInfo, 4,
                               file,
                               &file_info_guid,
                               &info_size,
                               info);
    if (status == EFI_BUFFER_TOO_SMALL) {
        FreePool(info);
        info = AllocatePool(info_size);
        if (info == NULL)
            return EFI_OUT_OF_RESOURCES;

        status = uefi_call_wrapper(file->GetInfo, 4,
                                   file,
                                   &file_info_guid,
                                   &info_size,
                                   info);
    }

    if (EFI_ERROR(status)) {
        FreePool(info);
        return status;
    }

    *out_size = (UINTN)info->FileSize;
    FreePool(info);
    if (*out_size == 0)
        return EFI_LOAD_ERROR;

    buf = AllocatePool(*out_size);
    if (buf == NULL)
        return EFI_OUT_OF_RESOURCES;

    read_size = *out_size;
    status = uefi_call_wrapper(file->Read, 3, file, &read_size, buf);
    if (EFI_ERROR(status) || read_size != *out_size) {
        FreePool(buf);
        return EFI_LOAD_ERROR;
    }

    *out_buf = buf;
    return EFI_SUCCESS;
}

static BOOLEAN validate_elf64(const VOID *image, UINTN size) {
    const elf64_ehdr_t *eh = (const elf64_ehdr_t *)image;
    UINT64 ph_end;
    
    if (size < sizeof(elf64_ehdr_t))
        return FALSE;

    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' ||
        eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F')
        return FALSE;

    if (eh->e_ident[4] != ELFCLASS64 || eh->e_ident[5] != ELFDATA2LSB ||
        eh->e_ident[6] != EV_CURRENT)
        return FALSE;

    if (eh->e_machine != EM_X86_64 || eh->e_type != ET_EXEC)
        return FALSE;

    if (eh->e_phentsize != sizeof(elf64_phdr_t) || eh->e_phnum == 0 || eh->e_phoff == 0)
        return FALSE;

    ph_end = eh->e_phoff + ((UINT64)eh->e_phnum * (UINT64)eh->e_phentsize);
    if (ph_end > (UINT64)size)
        return FALSE;

    return TRUE;
}

static EFI_STATUS load_kernel_elf(const VOID *image,
                                  UINTN image_size,
                                  EFI_PHYSICAL_ADDRESS *out_entry,
                                  EFI_PHYSICAL_ADDRESS *out_base,
                                  UINT64 *out_size) {
    const elf64_ehdr_t *eh = (const elf64_ehdr_t *)image;
    const elf64_phdr_t *ph = (const elf64_phdr_t *)((const UINT8 *)image + eh->e_phoff);

    UINT64 min_addr = ~0ULL;
    UINT64 max_addr = 0;
    UINT64 aligned_min;
    UINT64 aligned_max;
    UINTN pages;
    EFI_PHYSICAL_ADDRESS alloc_addr;
    EFI_STATUS status;
    UINTN i;

    for (i = 0; i < eh->e_phnum; ++i) {
        UINT64 seg_start;
        UINT64 seg_end;
        UINT64 file_end;

        if (ph[i].p_type != PT_LOAD || ph[i].p_memsz == 0)
            continue;

        file_end = ph[i].p_offset + ph[i].p_filesz;
        if (file_end > (UINT64)image_size || ph[i].p_filesz > ph[i].p_memsz)
            return EFI_LOAD_ERROR;

        seg_start = (ph[i].p_paddr != 0) ? ph[i].p_paddr : ph[i].p_vaddr;
        seg_end = seg_start + ph[i].p_memsz;

        if (seg_start < min_addr)
            min_addr = seg_start;
        if (seg_end > max_addr)
            max_addr = seg_end;
    }

    if (min_addr == ~0ULL || max_addr <= min_addr)
        return EFI_LOAD_ERROR;

    aligned_min = min_addr & ~(UINT64)0xFFF;
    aligned_max = (max_addr + 0xFFF) & ~(UINT64)0xFFF;
    pages = (UINTN)((aligned_max - aligned_min) / 0x1000);

    alloc_addr = (EFI_PHYSICAL_ADDRESS)aligned_min;
    status = uefi_call_wrapper(BS->AllocatePages, 4,
                               AllocateAddress,
                               EfiLoaderData,
                               pages,
                               &alloc_addr);
    if (EFI_ERROR(status))
        return status;

    zero_bytes((VOID *)(UINTN)aligned_min, (UINTN)(aligned_max - aligned_min));
    for (i = 0; i < eh->e_phnum; ++i) {
        UINT64 seg_start;

        if (ph[i].p_type != PT_LOAD || ph[i].p_memsz == 0)
            continue;

        seg_start = (ph[i].p_paddr != 0) ? ph[i].p_paddr : ph[i].p_vaddr;
        copy_bytes((VOID *)(UINTN)seg_start,
                   (const VOID *)((const UINT8 *)image + ph[i].p_offset),
                   (UINTN)ph[i].p_filesz);

        if (ph[i].p_memsz > ph[i].p_filesz)
            zero_bytes((VOID *)(UINTN)(seg_start + ph[i].p_filesz),
                       (UINTN)(ph[i].p_memsz - ph[i].p_filesz));
    }

    *out_entry = (EFI_PHYSICAL_ADDRESS)eh->e_entry;
    *out_base = (EFI_PHYSICAL_ADDRESS)aligned_min;
    *out_size = aligned_max - aligned_min;

    return EFI_SUCCESS;
}

static VOID fill_framebuffer_info(boot_info_t *bi) {
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_STATUS status;

    status = uefi_call_wrapper(BS->LocateProtocol, 3,
                               &gop_guid,
                               NULL,
                               (VOID **)&gop);
    if (EFI_ERROR(status) || gop == NULL || gop->Mode == NULL || gop->Mode->Info == NULL)
        return;

    bi->fb.base = (uint64_t)gop->Mode->FrameBufferBase;
    bi->fb.size = (uint64_t)gop->Mode->FrameBufferSize;
    bi->fb.width = gop->Mode->Info->HorizontalResolution;
    bi->fb.height = gop->Mode->Info->VerticalResolution;
    bi->fb.ppl = gop->Mode->Info->PixelsPerScanLine;
    bi->fb.format = gop->Mode->Info->PixelFormat;
}

static uint64_t find_rsdp(EFI_SYSTEM_TABLE *system_table) {
    EFI_GUID acpi20 = ACPI_20_TABLE_GUID;
    EFI_GUID acpi10 = ACPI_TABLE_GUID;
    UINTN i;

    for (i = 0; i < system_table->NumberOfTableEntries; ++i) {
        EFI_CONFIGURATION_TABLE *t = &system_table->ConfigurationTable[i];
        if (guid_equal(&t->VendorGuid, &acpi20) || guid_equal(&t->VendorGuid, &acpi10))
            return (uint64_t)(UINTN)t->VendorTable;
    }

    return 0;
}

static EFI_STATUS capture_memory_map(memory_map_info_t *mmap, UINTN *map_key_out) {
    EFI_STATUS status;
    EFI_MEMORY_DESCRIPTOR *map;
    UINTN map_size;
    UINTN map_key;
    UINTN desc_size;
    UINT32 desc_ver;

    map_size = 0;
    map_key = 0;
    desc_size = 0;
    desc_ver = 0;

    status = uefi_call_wrapper(BS->GetMemoryMap, 5,
                               &map_size,
                               NULL,
                               &map_key,
                               &desc_size,
                               &desc_ver);
    if (status != EFI_BUFFER_TOO_SMALL)
        return status;

    map_size += (2 * desc_size);
    map = AllocatePool(map_size);
    if (map == NULL)
        return EFI_OUT_OF_RESOURCES;

    status = uefi_call_wrapper(BS->GetMemoryMap, 5,
                               &map_size,
                               map,
                               &map_key,
                               &desc_size,
                               &desc_ver);
    if (EFI_ERROR(status)) {
        FreePool(map);
        return status;
    }

    mmap->ptr = (uint64_t)(UINTN)map;
    mmap->size = (uint64_t)map_size;
    mmap->desc_size = (uint64_t)desc_size;
    mmap->desc_ver = (uint64_t)desc_ver;
    *map_key_out = map_key;

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    EFI_STATUS status;
    EFI_FILE_HANDLE kernel_file;
    VOID *kernel_image;
    UINTN kernel_image_size;
    EFI_PHYSICAL_ADDRESS kernel_entry;
    EFI_PHYSICAL_ADDRESS kernel_base;
    UINT64 kernel_size;
    boot_info_t *bi;
    UINTN map_key;
    UINTN attempt;

    InitializeLib(image_handle, system_table);
    Print(L"UEFI loader: start\r\n");

    status = open_kernel_file(image_handle, &kernel_file);
    if (EFI_ERROR(status)) {
        Print(L"UEFI loader: failed to open kernel.elf: %r\r\n", status);
        return status;
    }

    status = read_entire_file(kernel_file, &kernel_image, &kernel_image_size);
    uefi_call_wrapper(kernel_file->Close, 1, kernel_file);
    if (EFI_ERROR(status)) {
        Print(L"UEFI loader: failed to read kernel.elf %r\r\n", status);
        return status;
    }

    if (!validate_elf64(kernel_image, kernel_image_size)) {
        Print(L"UEFI loader: kernel.elf is not a valid ELF64 ET_EXEC image\r\n");
        FreePool(kernel_image);
        return EFI_LOAD_ERROR;
    }

    status = load_kernel_elf(kernel_image,
                             kernel_image_size,
                             &kernel_entry,
                             &kernel_base,
                             &kernel_size);
    FreePool(kernel_image);
    if (EFI_ERROR(status)) {
        Print(L"UEFI loader: failed to load ELF segments: %r\r\n", status);
        return status;
    }

    bi = AllocateZeroPool(sizeof(*bi));
    if (bi == NULL) {
        Print(L"UEFI loader: boot_info allocation failed\r\n");
        return EFI_OUT_OF_RESOURCES;
    }

    bi->magic = BOOT_INFO_MAGIC;
    bi->version = BOOT_INFO_VERSION;
    bi->kernel_entry = (uint64_t)kernel_entry;
    bi->kernel_base = (uint64_t)kernel_base;
    bi->kernel_size = (uint64_t)kernel_size;
    bi->rsdp = find_rsdp(system_table);
    fill_framebuffer_info(bi);

    for (attempt = 0; attempt < 3; ++attempt) {
        status = capture_memory_map(&bi->mmap, &map_key);
        if (EFI_ERROR(status)) {
            Print(L"UEFI loader: GetMemoryMap failed: %r\r\n", status);
            return status;
        }

        status = uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, map_key);
        if (!EFI_ERROR(status))
            break;

        if (status != EFI_INVALID_PARAMETER) {
            Print(L"UEFI loader: ExitBootServices failed: %r\r\n", status);
            return status;
        }
    }

    if (EFI_ERROR(status))
        return status;

    {
        typedef VOID (*kernel_entry_t)(boot_info_t *);
        kernel_entry_t entry = (kernel_entry_t)(UINTN)kernel_entry;
        entry(bi);
    }

    halt_forever();
    return EFI_SUCCESS;

    for (;;)
        __asm__ __volatile__("hlt");

    return EFI_SUCCESS;
}
