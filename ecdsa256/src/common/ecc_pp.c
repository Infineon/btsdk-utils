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
 *    File Name: ecc_pp.c
 *
 *    Abstract: simple pairing algorithms
 *
 *    Functions:
 *            --
 *
 *    $History:$
 *
 ********************************************************************
*/
#include "multprecision.h"
#include "ecc_pp.h"
#include <stdio.h>
#include <string.h>


int nd;
EC curve = {
    { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x0, 0x0, 0x0, 0x1, 0xFFFFFFFF },
    { 0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD, 0xFFFFFFFF, 0xFFFFFFFF, 0x0, 0xFFFFFFFF },
    {
        { 0xd898c296, 0xf4a13945, 0x2deb33a0, 0x77037d81, 0x63a440f2, 0xf8bce6e5, 0xe12c4247, 0x6b17d1f2},
        { 0x37bf51f5, 0xcbb64068, 0x6b315ece, 0x2bce3357, 0x7c0f9e16, 0x8ee7eb4a, 0xfe1a7f9b, 0x4fe342e2},
        { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
    }
};

UINT32 nprime[KEY_LENGTH_DWORDS] = { 0xEE00BC4F, 0xCCD1C8AA, 0x7D74D2E4, 0x48C94408, 0xC588C6F6, 0x50FE77EC, 0xA9D6281C, 0x60D06633};

UINT32 rr[KEY_LENGTH_DWORDS] = { 0xBE79EEA2, 0x83244C95, 0x49BD6FA6, 0x4699799C, 0x2B6BEC59, 0x2845B239, 0xF3D95620, 0x66E12D94 };

void InitPoint(Point *q)
{
    memset(q, 0, sizeof(Point));
}


void CopyPoint(Point *q, PointAff *p)
{
    memcpy(q, p, sizeof(PointAff));
    memset(q->z, 0, sizeof(q->z));
    q->z[0] = 0x1;
}


// q=2q, zq of length KEY_LENGTH_DWORDS+1
void ECC_Double(Point *q, Point *p)
{
	DWORD t1[KEY_LENGTH_DWORDS], t2[KEY_LENGTH_DWORDS], t3[KEY_LENGTH_DWORDS];
	DWORD *x1, *x3, *y1, *y3, *z1, *z3;
//	DWORD t4[KEY_LENGTH_DWORDS];

	if(MP_isZero(p->z))
	{
		MP_Init(q->z);
		return;						// return infinity
	}
	x1=p->x; y1=p->y; z1=p->z;
	x3=q->x; y3=q->y; z3=q->z;

	MP_MersennsSquaMod(t1, z1);				// t1=z1^2
	MP_SubMod(t2, x1, t1);				// t2=x1-t1
	MP_AddMod(t1, x1, t1);				// t1=x1+t1
	MP_MersennsMultMod(t2, t1, t2);			// t2=t2*t1
	MP_LShiftMod(t3, t2);
	MP_AddMod(t2, t3, t2);				// t2=3t2

	MP_MersennsMultMod(z3, y1, z1);			// z3=y1*z1
	MP_LShiftMod(z3, z3);

	MP_MersennsSquaMod(y3, y1);				// y3=y1^2
	MP_LShiftMod(y3, y3);
	MP_MersennsMultMod(t3, y3, x1); 		// t3=y3*x1=x1*y1^2
	MP_LShiftMod(t3, t3);
	MP_MersennsSquaMod(y3, y3);				// y3=y3^2=y1^4
	MP_LShiftMod(y3, y3);

	MP_MersennsSquaMod(x3, t2);				// x3=t2^2
	MP_LShiftMod(t1, t3);			// t1=2t3
	MP_SubMod(x3, x3, t1);				// x3=x3-t1
	MP_SubMod(t1, t3, x3);				// t1=t3-x3
	MP_MersennsMultMod(t1, t1, t2);			// t1=t1*t2
	MP_SubMod(y3, t1, y3);				// y3=t1-y3
}

// q=q+p,     p is affine point
void ECC_Add(Point *r, Point *p, PointAff *q)
{
	DWORD t1[KEY_LENGTH_DWORDS], t2[KEY_LENGTH_DWORDS];
	DWORD k[KEY_LENGTH_DWORDS];
	DWORD s[KEY_LENGTH_DWORDS];
	DWORD *x1, *x2, *x3, *y1, *y2, *y3, *z1, *z3;

	x1=p->x; y1=p->y; z1=p->z;
	x2=q->x; y2=q->y;
	x3=r->x; y3=r->y; z3=r->z;

	// if P=infinity, return q
	if(MP_isZero(z1))
	{
        CopyPoint(r, q);
        return;
	}

	MP_MersennsSquaMod(t1, z1);				// t1=z1^2
	MP_MersennsMultMod(t2, z1, t1);			// t2=t1*z1
	MP_MersennsMultMod(t1, x2, t1);			// t1=t1*x2
	MP_MersennsMultMod(t2, y2, t2);			// t2=t2*y2

	MP_SubMod(t1, t1, x1);				// t1=t1-x1
	MP_SubMod(t2, t2, y1);				// t2=t2-y1


	if(MP_isZero(t1))
	{
		if(MP_isZero(t2))
		{
			ECC_Double(r, q) ;
			return;
		}
		else
		{
			MP_Init(z3);
			return;				// return infinity
		}
	}

	MP_MersennsMultMod(z3, z1, t1);					// z3=z1*t1
	MP_MersennsSquaMod(s, t1);						// t3=t1^2
	MP_MersennsMultMod(k, s, t1);					// t4=t3*t1
	MP_MersennsMultMod(s, s, x1);					// t3=t3*x1
	MP_LShiftMod(t1, s);					// t1=2*t3
	MP_MersennsSquaMod(x3, t2);						// x3=t2^2
	MP_SubMod(x3, x3, t1);						// x3=x3-t1
	MP_SubMod(x3, x3, k);						// x3=x3-t4
	MP_SubMod(s, s, x3);						// t3=t3-x3
	MP_MersennsMultMod(s, s, t2);					// t3=t3*t2
	MP_MersennsMultMod(k, k, y1);					// t4=t4*t1
	MP_SubMod(y3, s, k);
}


// Computing the NAF of a positive integer
void ECC_NAF(UINT8 *naf, UINT32 *NumNAF, DWORD *k)
{
    UINT32    sign;
	int i=0;
	int j;

	while(MP_MostSignBits(k)>=1)			// k>=1
	{
		if(k[0] & 0x01)		// k is odd
		{
			sign=(k[0]&0x03);			// 1 or 3

			// k = k-naf[i]
			if(sign==1)
				k[0]=k[0]&0xFFFFFFFE;
			else
			{
				k[0]=k[0]+1;
				if(k[0]==0)				//overflow
				{
					j=1;
					do
					{
						k[j]++;
					}while(k[j++]==0);			//overflow
				}
			}
		}
		else
			sign=0;

		MP_RShift(k, k);

        naf[i / 4] |= (sign) << ((i % 4) * 2);

		i++;
	}

	*NumNAF=i;
}

void ECC_PRJ_TO_AFF(Point* q)
{
    UINT32 w[KEY_LENGTH_DWORDS];

    MP_InvMod(w, q->z, modp);
    MP_MersennsSquaMod(q->z, w);
    MP_MersennsMultMod(q->x, q->x, q->z);

    MP_MersennsMultMod(q->z, q->z, w);
    MP_MersennsMultMod(q->y, q->y, q->z);

	MP_Init(q->z);
	q->z[0]=1;
}

// Binary NAF for point multiplication
void ECC_PM_B_NAF(Point *q, Point *p, DWORD *n)
{
	int i;
    UINT32 sign;
	UINT8 naf[KEY_LENGTH_BITS / 4 +1];
	UINT32 NumNaf;
	PointAff minus_p;
	Point r;

	InitPoint(&r);

	// initialization
	InitPoint(q);
	// -p
	MP_Copy(minus_p.x, (DWORD*)p->x);
	MP_Sub(minus_p.y, modp, (DWORD*)p->y);

	// NAF
	memset(naf, 0, sizeof(naf));
	ECC_NAF(naf, &NumNaf, n);
	for(i=NumNaf-1; i>=0; i--)
    {
		ECC_Double(q, q);

		sign = (naf[i / 4] >> ((i % 4) * 2)) & 0x03;

		if(sign==1)
		{
			ECC_Add(q, q, (PointAff*)p);
		}
		else if(sign == 3)
		{
			ECC_Add(q, q, &minus_p);
		}

	}

    ECC_PRJ_TO_AFF(q);
}

/*!
  @brief This function divides n-bit integer to m-bit array
  @param [in] n input scalar
  @param [out] m output m-bit array
*/
void mapScalar(UINT32* n, UINT32* m)
{
    UINT32 mapIdx = 0;
    UINT32 mapPos = 0;
    UINT32 tmp32;
    UINT32 i;
    UINT32 j;

    // scan n from right to left word by word
    for(i = 0; i < KEY_LENGTH_DWORDS; i++)
    {
        // read current word value
        tmp32 = n[i];

        // scan current word from right to left
        for(j = 0; j < DWORD_BITS; j++)
        {
            // set map value based on right-most bit
            m[mapIdx] |= (tmp32 & 0x1) << mapPos;

            // right shift 1 bit
            tmp32 >>= 1;

            // increment map index
            mapIdx++;
            if(mapIdx >= FP_MAP_SIZE)
            {
                // reset map index and shift position
                mapIdx = 0;
                mapPos++;
            }
        }
    }
}

#if 0
/*!
  @brief This function computes u1*G + u2*Q using Fixed Point method
         and Shamir's trick.
  @param [out] q output point
  @param [in] u1, first scalar
  @param [in] u2, second scalar
*/
void ecdsa_fp_shamir(Point *q, UINT32* u1, UINT32* u2)
{
    int i;
    int map;
    UINT32 m1[FP_MAP_SIZE];
    UINT32 m2[FP_MAP_SIZE];
    PointAff* t;

    // initialize output point
    InitPoint(q);

    // remap u1
    memset(m1, 0, sizeof(m1));
    mapScalar(u1, m1);

    // remap u2
    memset(m2, 0, sizeof(m2));
    mapScalar(u2, m2);

    // scan m1 and m2 from left to right
    for(i = FP_MAP_SIZE; i > 0; i--)
    {
        // double in each iteration
        ECC_Double(q, q);

        // check m1 current bit pattern
        map = m1[i-1];
        if(map)
        {
            // add pre-computed G
            t = &(ecdsa_pre_g[map]);
            ECC_Add(q, q, t);
        }

        // check m2 current bit pattern
        map = m2[i-1];
        if(map)
        {
            // add pre-computed Q
            t = &(ecdsa_pre_q[map]);
            ECC_Add(q, q, t);
        }
    }

    // convert output from projective to affine domain
    ECC_PRJ_TO_AFF(q);
}
#endif


// ECDSA Verify
BOOL32 ecdsa_verify(unsigned char* digest, unsigned char* signature, Point* key)
{
   UINT32 e[KEY_LENGTH_DWORDS];
   UINT32 r[KEY_LENGTH_DWORDS];
   UINT32 s[KEY_LENGTH_DWORDS];

   UINT32 u1[KEY_LENGTH_DWORDS];
   UINT32 u2[KEY_LENGTH_DWORDS];

   UINT32 tmp1[KEY_LENGTH_DWORDS];
   UINT32 tmp2[KEY_LENGTH_DWORDS];

   Point p1, p2;
   UINT32 i;

   // swap input data endianess
   for(i = 0; i < KEY_LENGTH_DWORDS; i++)
   {
       // import digest to long integer
       e[KEY_LENGTH_DWORDS-1-i] = BE_SWAP(digest, 4*i);

       // import signature to long integer
       r[KEY_LENGTH_DWORDS-1-i] = BE_SWAP(signature, 4*i);
       s[KEY_LENGTH_DWORDS-1-i] = BE_SWAP(signature, KEY_LENGTH_BYTES+4*i);
   }

   // compute s' = s ^ -1 mod n
   MP_InvMod(tmp1, s, modn);

   // convert s' to montgomery domain
   MP_MultMont(tmp2, tmp1, (DWORD*)rr);

   // convert e to montgomery domain
   MP_MultMont(tmp1, e, (DWORD*)rr);

   // compute u1 = e * s' mod n
   MP_MultMont(u1, tmp1, tmp2);

   // convert r to montgomery domain
   MP_MultMont(tmp1, r, (DWORD*)rr);

   // compute u2 = r * s' (mod n)
   MP_MultMont(u2, tmp1, tmp2);

   // set tmp1 = 1
   MP_Init(tmp1);
   tmp1[0]=1;

   // convert u1 to normal domain
   MP_MultMont(u1, u1, tmp1);

   // convert u2 to normal domain
   MP_MultMont(u2, u2, tmp1);

   // compute (x,y) = u1G + u2QA
   if(key)
   {
       // if public key is given, using legacy method
       ECC_PM(&p1, &(curve.G), u1);
       ECC_PM(&p2, key, u2);
       ECC_Add(&p1, &p1, (PointAff*)&p2);

       // convert point to affine domain
       MP_InvMod(tmp1, p1.z, modp);
       MP_MersennsSquaMod(p1.z, tmp1);
       MP_MersennsMultMod(p1.x, p1.x, p1.z);
   }
   else
   {
       printf("Error\n");
#if 0
       // if public key is not given, using pre-computed method
       ecdsa_fp_shamir(&p1, u1, u2);
#endif
   }

   // w = r (mod n)
   if(MP_CMP(r, modp) >= 0)
       MP_Sub(r, r, modp);

   // verify r == x ?
   if(memcmp(r, p1.x, KEY_LENGTH_BYTES))
       return FALSE;

   // signature match, return true
   return TRUE;
}

BOOL32 ecdsa_sign(unsigned char* digest, unsigned char* signature, unsigned char* privateKey, unsigned char* randomK)
{
   UINT32 kprime[KEY_LENGTH_DWORDS];
   UINT32 k[KEY_LENGTH_DWORDS];
   UINT32 da[KEY_LENGTH_DWORDS];
   UINT32 e[KEY_LENGTH_DWORDS];

   // signature part 1
   UINT32 r[KEY_LENGTH_DWORDS];

   // signature part 2
   UINT32 s[KEY_LENGTH_DWORDS];

   // intermediate result
   UINT32 tmp1[KEY_LENGTH_DWORDS];
   UINT32 tmp2[KEY_LENGTH_DWORDS];
   UINT32 tmp3[KEY_LENGTH_DWORDS];

   Point p;
   UINT32 i;

   /////////////////////////////////////////////////
   // check input and swap endianess
   /////////////////////////////////////////////////

   // swap input endianess
   for(i = 0; i < KEY_LENGTH_DWORDS; i++)
   {
       // k = randomK
       k[KEY_LENGTH_DWORDS-1-i] = BE_SWAP(randomK, 4*i);

       // da = private key
       da[KEY_LENGTH_DWORDS-1-i] = BE_SWAP(privateKey, 4*i);

       // e = digest
       e[KEY_LENGTH_DWORDS-1-i] = BE_SWAP(digest, 4*i);
   }

   // random number should not be zero
   if(MP_isZero(k)) return FALSE;

   /////////////////////////////////////////////////
   // step 1: k' = k ^ -1 mod n
   /////////////////////////////////////////////////
   memcpy(tmp1, k, sizeof(k));
   MP_InvMod(kprime, tmp1, modn);

   /////////////////////////////////////////////////
   // step 2 : p = k * G
   /////////////////////////////////////////////////
   ECC_PM(&p, &(curve.G), k);

   /////////////////////////////////////////////////
   // step 3: r = x-coordinate of (k * G) mod n
   /////////////////////////////////////////////////

   // 3.1 r = x-coordinate of (k * G)
   MP_Copy(r, p.x);

   // 3.2 r = r mod n
   while(MP_CMP(r, modn) >= 0)
       MP_Sub(r, r, modn);

   // 3.3 fail if r == 0
   if(MP_isZero(r)) return FALSE;

   /////////////////////////////////////////////////
   // step 4: compute da * r mod n
   /////////////////////////////////////////////////

   // 4.1 convert r to montgomery domain
   MP_MultMont(tmp1, r, (DWORD*)rr);

   // 4.2 convert da to montgomery domain
   MP_MultMont(tmp2, da, (DWORD*)rr);

   // 4.3 compute da * r mod n
   MP_MultMont(tmp3, tmp2, tmp1);

   // 4.4 let da = 1
   MP_Init(da);
   da[0]=1;

   // 4.5 convert to normal domain
   MP_MultMont(s, tmp3, da);

   /////////////////////////////////////////////////
   // step 5: compute e + da * r mod n
   /////////////////////////////////////////////////
   if(MP_Add(tmp3, s, e)!=0)
       MP_Sub(tmp3, tmp3, modn);

   /////////////////////////////////////////////////
   // step 6: compute s = k' * (e + da*r)
   /////////////////////////////////////////////////

   // 6.1 conver to montgomery domain
   MP_MultMont(tmp1, tmp3, (DWORD*)rr);

   // 6.2 convert k' to montgomery domain
   MP_MultMont(tmp2, kprime, (DWORD*)rr);

   // 6.3 compute k' * (e + da * r)
   MP_MultMont(tmp3, tmp1, tmp2);

   // 6.4 convert to normal domain
   MP_MultMont(s, tmp3, da);

   // 6.5 s should not be zero
   if(MP_isZero(s)) return FALSE;

   /////////////////////////////////////////////////
   // dump signature
   /////////////////////////////////////////////////
   for(i = 0; i < KEY_LENGTH_DWORDS*2; i++)
   {
       UINT32 tmp32;

       if(i < KEY_LENGTH_DWORDS)
           tmp32 = r[KEY_LENGTH_DWORDS - 1 - i];
       else
           tmp32 = s[2*KEY_LENGTH_DWORDS - 1 - i];

       signature[4*i] = (tmp32 >> 24) & 0xff;
       signature[4*i+1] = (tmp32 >> 16) & 0xff;
       signature[4*i+2] = (tmp32 >> 8) & 0xff;
       signature[4*i+3] = (tmp32 >> 0) & 0xff;
   }

   return TRUE;
}
