#ifndef ROM_VERIFY_H
#define ROM_VERIFY_H

#include <stddef.h>

const char *VerifyRomBuffer(const unsigned char *data, size_t len);
const char *VerifyRomFile(const char *path);

#endif
