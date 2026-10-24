#include "memory.h"
#include <stdio.h>
#include <stdint.h>
#include "hardware/regs/addressmap.h"
#include "pico/stdlib.h"

extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;

static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

void mem_info(void)
{
    const uintptr_t flash_start = XIP_BASE;
    const uintptr_t flash_end = flash_start + PICO_FLASH_SIZE_BYTES;
    const uintptr_t sram_start = SRAM_BASE;
    const uintptr_t sram_end = sram_start + 264u * 1024u;
    const uintptr_t rom_start = ROM_BASE;
    const uintptr_t rom_end = rom_start + 16u * 1024u;

    const uintptr_t image_start = (uintptr_t)&__flash_binary_start;
    const uintptr_t image_end = (uintptr_t)&__flash_binary_end;
    const uintptr_t boot2_start = (uintptr_t)&__boot2_start__;
    const uintptr_t boot2_end = (uintptr_t)&__boot2_end__;
    const uintptr_t text_end = (uintptr_t)&__etext;
    const uintptr_t data_start = (uintptr_t)&__data_start__;
    const uintptr_t data_end = (uintptr_t)&__data_end__;
    const uintptr_t bss_start = (uintptr_t)&__bss_start__;
    const uintptr_t bss_end = (uintptr_t)&__bss_end__;
    const uintptr_t heap_end = (uintptr_t)&__HeapLimit;
    const uintptr_t stack_start = (uintptr_t)&__StackBottom;
    const uintptr_t stack_end = (uintptr_t)&__StackTop;

    const unsigned image_size = (unsigned)(image_end - image_start);
    const unsigned boot2_size = (unsigned)(boot2_end - boot2_start);
    const unsigned text_size = (unsigned)(text_end - boot2_end);
    const unsigned data_size = (unsigned)(data_end - data_start);
    const unsigned bss_size = (unsigned)(bss_end - bss_start);
    const unsigned heap_size = (unsigned)(heap_end - bss_end);
    const unsigned stack_size = (unsigned)(stack_end - stack_start);
    const unsigned flash_free = (unsigned)(flash_end - image_end);

    printf("area       start      end        size\n");
    row("flash", flash_start, flash_end);
    row("sram", sram_start, sram_end);
    row("rom", rom_start, rom_end);
    row("image", image_start, image_end);
    row("free", image_end, flash_end);
    row("boot2", boot2_start, boot2_end);
    row("text", boot2_end, text_end);
    row("data flash", text_end, text_end + data_size);
    row("data ram", data_start, data_end);
    row("bss", bss_start, bss_end);
    row("heap", bss_end, heap_end);
    row("stack", stack_start, stack_end);

    printf("\ntotal\n");
    printf("  flash image %8u = boot2 %u + text %u + data %u\n",
           image_size, boot2_size, text_size, data_size);
    printf("  flash free  %8u of %u\n",
           flash_free, (unsigned)PICO_FLASH_SIZE_BYTES);
    printf("  ram used    %8u = data %u + bss %u\n",
           data_size + bss_size, data_size, bss_size);
    printf("  ram free    %8u for heap and %u for stack\n",
           heap_size, stack_size);
}
