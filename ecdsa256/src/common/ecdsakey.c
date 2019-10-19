#include "sha256.h"
#include "multprecision.h"
#include "ecc_pp.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#define _CRT_RAND_S
#include <stdlib.h>

void fprint_point(Point *p, FILE *fp)
{
    unsigned char buf[200];
    int i;
    fputs("    { ", fp);
    for (i = 0; i < KEY_LENGTH_DWORDS; i++)
        sprintf_s(&buf[12 * i], sizeof(buf) - (12 * i), "0x%08x, ", p->x[i]);
    buf[12 * i] = 0;
    fputs(buf, fp);
    fputs("}, \n    { ", fp);
    for (i = 0; i < KEY_LENGTH_DWORDS; i++)
        sprintf_s(&buf[12 * i], sizeof(buf) - (12 * i), "0x%08x, ", p->y[i]);
    buf[12 * i] = 0;
    fputs(buf, fp);
    fputs("},\n", fp);
}

//extern int sha2_self_test(int verbose);

int main (int argc, char *argv[])
{
    unsigned char buf[KEY_LENGTH_BYTES * 2];
    FILE *fKey;
    int i, j;
    UINT32 privateKey[KEY_LENGTH_DWORDS];
    Point publicKey;
    unsigned char *p;
    unsigned short zeroes_count = 0;

    if (argc > 2)
    {
        printf("usage: ecgenkey <private_key.bin> to generate public key from existing private key\n");
        printf("usage: ecgenkey to generate private - public key pair\n");
        return 0;
    }

    if (argc == 2)
    {
        if ((fopen_s(&fKey, argv[1], "rb")) != 0)
        {
            printf("can not open key file %s\n", argv[1]);
            return 0;
        }
        else
        {
            int fileSize, readBytes;

            // read private key
            fseek(fKey, 0, SEEK_END);
            fileSize = ftell(fKey);
            rewind(fKey);
            readBytes = (int)fread(buf, 1, fileSize, fKey);
            fclose(fKey);

            if (readBytes != KEY_LENGTH_BYTES)
            {
                printf("wrong key length\n");
                return 0;
            }
        }

        for (i = 0; i < KEY_LENGTH_DWORDS; i++)
        {
            //privateKey[KEY_LENGTH_DWORDS-1-i] = buf[0+4*i] << 24 | buf[1+4*i] << 16 | buf[2+4*i] << 8 | buf[3+4*i] ;
            privateKey[KEY_LENGTH_DWORDS - 1 - i] = BE_SWAP(buf, 4 * i);
        }
    }
    else
    {
        // generate private key
        srand((UINT32)time(NULL));
        for (i = 0; i < KEY_LENGTH_DWORDS; i++)
        {
            UINT32 r;
#ifdef WIN32
            rand_s(&r);
#else
            r = rand_s(&r);
#endif
            *(UINT32 *)&buf[4 * i] = r;
            //privateKey[KEY_LENGTH_DWORDS-1-i] = buf[0+4*i] << 24 | buf[1+4*i] << 16 | buf[2+4*i] << 8 | buf[3+4*i] ;
            privateKey[KEY_LENGTH_DWORDS - 1 - i] = BE_SWAP(buf, 4 * i);
        }
        if ((fopen_s(&fKey, "ecdsa256_key.pri.bin", "wb")) != 0)
        {
            printf("failed to open output file ecdsa256_key.pri\n");
            return(-1);
        }
        if (fwrite(buf, 1, KEY_LENGTH_BYTES, fKey) != KEY_LENGTH_BYTES)
        {
            printf("failed to write private key\n");
            return(-1);
        }
        fclose(fKey);
    }

    ECC_PM(&publicKey, &(curve.G), (DWORD*)privateKey);

    if ((fopen_s(&fKey, "ecdsa256_key.pub.bin", "wb")) != 0)
    {
        printf("failed to open output file ecdsa256_key.pub.bin\n");
        return(-1);
    }
    if (fwrite(&publicKey, 1, KEY_LENGTH_BYTES * 2, fKey) != KEY_LENGTH_BYTES * 2)
    {
        printf("failed to write public binary key\n");
        return(-1);
    }
    fclose(fKey);

    if ((fopen_s(&fKey, "ecdsa256_pub.c", "w")) != 0)
    {
        printf("failed to open output file ecdsa256_key.pub.bin\n");
        return(-1);
    }
    fputs("#include <bt_types.h>\n", fKey);
    fputs("#include <p_256_ecc_pp.h>\n\n", fKey);
    fputs("// public key\n", fKey);
    fputs("Point ecdsa256_public_key = \n", fKey);
    fputs("{\n", fKey);
    fprint_point(&publicKey, fKey);
    fputs("};\n", fKey);
    fclose(fKey);

    if ((fopen_s(&fKey, "ecdsa256_key_plus.pub.bin", "wb")) != 0)
    {
        printf("failed to open output file ecdsa256_key.pub.bin\n");
        return(-1);
    }
    if (fwrite(&publicKey, 1, KEY_LENGTH_BYTES * 2, fKey) != KEY_LENGTH_BYTES * 2)
    {
        printf("failed to write public binary key\n");
        return(-1);
    }
    p = (unsigned char *)&publicKey;

    for (i = 0; i < KEY_LENGTH_BYTES * 2; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if ((p[i] & 1) == 0)
                zeroes_count++;
            p[i] = p[i] >> 1;
        }
    }
    if (fwrite(&zeroes_count, 1, 2, fKey) != 2)
    {
        printf("failed to write zero count\n");
        return(-1);
    }
    fclose(fKey);

    printf("private key saved in ecdsa256_key.pri.bin\n");
    printf("public key saved in ecdsa256_key.pub.bin\n");
    printf("public key with number of zeroes saved in ecdsa256_key_plus.pub.bin\n");
    printf("include ecdsa256_pub.c in the project\n");
    return 0;
}
