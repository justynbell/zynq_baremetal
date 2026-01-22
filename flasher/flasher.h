#ifndef FLASHER_H
#define FLASHER_H

typedef struct {
    uint32_t START;
    uint32_t IMAGE_SIZE;
    // Add other config registers here
} ConfigMemory;

#define CONFIG_REGISTER ((volatile ConfigMemory *)0x00200000)

#define QSPI_ADDR        0x00000000 // QSPI flash offset

u32 flasherInit(void);
s32 flasherProgram(u32 flashAddr, u32 sourceAddr, u32 byteCount);
s32 flashRead(u32 address, u8 *rdPtr, u32 size);

#endif /* FLASHER_H */