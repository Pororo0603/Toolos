/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. 
 * Copyright (c) 2026, "뿌댕이"
 * Project: ToolOS
 * File: Kernel.c
 * * Description: 
 * The kernel hasn't been touched much yet. 
 * 
*/

#include <Bootinfo.h>
#include <stdint.h>

void Kernel_Start(TOOLOS_MEMORYMAP_INFO *MemoryMap, TOOLOS_GRAFHICS_INFO *Graphic, uint64_t ACPI) {
  uint64_t* Gop = (uint64_t*)Graphic->FrameBufferBase;
  for (uint32_t i = 0; i < Graphic->PixelsPerScanLine * Graphic->VerticalResolution; i++) {
    Gop[i] = 0x00FFFFFF;
  }
  __asm__("hlt");
  while(1);
}