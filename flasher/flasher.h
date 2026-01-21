#ifndef FLASHER_H
#define FLASHER_H

u32 flasherInit(void);
s32 flasherProgram(u32 flashAddr, u8 *sourceAddr, u32 byteCount);
s32 flashRead(u32 address, u8 *rdPtr, u32 size);

#endif /* FLASHER_H */