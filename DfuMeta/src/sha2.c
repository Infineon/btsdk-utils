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

/*
 *  FIPS-180-2 compliant SHA-256 implementation
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *  The SHA-256 Secure Hash Standard was published by NIST in 2002.
 *
 *  http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf
 */

#include "sha2.h"
//unsigned long sha2_algo = 0;

/*
 * 32-bit integer manipulation macros (big endian)
 */
unsigned long GET_ULONG_BE(const unsigned char* b, unsigned long i)
{
	return (b[i] << 24 )|(b[i+1] << 16 )|(b[i+2] <<  8 )| (b[i+3]);
}
void PUT_ULONG_BE(unsigned long n, unsigned char* b, unsigned long i)
{
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );
    (b)[(i) + 3] = (unsigned char) ( (n)       );
}
/*
 * SHA-256 context setup
 */

const unsigned long X[8] = {
0x6A09E667,
0xBB67AE85,
0x3C6EF372,
0xA54FF53A,
0x510E527F,
0x9B05688C,
0x1F83D9AB,
0x5BE0CD19
};
const unsigned long Z[64] = {0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
						   0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
						   0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
						   0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
						   0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
						   0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
						   0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
						   0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
						   0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
						   0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
						   0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
						   0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
						   0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
						   0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
						   0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
						   0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
	};


void sha2_starts( sha2_context *ctx, int is224 )
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;
	memcpy(ctx->state, X, 8*sizeof(unsigned long));

}

#define  SHR(x,n) ((x & 0xFFFFFFFF) >> n)
#define ROTR(x,n) (SHR(x,n) | (x << (32 - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x,18) ^  SHR(x, 3))
#define S1(x) (ROTR(x,17) ^ ROTR(x,19) ^  SHR(x,10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x,13) ^ ROTR(x,22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x,11) ^ ROTR(x,25))

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

static void sha2_process( sha2_context *ctx, const unsigned char data[64] )
{
    unsigned long temp1, temp2, W[64], i;
	unsigned long Y[8];

	memcpy(Y, ctx->state, 8*sizeof(unsigned long));

	for(i=0;i<64;i++)
	{
		unsigned long j = (64-i)%8;
		if(i<16)
			W[i] = GET_ULONG_BE(data,  i*4 );
		else
			W[i] = S1(W[i -  2]) + W[i -  7] + S0(W[i - 15]) + W[i - 16];

		temp1 = Y[(j+7)%8] + S3(Y[(j+4)%8]) + F1(Y[(j+4)%8],Y[(j+5)%8],Y[(j+6)%8]) + Z[i] + W[i];
		temp2 = S2(Y[j%8]) + F0(Y[j%8],Y[(j+1)%8],Y[(j+2)%8]);
		Y[(j+3)%8] += temp1;
		Y[(j+7)%8] = temp1 + temp2;
	}
	for(i=0;i<8;i++)
		ctx->state[i] += Y[i];
}

/*
 * SHA-256 process buffer
 */
void sha2_update( sha2_context *ctx, unsigned char *input, size_t ilen )
{
    size_t fill;
    unsigned long left;

    if( ilen <= 0 )
        return;

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (unsigned long) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < (unsigned long) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy( (void *) (ctx->buffer + left),
                (void *) input, fill );
        sha2_process( ctx, ctx->buffer );
        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 64 )
    {
        sha2_process( ctx, input );
        input += 64;
        ilen  -= 64;
    }

    if( ilen > 0 )
    {
        memcpy( (void *) (ctx->buffer + left),
                (void *) input, ilen );
    }
}

/*
 * SHA-256 final digest
 */
void sha2_finish( sha2_context *ctx, unsigned char output[32] )
{
    unsigned long last, padn, i;
    unsigned long high, low;
    unsigned char msglen[8];
	unsigned char sha2_padding[64];

	memset(sha2_padding, 0, 64);
	sha2_padding[0] = 0x80;

    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_ULONG_BE( high, msglen, 0 );
    PUT_ULONG_BE( low,  msglen, 4 );

    last = ctx->total[0] & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    sha2_update( ctx, (unsigned char *) sha2_padding, padn );
    sha2_update( ctx, msglen, 8 );

	for(i=0;i<8;i++)
		PUT_ULONG_BE( ctx->state[i], output,  i*4 );
}
