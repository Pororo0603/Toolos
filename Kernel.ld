/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. 
 * Copyright (c) 2026, "뿌댕이"
 * Project: ToolOS
 * File: Kernel.ld
 * * Description: 
 * Sets the kernel load point.
*/

ENTRY(Kernel_Start)

SECTIONS
{
    . = 0x0; 

    .text BLOCK(0x1000) : {
        *(.text)   
    }

    .rodata BLOCK(0x1000) : {
        *(.rodata)
    }

    .data BLOCK(0x1000) : {
        *(.data)
    }

    .bss BLOCK(0x1000) : {
        *(.bss)
    }
}