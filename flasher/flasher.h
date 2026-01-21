#ifndef FLASHER_H
#define FLASHER_H

u32 flasherInit(void);
void flasherProgram(u32 flashAddr, u8 *sourceAddr, u32 byteCount);
u32 flashRead(u32 address, u8 *rdPtr, u32 size);

#endif /* FLASHER_H */