#include "rom_verify.h"
#include "sha1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    const char *hex_sha1;
} KnownRom;

static const KnownRom kKnownRoms[] = {
    { "Sonic Advance 2 (USA)",    "7bcd6a07af7c894746fa28073fe0c0e34408022d" },
};

static void BytesToHex(const unsigned char *bytes, int len, char *out_hex)
{
    static const char hexchars[] = "0123456789abcdef";
    int i;
    for (i = 0; i < len; i++) {
        out_hex[i*2]   = hexchars[(bytes[i] >> 4) & 0xF];
        out_hex[i*2+1] = hexchars[bytes[i] & 0xF];
    }
    out_hex[len*2] = '\0';
}

const char *VerifyRomBuffer(const unsigned char *data, size_t len)
{
    SHA1_CTX ctx;
    unsigned char digest[20];
    char hexdigest[41];
    int i;

    SHA1Init(&ctx);
    SHA1Update(&ctx, data, len);
    SHA1Final(digest, &ctx);
    BytesToHex(digest, 20, hexdigest);

    for (i = 0; i < (int)(sizeof(kKnownRoms) / sizeof(kKnownRoms[0])); i++) {
        if (strcmp(hexdigest, kKnownRoms[i].hex_sha1) == 0) {
            return kKnownRoms[i].name;
        }
    }
    return NULL;
}

const char *VerifyRomFile(const char *path)
{
    FILE *f = fopen(path, "rb");
    unsigned char *buf;
    long size;
    const char *result;

    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    buf = (unsigned char *)malloc(size);
    if (!buf) { fclose(f); return NULL; }

    fread(buf, 1, size, f);
    fclose(f);

    result = VerifyRomBuffer(buf, (size_t)size);
    free(buf);
    return result;
}

#ifdef __ANDROID__
#include <jni.h>

JNIEXPORT jstring JNICALL
Java_com_satr_romgate_RomVerifyActivity_nativeVerifyRom(JNIEnv *env, jobject thiz, jbyteArray romData)
{
    jsize len = (*env)->GetArrayLength(env, romData);
    jbyte *bytes = (*env)->GetByteArrayElements(env, romData, NULL);

    const char *result = VerifyRomBuffer((const unsigned char *)bytes, (size_t)len);

    (*env)->ReleaseByteArrayElements(env, romData, bytes, JNI_ABORT);

    if (result != NULL) {
        return (*env)->NewStringUTF(env, result);
    }
    return NULL;
}
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE
const char *WebVerifyRom(const unsigned char *data, int len)
{
    return VerifyRomBuffer(data, (size_t)len);
}
#endif
