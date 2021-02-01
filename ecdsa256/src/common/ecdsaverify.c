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
#include <stdlib.h>

#define HASH_SIZE       32

extern BOOL32 ecdsa_verify(unsigned char* digest, unsigned char* signature, Point* key);


int main (int argc, char *argv[])
{
    mbedtls_sha256_context sha2_ctx;
    unsigned char *buf;
    unsigned char testDigest[KEY_LENGTH_BYTES * 2];
    unsigned char *pSignature;
    unsigned char signature[KEY_LENGTH_BYTES*2];
    unsigned char pubKey[KEY_LENGTH_BYTES * 2];
    FILE *fPatch;
    FILE *fPublicKey;
    int fileSize;
    int totalSize;
    int i = 0;

    if(argc != 2)
    {
        printf("usage: %s <patch.signed>\n", argv[0]);
        return 0;
    }

    // open public key file
    if ((fopen_s(&fPublicKey, "ecdsa256_key.pub.bin", "rb")) != 0)
    {
        printf("can not open output signature file %s\n", argv[2]);
        return -1;
    }
    else
    {
        // read the key
        totalSize = (int)fread(pubKey, 1, KEY_LENGTH_BYTES * 2, fPublicKey);
        fclose(fPublicKey);

        // check key length
        if (totalSize != KEY_LENGTH_BYTES * 2)
        {
            printf("public key legnth incorrect\n");
            return -1;
        }
    }

    // open patch file
    if ((fopen_s(&fPatch, argv[1], "rb")) != 0)
    {
        printf("can not open patch file %s\n", argv[1]);
        return -1;
    }

    // read patch size
    fseek(fPatch, 0, SEEK_END);
    fileSize = ftell(fPatch);
    rewind(fPatch);

    // allocate memory and read the file
    if ((buf = (unsigned char *)malloc(fileSize)) == NULL)
    {
        printf("no memory\n");
        return -1;
    }

    totalSize = (int)fread(buf, fileSize, 1,  fPatch);
    fclose(fPatch);

    mbedtls_sha256_init(&sha2_ctx);

    // everything is ready. Buffer starts at location buf.  The length is
    // totalSize.  The last 64 bytes is the signature.

    // initialize sha256 context
    mbedtls_sha256_starts_ret(&sha2_ctx, 0);

    // generate digest, last bytes are the signature, do not include.
    mbedtls_sha256_update_ret(&sha2_ctx, buf, fileSize - (KEY_LENGTH_BYTES * 2));
    mbedtls_sha256_finish_ret(&sha2_ctx, testDigest);

    // signature is the last (KEY_LENGTH_BYTES * 2) bytes of the file
    pSignature = &buf[fileSize - (KEY_LENGTH_BYTES * 2)];
    for (i = 0; i < KEY_LENGTH_BYTES * 2; ++i)
        signature[i] = *pSignature++;

    if (!ecdsa_verify(testDigest, signature, (Point *)pubKey))
    {
        printf("verify failure\n");
    }
    else
    {
        printf("verify success\n");
    }
    return 0;
}
