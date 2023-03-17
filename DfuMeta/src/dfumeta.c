/*
* Copyright 2016-2023, Cypress Semiconductor Corporation (an Infineon company) or
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

/** @file
*
* DFU Meta Data generator
*
* Usage: DfuMeta cid=0x<company ID> pid=0x<product ID> vid=0x<hardware version> version=<n.n> -k <key file> -i <input file> -m <output metadata file> -o <output manifest file>
* For example: DfuMeta cid=0x0131 pid=0x3016 vid=0x0002 version=1.1 -k ecdsa256_key.pri.bin -i BLE_Mesh_OnOffServer_CYBT-213043-MESH.ota.bin -m metadata -o manifest.json
*
* This console application takes input binary file, compute the hash from firmware ID + binary file,
* sign it with a private key.
*
* Following is an example of manifest file:
* {
*   "manifest": {
*     "firmware": {
*       "firmware_file": "BLE_Mesh_OnOffServer_CYBT-213043-MESH.ota.bin",
*       "metadata_file": "metadata",
*       "firmware_id": "0131301600020101"
*     }
*   }
* }
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sha2.h"
#include "multprecision.h"
#include "ecc_pp.h"

//#define DEBUG

#define MAX_METADATA_LENGTH             16
#define HASH_SIZE                       32

#define JSON_FILE_HEADER   "{\n  \"manifest\": {\n    \"firmware\": {\n"
#define JSON_FILE_FOOTER   "    }\n  }\n} "
#define TIMESTAMP_TAG      "      \"timestamp\": "
#define FIRMWARE_FILE_TAG  "      \"firmware_file\": "
#define FIRMWARE_ID_TAG    "      \"firmware_id\": "
#define METADATA_FILE_TAG  "      \"metadata_file\": "
#define FACTORY_RESET_TAG  "      \"factory_reset\": "

void write_hex_data_to_file(FILE *pf, unsigned char *data, int data_len)
{
    int i;

    for (i = 0; i < data_len; i++)
        fprintf(pf, "%02X", data[i]);
}

#ifdef DEBUG
void dump_hex(unsigned char *p, int len)
{
    int i = 0;

    while (i < len)
    {
        printf("%02X ", p[i++]);
        if (i % 16 == 0)
            printf("\n");
    }

    if (i % 16)
        printf("\n");
}
#endif

int main(int argc, char *argv[])
{
    FILE * pfInput = NULL;
    FILE * pfOutput = NULL;
    FILE * pfMetadata = NULL;
    FILE * pfKey = NULL;
    FILE * pfDfuBin = NULL;
    unsigned int cid = 0, pid = 0, vid = 0, major_version = 0, minor_version = 0;
    unsigned char imagedata[16];
    unsigned char metadata[MAX_METADATA_LENGTH];
    unsigned int metadata_length = 0;
    size_t file_size;
    size_t read_size;
    sha2_context sha2_ctx;
    unsigned char *buf;
    unsigned char digest[HASH_SIZE];
    unsigned char signature[KEY_LENGTH_BYTES*2];
    unsigned char private_key[KEY_LENGTH_BYTES];
    unsigned char random_key[KEY_LENGTH_BYTES];
    int i;
    char * p_metadata_filename = NULL;
    char * p_firmware_filename = NULL;
    char dfu_image_filename[256];
    char * p;
    int timestamp = 0;
    int factory_reset = 0;

    /*
     * Read command line arguments
     */
    for (i = 1; i < argc; i++)
    {
        if (strstr(argv[i], "cid=") == argv[i])
        {
            if (sscanf(argv[i], "cid=0x%x", &cid) != 1)
            {
                printf("ERROR!!! Invalid CID\n");
                return -1;
            }
        }
        else if (strstr(argv[i], "pid=") == argv[i])
        {
            if (sscanf(argv[i], "pid=0x%x", &pid) != 1)
            {
                printf("ERROR!!! Invalid PID\n");
                return -1;
            }
        }
        else if (strstr(argv[i], "vid=") == argv[i])
        {
            if (sscanf(argv[i], "vid=0x%x", &vid) != 1)
            {
                printf("ERROR!!! Invalid VID\n");
                return -1;
            }
        }
        else if (strstr(argv[i], "version=") == argv[i])
        {
            if (sscanf(argv[i], "version=%d.%d", &major_version, &minor_version) != 2)
            {
                printf("ERROR!!! Invalid version\n");
                return -1;
            }
        }
        else if (strcmp(argv[i], "-k") == 0)
        {
            pfKey = fopen(argv[++i], "rb");
            if (!pfKey)
            {
                printf("ERROR!!! Failed to open private key file: %s\n", argv[i]);
                return -1;
            }
        }
        else if (strcmp(argv[i], "-i") == 0)
        {
            pfInput = fopen(argv[++i], "rb");
            if (!pfInput)
            {
                printf("ERROR!!! Failed to open input file: %s\n", argv[i]);
                return -1;
            }
            p_firmware_filename = argv[i];
        }
        else if (strcmp(argv[i], "-m") == 0)
        {
            pfMetadata = fopen(argv[++i], "wb");
            if (!pfMetadata)
            {
                printf("ERROR!!! Failed to open metadata file for writing\n");
                return -1;
            }
            p_metadata_filename = argv[i];
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            pfOutput = fopen(argv[++i], "w");
            if (!pfOutput)
            {
                printf("ERROR!!! Failed to open manifest file for writing\n");
                return -1;
            }
        }
        else if (strcmp(argv[i], "-t") == 0)
        {
            timestamp = 1;
        }
        else if (strcmp(argv[i], "-f") == 0)
        {
            factory_reset = 1;
        }
        else
        {
            printf("ERROR!!! Unknown argument: %s\n", argv[i]);
            return -1;
        }
    }
    if (cid == 0 || pid == 0 || vid == 0 || (major_version + minor_version) == 0 || pfKey == NULL || pfInput == NULL || pfOutput == NULL)
    {
        printf("Usage: %s cid=0x<company ID> pid=0x<product ID> vid=0x<hardware version> version=<n.n> -k <key file> -i <input file> -m <output metadata file> -o <output manifest file>\n", argv[0]);
        return -1;
    }
    fread(imagedata, 1, 16, pfInput);
    if (memcmp(imagedata, "BRCM", 4) == 0)  // exclude 20706 firmware image
    {
        if (major_version != (unsigned int)imagedata[10] || minor_version != (unsigned int)imagedata[11])
        {
            printf("ERROR!!! Image version %d.%d does not match signing version %d.%d\n", imagedata[10], imagedata[11], major_version, minor_version);
            return -1;
        }
    }
    rewind(pfInput);

    // remove path
    if ((p = strrchr(p_firmware_filename, '\\')) != NULL)
        p_firmware_filename = p + 1;
    // construct DFU image file name
    strncpy(dfu_image_filename, p_firmware_filename, 256);
    p = strstr(dfu_image_filename, ".ota.bin");
    if (p)
        strcpy(p, ".dfu.bin");
    else
        strcat(dfu_image_filename, ".dfu.bin");
    pfDfuBin = fopen(dfu_image_filename, "wb");
    if (!pfDfuBin)
    {
        printf("ERROR!!! Failed to open .dfu.bin file for writing\n");
        return -1;
    }

    /*
     * Construct metadata
     */
    i = 0;
    metadata[i++] = (unsigned char)(cid >> 8);
    metadata[i++] = (unsigned char) cid;
    metadata[i++] = (unsigned char)(pid >> 8);
    metadata[i++] = (unsigned char) pid;
    metadata[i++] = (unsigned char)(vid >> 8);
    metadata[i++] = (unsigned char) vid;
    metadata[i++] = (unsigned char) major_version;
    metadata[i++] = (unsigned char) minor_version;
    metadata_length = i;

    /*
     * Generate digest
     */
    sha2_starts(&sha2_ctx, 0);

    // Metadata should be included in the hash
    sha2_update(&sha2_ctx, metadata, metadata_length);

    // Then the firmware
    fseek(pfInput, 0, SEEK_END);
    file_size = ftell(pfInput);
    rewind(pfInput);
    if ((buf = (unsigned char *)malloc(file_size)) == NULL)
    {
        printf("ERROR!!! Out of memory\n");
        return -1;
    }
    read_size = fread(buf, 1, file_size, pfInput);
    if (read_size != file_size)
    {
        printf("ERROR!!! File size: %d, read size: %d\n", (int)file_size, (int)read_size);
        return -1;
    }
    sha2_update(&sha2_ctx, buf, file_size);
    fwrite(buf, 1, file_size, pfDfuBin);
    free(buf);

    sha2_finish(&sha2_ctx, digest);
#ifdef DEBUG
    printf("Digest:\n");
    dump_hex(digest, sizeof(digest));
#endif

    /*
     * Generate signature
     */
    // Read private key from file
    fseek(pfKey, 0, SEEK_END);
    file_size = ftell(pfKey);
    rewind(pfKey);
    read_size = fread(private_key, 1, file_size, pfKey);
    fclose(pfKey);

    if (read_size != KEY_LENGTH_BYTES)
    {
        printf("ERROR!!! Wrong private key size %d\n", (int)read_size);
        return -1;
    }

    // Generate random key
    srand((UINT32)time(NULL));
    for (i = 0; i < KEY_LENGTH_DWORDS; i++)
    {
        *(UINT32 *)&random_key[4 * i] = rand();
    }

    // Sign
    if(!ecdsa_sign(digest, signature, private_key, random_key))
    {
        printf("ERROR!!! Failed to generate signature\n");
        return -1;
    }

    /*
     * Add signature to the end of DFU image file
     */
    fwrite(signature, 1, sizeof(signature), pfDfuBin);
    fclose(pfDfuBin);

    /*
    * Save metadata to file
    */
    if (pfMetadata)
    {
        fwrite(metadata, 1, metadata_length, pfMetadata);
        fclose(pfMetadata);
    }

    /*
     * Write output file
     */
    fwrite(JSON_FILE_HEADER, 1, strlen(JSON_FILE_HEADER), pfOutput);
    fwrite(FIRMWARE_FILE_TAG, 1, strlen(FIRMWARE_FILE_TAG), pfOutput);
    fwrite("\"", 1, 1, pfOutput);
    fwrite(dfu_image_filename, 1, strlen(dfu_image_filename), pfOutput);
    fwrite("\",\n", 1, 3, pfOutput);
    if (p_metadata_filename)
    {
        fwrite(METADATA_FILE_TAG, 1, strlen(METADATA_FILE_TAG), pfOutput);
        fwrite("\"", 1, 1, pfOutput);
        fwrite(p_metadata_filename, 1, strlen(p_metadata_filename), pfOutput);
        fwrite("\",\n", 1, 3, pfOutput);
    }
    if (timestamp)
    {
        char str[32];
        time_t t = time(NULL);
        struct tm *utc = gmtime(&t);
        size_t len = strftime(str, 32, "%Y-%m-%dT%H:%M:%SZ", utc);

        fwrite(TIMESTAMP_TAG, 1, strlen(TIMESTAMP_TAG), pfOutput);
        fwrite("\"", 1, 1, pfOutput);
        fwrite(str, 1, len, pfOutput);
        fwrite("\",\n", 1, 3, pfOutput);
    }
    if (factory_reset)
    {
        fwrite(FACTORY_RESET_TAG, 1, strlen(FACTORY_RESET_TAG), pfOutput);
        fwrite("\"", 1, 1, pfOutput);
        fwrite("1", 1, 1, pfOutput);
        fwrite("\",\n", 1, 3, pfOutput);
    }
    fwrite(FIRMWARE_ID_TAG, 1, strlen(FIRMWARE_ID_TAG), pfOutput);
    fwrite("\"", 1, 1, pfOutput);
    // Firmware ID can be two bytes company ID plus any vendor specific data. We use metadata here for demo only.
    write_hex_data_to_file(pfOutput, metadata, metadata_length);
    fwrite("\"\n", 1, 2, pfOutput);
    fwrite(JSON_FILE_FOOTER, 1, strlen(JSON_FILE_FOOTER), pfOutput);

    fclose(pfInput);
    fclose(pfOutput);

    return 0;
}
