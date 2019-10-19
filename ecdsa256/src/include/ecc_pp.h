/*
********************************************************************
* THIS INFORMATION IS PROPRIETARY TO
* BROADCOM CORP.
*-------------------------------------------------------------------
*
*           Copyright (c) 2013 Broadcom Corp.
*                      ALL RIGHTS RESERVED
*
********************************************************************

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
