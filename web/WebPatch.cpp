#include "RetroEngine.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef WEBPATCH_PATH
#define WEBPATCH_PATH "/webport.patch"
#endif

static unsigned int PatchHash(const int *words, int count)
{
    unsigned int h = 2166136261u;
    for (int i = 0; i < count; ++i) {
        unsigned int w = (unsigned int)words[i];
        for (int b = 0; b < 4; ++b) {
            h ^= (w >> (8 * b)) & 0xFF;
            h *= 16777619u;
        }
    }
    return h;
}

void ApplyScriptPatches(const char *scriptPath, int codeStart, int codeLength)
{
    FILE *f = fopen(WEBPATCH_PATH, "rb");
    if (!f)
        return;

    const char *name = strrchr(scriptPath, '/');
    name             = name ? name + 1 : scriptPath;

    char line[8192];
    int applied = 0, skipped = 0;
    while (fgets(line, sizeof(line), f)) {
        char *save = NULL;
        char *op   = strtok_r(line, " \t\r\n", &save);
        if (!op || strcmp(op, "set") != 0)
            continue;

        char *file = strtok_r(NULL, " \t\r\n", &save);
        char *posS = strtok_r(NULL, " \t\r\n", &save);
        char *hashS = strtok_r(NULL, " \t\r\n", &save);
        char *cntS = strtok_r(NULL, " \t\r\n", &save);
        if (!file || !posS || !hashS || !cntS || strcmp(file, name) != 0)
            continue;

        int pos            = (int)strtol(posS, NULL, 10);
        unsigned int hash  = (unsigned int)strtoul(hashS, NULL, 16);
        int count          = (int)strtol(cntS, NULL, 10);
        int words[256];
        bool valid = count > 0 && count <= 256;
        for (int i = 0; valid && i < count; ++i) {
            char *w = strtok_r(NULL, " \t\r\n", &save);
            if (!w)
                valid = false;
            else
                words[i] = (int)strtol(w, NULL, 10);
        }

        if (!valid || pos < 0 || pos + count > codeLength || codeStart + pos + count > SCRIPTCODE_COUNT) {
            printf("webport patch: skipped a change in %s at %d (does not fit)\n", name, pos);
            ++skipped;
            continue;
        }
        if (PatchHash(&scriptCode[codeStart + pos], count) != hash) {
            printf("webport patch: skipped a change in %s at %d (your file is different from the one it was made for)\n", name, pos);
            ++skipped;
            continue;
        }
        memcpy(&scriptCode[codeStart + pos], words, count * sizeof(int));
        ++applied;
    }
    fclose(f);
    if (applied || skipped)
        printf("webport patch: %s: %d applied, %d skipped\n", name, applied, skipped);
}
