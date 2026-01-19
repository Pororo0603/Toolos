/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. 
 * Copyright (c) 2026, "뿌댕이"
 * Project: ToolOS
 * File: ToolosBootLoader.c
 * * Description: 
 * Toolos 0.1.0 Bootloader. 
 * This bootloader operates in the following order: 
 * 1. variable setting 
 * 2. information extraction 
 * 3. information verification 
 * 4. kernel loading, and cleanup
 * 5. all through structured abstraction.
 * 
 * A detailed description of the code will be added in version 0.1.1.
 */

#include <Uefi.h> 
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h> 
#include <Protocol/SimpleFileSystem.h> 
#include <Protocol/LoadedImage.h>

typedef struct {
  UINTN PhysicalStart;
  UINTN NumberOfPages;
  UINTN Type;
} TOOLOS_MEMORYMAP_INFO;

typedef struct {
  UINTN FrameBufferBase; 
  UINTN FrameBufferSize; 
  UINT32 VerticalResolution; 
  UINT32 HorizontalResolution;
  UINT32 PixelsPerScanLine; 
} TOOLOS_GRAFHICS_INFO;

typedef struct {
  unsigned char e_ident[16];
	UINT16 e_type;
	UINT16 e_machine;
	UINT32 e_version;
	UINT64 e_entry;
	UINT64 e_phoff;
	UINT64 e_shoff;
	UINT32 e_flags;
	UINT16 e_ehsize;
	UINT16 e_phentsize;
	UINT16 e_phnum;
	UINT16 e_shentsize;
	UINT16 e_shnum;
	UINT16 e_shstrndx;
} elf64_ehdr;

typedef struct {
  UINT32 p_type; 
  UINT32 p_flags;  
  UINT64 p_offset; 
  UINT64 p_vaddr;  
  UINT64 p_paddr;  
  UINT64 p_filesz; 
  UINT64 p_memsz;  
  UINT64 p_align;  
} elf64_phdr;

typedef void (*KERNEL_ENTRY_POINT)(TOOLOS_MEMORYMAP_INFO*, TOOLOS_GRAFHICS_INFO*, UINT64);

EFI_STATUS
EFIAPI
UefiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
  // ----- Environment Setting area ------
  EFI_STATUS Status;
  KERNEL_ENTRY_POINT KernelEntry;
  
  EFI_GRAPHICS_OUTPUT_PROTOCOL* Gop = NULL;
  
  UINTN AcpiBase;
  
  UINTN MemoryMapSize = 0;
  EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
  UINTN MapKey;
  UINTN DescriptorSize;
  UINT32 DescriptorVersion;
  UINTN NeedMemory;
  BOOLEAN Kernel_Load_Safe = FALSE;
  UINTN Kernel_Load_Address = 0;
  UINTN TotalNum;
  EFI_MEMORY_DESCRIPTOR* MPointer;
  
  EFI_LOADED_IMAGE_PROTOCOL* LoadedImage_Info = NULL;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem = NULL;
  EFI_FILE_PROTOCOL* Root = NULL;
  EFI_FILE_PROTOCOL* KernelFile = NULL;
  elf64_ehdr Elf_header_reader;
  UINTN Header_size = sizeof(elf64_ehdr);
  UINTN ProgramHeader_Size;
  VOID* ProgramHeader_Table;
  elf64_phdr* ProgramHeader_Entry;
  
  TOOLOS_GRAFHICS_INFO TGI;
  gBS->SetWatchdogTimer(0, 0, 0, NULL);
  gST->ConOut->ClearScreen(gST->ConOut);
  // ---- Information extraction area -------
  Status = gBS->LocateProtocol(
    &gEfiGraphicsOutputProtocolGuid,
    NULL,
    (VOID **)&Gop
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR]\r\nFailed to load graphics information.\r\nBooting is no longer possible. Error Code: %r\r\nSolution: Try rebooting your system. If the issue persists, recommend updating your UEFI firmware.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }


  // ACPI 로직 여기에
  

  Status = gBS->GetMemoryMap(
    &MemoryMapSize,
    NULL,
    &MapKey,
    &DescriptorSize,
    &DescriptorVersion
  );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    Print(L"[ERROR]\r\nFailed to determine the size of memory required to hold all memory information. Error Code: %r\r\nSolution: Try rebooting your system. If the issue persists, recommend updating your UEFI firmware.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }

  MemoryMapSize += (2 * DescriptorSize);

  Status = gBS->AllocatePool(
    EfiLoaderData,
    MemoryMapSize,
    (VOID **)&MemoryMap
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR]\r\nFailed to allocate memory space to hold full memory information. Error Code: %r\r\nSolution: Try rebooting your system. If the issue persists, recommend updating your UEFI firmware.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }

  Status = gBS->GetMemoryMap(
    &MemoryMapSize,
    MemoryMap,
    &MapKey,
    &DescriptorSize,
    &DescriptorVersion
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR]\r\nFailed to load full memory information. Error Code: %r\r\nSolution: Try rebooting your system. If the issue persists, recommend updating your UEFI firmware.\r\nIt is safe to shut down by pressing the power button. [1/2]", Status);
    while(1);
  }
  // ------ Information Verification Area ---------
  if (Gop != NULL) {
    TGI.FrameBufferBase = Gop->Mode->FrameBufferBase; 
    TGI.FrameBufferSize = Gop->Mode->FrameBufferSize; 
    TGI.VerticalResolution = Gop->Mode->Info->VerticalResolution; 
    TGI.HorizontalResolution = Gop->Mode->Info->HorizontalResolution;
    TGI.PixelsPerScanLine = Gop->Mode->Info->PixelsPerScanLine;
  } else {
    Print(L"[ERROR]\r\nThe memory that should contain graphics output information is empty.\r\nSolution: Try rebooting your system or try changing to integrated graphics or external graphics.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }


  // 여기 ACPI 로직


  TotalNum = MemoryMapSize / DescriptorSize;
  MPointer = MemoryMap;
  NeedMemory = 4 * 1048576;

  for (UINTN i = 0; i < TotalNum; i++) {
    if (MPointer->Type == EfiConventionalMemory && NeedMemory < (MPointer->NumberOfPages * 4096) && 0x100000 < MPointer->PhysicalStart) {
      Kernel_Load_Address = MPointer->PhysicalStart;
      Kernel_Load_Safe = TRUE;
      break;
    }
    MPointer = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)MPointer + DescriptorSize);
  }
  if (!Kernel_Load_Safe) {
    Print(L"[ERROR]\r\nCould not find suitable space to load the kernel.\r\nSolution: Try adding more memory to your device or upgrading your memory.\r\nIt is safe to shut down by pressing the power button.");
    while(1);
  }


  Status = gBS->HandleProtocol(
      ImageHandle,
      &gEfiLoadedImageProtocolGuid,
      (VOID **)&LoadedImage_Info
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR]\r\nFailed to retrieve information about the current device to activate the file system. Error Code: %r\r\nSolution: Check that the storage device is properly connected. If the problem persists, recommend updating the UEFI firmware.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }

  Status = gBS->HandleProtocol(
      LoadedImage_Info->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID **)&FileSystem
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR]\r\nFailed to access the File System Protocol on the boot device. Error Code:%r\r\nSolution: Make sure your storage device is properly connected. If the issue persists, recommend updating your UEFI firmware.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }

  Status = FileSystem->OpenVolume(
    FileSystem,
    &Root
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR]\r\nFailed to activate the file system. Error Code: %r\r\nSolution: Check that the storage device is properly connected and free of physical damage. If the problem persists, recommend updating the UEFI.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }

  Status = Root->Open(
    Root,
    &KernelFile,
    L"Kernel.elf",
    EFI_FILE_MODE_READ,
    0
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR]\r\nFailed to read kernel file. Error Code: %r\r\nSolution: Verify that the \"Kernel.elf\" file exists and that the device is properly connected. If the problem persists, recommend updating the UEFI.", Status);
    while(1);
  }

  Status = KernelFile->Read(
    KernelFile,
    &Header_size,
    &Elf_header_reader
  );

  if (Elf_header_reader.e_ident[0] != 0x7F || Elf_header_reader.e_ident[1] != 'E' || Elf_header_reader.e_ident[2] != 'L' || Elf_header_reader.e_ident[3] != 'F') {
      Print(L"[ERROR]\r\nThe kernel file is in an untrusted format.\r\nReason: The actual file does not follow the header standard rules.\r\nSolution: Try repairing the elf file using the installation USB. command is \"fixkernel\".\r\nIt is safe to shut down by pressing the power button.");
      while(1);
  }
  
  if (Elf_header_reader.e_ident[4] != 0x02 || Elf_header_reader.e_machine != 0x3E) {
      Print(L"[ERROR]\r\nThe kernel elf file does not support 64-bit.\r\n(Note: Toolos only supports 64-bit systems. This may be a modified kernel file.)\r\n");
      while(1);
  }

  // --------- Kernel load area -------------
  Status = KernelFile->SetPosition(
    KernelFile, 
    Elf_header_reader.e_phoff
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR] An error occurred while parsing the kernel file. Error Code: %r\r\nSolution: Reboot your system. If the problem persists, you can repair it using the \"fixkernel\" command from the installation USB.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }

  ProgramHeader_Size = Elf_header_reader.e_phentsize * Elf_header_reader.e_phnum;

  Status = gBS->AllocatePool(
    EfiLoaderData, 
    ProgramHeader_Size, 
    &ProgramHeader_Table
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR] Could not allocate memory space for kernel file analysis. Error Code:%r\r\nSolution: Reboot the system. If the problem persists, update your UEFI firmware.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }

  Status = KernelFile->Read(
    KernelFile, 
    &ProgramHeader_Size, 
    ProgramHeader_Table
  );
  
  ProgramHeader_Entry = (elf64_phdr*)ProgramHeader_Table;

  for (UINTN i = 0; i < Elf_header_reader.e_phnum; i++) {
    if (ProgramHeader_Entry[i].p_type == 1) { 
        UINT8* Dest = (UINT8*)(Kernel_Load_Address + ProgramHeader_Entry[i].p_vaddr);

        KernelFile->SetPosition(
          KernelFile, 
          ProgramHeader_Entry[i].p_offset
        );

        UINTN Size = ProgramHeader_Entry[i].p_filesz;

        KernelFile->Read(
          KernelFile, 
          &Size, 
          (VOID*)Dest
        );

        if (ProgramHeader_Entry[i].p_filesz < ProgramHeader_Entry[i].p_memsz) {
            UINTN ZeroFillSize = ProgramHeader_Entry[i].p_memsz - ProgramHeader_Entry[i].p_filesz;
            gBS->SetMem(Dest + ProgramHeader_Entry[i].p_filesz, ZeroFillSize, 0);
      }
    }
  }

  // ------------ End area -------------- 
  Status = gBS->GetMemoryMap(
    &MemoryMapSize,
    NULL,
    &MapKey,
    &DescriptorSize,
    &DescriptorVersion
  );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    Print(L"[ERROR]\r\nFailed to determine the size of memory required to hold all memory information. Error Code: %r\r\nSolution: Try rebooting your system. If the issue persists, recommend updating your UEFI firmware.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }

  MemoryMapSize += (2 * DescriptorSize);

  Status = gBS->AllocatePool(
    EfiLoaderData,
    MemoryMapSize,
    (VOID **)&MemoryMap
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR]\r\nFailed to allocate memory space to hold full memory information. Error Code: %r\r\nSolution: Try rebooting your system. If the issue persists, recommend updating your UEFI firmware.\r\nIt is safe to shut down by pressing the power button.", Status);
    while(1);
  }

  Status = gBS->GetMemoryMap(
    &MemoryMapSize,
    MemoryMap,
    &MapKey,
    &DescriptorSize,
    &DescriptorVersion
  );
  if (EFI_ERROR(Status)) {
    Print(L"[ERROR]\r\nFailed to load full memory information. Error Code: %r\r\nSolution: Try rebooting your system. If the issue persists, recommend updating your UEFI firmware.\r\nIt is safe to shut down by pressing the power button. [1/2]", Status);
    while(1);
  }

  TotalNum = MemoryMapSize / DescriptorSize;
  MPointer = MemoryMap;
  TOOLOS_MEMORYMAP_INFO TMI[TotalNum];

  for (UINTN i = 0; i < TotalNum; i++) {
    TMI[i].PhysicalStart = MPointer->PhysicalStart;
    TMI[i].NumberOfPages = MPointer->NumberOfPages;
    TMI[i].Type          = MPointer->Type;
    MPointer = (EFI_MEMORY_DESCRIPTOR*)((UINT8 *)MPointer + DescriptorSize);
  }
  
  KernelEntry = (KERNEL_ENTRY_POINT)(Kernel_Load_Address + Elf_header_reader.e_entry);
  AcpiBase = 0;

  Status = gBS->ExitBootServices(
    ImageHandle,
    MapKey
    );
  if (EFI_ERROR(Status)) {
    Print(L"Failed to exit UEFI environment. Error Code:%r", Status);
    while(1);
  }

  KernelEntry((TOOLOS_MEMORYMAP_INFO*)TMI, &TGI, AcpiBase);
  return EFI_SUCCESS;
}
