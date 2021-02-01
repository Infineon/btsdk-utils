/*
 * Copyright 2016-2021, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */
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
