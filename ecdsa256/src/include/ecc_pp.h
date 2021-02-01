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
 ********************************************************************
 *    File Name: ecc_pp.h
 *
 *    Abstract: ECDSA signature sign and verify algorithms
 *
 *    Functions:
 *            --
 *
 *    $History:$
 *
 ********************************************************************
*/

#ifndef ECC_PP_H
#define ECC_PP_H

#include "multprecision.h"

/* EC point with projective coordinates */
struct _point
{
	DWORD x[KEY_LENGTH_DWORDS];
	DWORD y[KEY_LENGTH_DWORDS];
	DWORD z[KEY_LENGTH_DWORDS];
};
typedef struct _point Point;

/* EC point with affine coordinates */
struct _pointAff
{
	DWORD x[KEY_LENGTH_DWORDS];
	DWORD y[KEY_LENGTH_DWORDS];
};
typedef struct _point PointAff;

/* EC curve domain parameter type */
typedef struct
{
	// prime modulus
    DWORD p[KEY_LENGTH_DWORDS];

    // order
    DWORD n[KEY_LENGTH_DWORDS];

	// base point, a point on E of order r
    Point G;

}EC;

extern EC curve;
#define modp (DWORD*)(curve.p)
#define modn (DWORD*)(curve.n)

extern UINT32 nprime[];
extern UINT32 rr[];


/* Point multiplication with NAF method */
void ECC_PM_B_NAF(Point *q, Point *p, DWORD *n);
#define ECC_PM(q, p, n)	ECC_PM_B_NAF(q, p, n)

/* ECDSA verification */
BOOL32 ecdsa_verify(unsigned char* digest, unsigned char* signature, Point* key);

/* Pre-computed window size */
#define FP_WINDOW_SIZE      4

/* Bit scanned when using Pre-computed method */
#define FP_MAP_SIZE         (KEY_LENGTH_BITS/FP_WINDOW_SIZE)

/* Built-in public key */
extern Point ecdsa_q;

/* Built-in pre-computed base point */
extern PointAff ecdsa_pre_g[];

/* Built-in pre-computed public key */
extern PointAff ecdsa_pre_q[];

/* Macro for endianess swap */
#define BE_SWAP(buf, index) \
    ((buf[index+0] << 24) | \
     (buf[index+1] << 16) | \
     (buf[index+2] << 8) | \
     (buf[index+3] << 0))


#endif
