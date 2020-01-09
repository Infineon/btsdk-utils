/*
********************************************************************
* THIS INFORMATION IS PROPRIETARY TO
* Cypress Semiconductor.
*-------------------------------------------------------------------
*
*           Copyright (c) 2013 Cypress Semiconductor.
*                      ALL RIGHTS RESERVED
*
********************************************************************

 ********************************************************************
 *    File Name: multprecision.h
 *
 *    Abstract: elliptic curve long integer math library
 *
 *    Functions:
 *            --
 *
 *    $History:$
 *
 ********************************************************************
*/
#ifndef MULTPRECISION_H
#define MULTPRECISION_H

#include "types.h"

//#define ARM_ASM

#define DWORD_BITS 		        32
#define DWORD_BYTES 	        4
#define DWORD_BITS_SHIFT        5

#define KEY_LENGTH_BITS			256
#define KEY_LENGTH_DWORDS   	(KEY_LENGTH_BITS/DWORD_BITS)
#define KEY_LENGTH_BYTES		(KEY_LENGTH_DWORDS*DWORD_BYTES)


/* return 1 if a > b, return -1 if a < b, return 0 if a == b */
int MP_CMP(DWORD *a, DWORD *b);

/* return 1 if a is zero, return 0 otherwise */
int MP_isZero(DWORD *a);

/* set c = 0 */
void MP_Init(DWORD *c);

/* assign c = a */
void MP_Copy(DWORD *c, DWORD *a);

/* return most significant bit postion */
UINT32 MP_MostSignBits(DWORD *a);

/* compute aminus = u ^ -1 mod modulus */
void MP_InvMod(DWORD *aminus, DWORD *u, const DWORD* modulus);

/* c = a + b */
DWORD MP_Add(DWORD *c, DWORD *a, DWORD *b);

/* c = a + b mod p */
void MP_AddMod(DWORD *c, DWORD *a, DWORD *b);

/* c = a - b */
DWORD MP_Sub(DWORD *c, DWORD *a, DWORD *b);

/* c = a - b mod p */
void MP_SubMod(DWORD *c, DWORD *a, DWORD *b);

/* c = a >> 1 */
void MP_RShift(DWORD * c, DWORD * a);

/* c = a << 1 */
DWORD MP_LShift(DWORD * c, DWORD * a);

/* c = a << 1  mod a */
void MP_LShiftMod(DWORD * c, DWORD * a);

/* c = a * b mod p */
void MP_MersennsMultMod(DWORD *c, DWORD *a, DWORD *b);

/* c = a * a mod p */
void MP_MersennsSquaMod(DWORD *c, DWORD *a);

/* c = a * b */
void MP_Mult(DWORD *c, DWORD *a, DWORD *b);

/* c = a * b * r^-1 mod n */
void MP_MultMont(DWORD *c, DWORD *a, DWORD *b);


#endif
