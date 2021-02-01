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
 *    File Name: multprecision.c
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
#include <string.h>
#include "multprecision.h"
#include "ecc_pp.h"

void MP_Init(DWORD *c)
{
	int i;

	for(i=0; i<KEY_LENGTH_DWORDS; i++)
		c[i]=0;
}

void MP_Copy(DWORD *c, DWORD *a)
{
	int i;

	for(i=0; i<KEY_LENGTH_DWORDS; i++)
	    c[i]=a[i];
}

int MP_CMP(DWORD *a, DWORD *b)
{
	int i;
	int NumDWORDs=KEY_LENGTH_DWORDS;

	for(i=NumDWORDs-1; i>=0; i--)
	{
		if(a[i]<b[i])
			return -1;
		if(a[i]>b[i])
			return 1;
	}
	return 0;
}

int MP_isZero(DWORD *a)
{
	int i;
	int NumDWORDs=KEY_LENGTH_DWORDS;

	for(i=0; i<NumDWORDs; i++)
		if(a[i])
			return 0;

	return 1;
}

UINT32 MP_DWORDBits (DWORD a)
{
	int i;

	for (i = 0; i < DWORD_BITS; i++, a >>= 1)
      if (a == 0)
        break;

    return i;
}

UINT32 MP_MostSignDWORDs(DWORD *a)
{
	int i;
	int NumDWORDs=KEY_LENGTH_DWORDS;

	for(i=NumDWORDs-1; i>=0; i--)
		if(a[i])
			break;
	return (i+1);
}

UINT32 MP_MostSignBits(DWORD *a)
{
	int aMostSignDWORDs;

	aMostSignDWORDs=MP_MostSignDWORDs(a);
	if(aMostSignDWORDs==0)
		return 0;

	return ( ((aMostSignDWORDs-1)<<DWORD_BITS_SHIFT)+MP_DWORDBits(a[aMostSignDWORDs-1]) );
}


DWORD MP_Add(DWORD *c, DWORD *a, DWORD *b)
{
	int i;
	DWORD carrier;
	DWORD temp;

	carrier=0;
	for(i=0; i<KEY_LENGTH_DWORDS; i++)
	{
		temp=a[i]+carrier;
		carrier = (temp<carrier);
		temp += b[i];
		carrier |= (temp<b[i]);
		c[i]=temp;
	}

	return carrier;
}

//c=a-b
DWORD MP_Sub(DWORD *c, DWORD *a, DWORD *b)
{
	int i;
	DWORD borrow;
	DWORD temp;

	borrow=0;
	for(i=0; i<KEY_LENGTH_DWORDS; i++)
	{
		temp=a[i]-borrow;
		borrow=(temp>a[i]);
		c[i] = temp-b[i];
		borrow |= (c[i]>temp);
	}

	return borrow;
}


// c=a*b; c must have a buffer of 2*Key_LENGTH_DWORDS, c != a != b
void MP_Mult(DWORD *c, DWORD *a, DWORD *b)
{
	int i, j;
	DWORD W, U, V;

    memset(c, 0, KEY_LENGTH_BYTES*2);

	//assume little endian right now
	for(i=0; i<KEY_LENGTH_DWORDS; i++)
	{
		U = 0;
		for(j=0; j<KEY_LENGTH_DWORDS; j++)
        {
            {
                UINT64 result;
                result = ((UINT64)a[i]) * ((UINT64)b[j]);
                W = result >> 32;
                V= (DWORD)result;
            }

			V=V+U;
			U=(V<U);
			U+=W;

			V = V+c[i+j];
			U += (V<c[i+j]);

			c[i+j] = V;
		}
		c[i+KEY_LENGTH_DWORDS]=U;
	}
}

void MP_FastMod_P256(DWORD *c, DWORD *a)
{
    DWORD A, B, C, D, E, F, G;
    UINT8 UA, UB, UC, UD, UE, UF, UG;
    DWORD U;

    // C = a[13] + a[14] + a[15];
    C = a[13];
    C += a[14];
    UC = (C < a[14]);
    C += a[15];
    UC += (C < a[15]);

    // E = a[8] + a[9];
    E = a[8];
    E += a[9];
    UE = (E < a[9]);

    // F = a[9] + a[10];
    F = a[9];
    F += a[10];
    UF = (F < a[10]);

    // G = a[10] + a[11]
    G = a[10];
    G += a[11];
    UG = (G < a[11]);

    // B = a[12] + a[13] + a[14] + a[15] == C + a[12]
    B = C;
    UB = UC;
    B += a[12];
    UB += (B < a[12]);

    // A = a[11] + a[12] + a[13] + a[14] == B + a[11] - a[15]
    A = B;
    UA = UB;
    A += a[11];
    UA += (A < a[11]);
    UA -= (A < a[15]);
    A -= a[15];

    // D = a[10] + a[11] + a[12] + a[13] == A + a[10] - a[14]
    D = A;
    UD = UA;
    D += a[10];
    UD += (D < a[10]);
    UD -= (D < a[14]);
    D -= a[14];

    c[0] = a[0];
    c[0] += E;
    U = (c[0] < E);
    U += UE;
    U -= (c[0] < A);
    U -= UA;
    c[0] -= A;

    if(U & 0x80000000)
    {
        DWORD UU;
        UU = 0 - U;
        U = (a[1] < UU);
        c[1] = a[1] - UU;
    }
    else
    {
        c[1] = a[1] + U;
        U = (c[1] < a[1]);
    }

    c[1] += F;
    U += (c[1] < F);
    U += UF;
    U -= (c[1] < B);
    U -= UB;
    c[1] -= B;

    if(U & 0x80000000)
    {
        DWORD UU;
        UU = 0 - U;
        U = (a[2] < UU);
        c[2] = a[2] - UU;
    }
    else
    {
        c[2] = a[2] + U;
        U = (c[2] < a[2]);
    }

    c[2] += G;
    U += (c[2] < G);
    U += UG;
    U -= (c[2] < C);
    U -= UC;
    c[2] -= C;

    if(U & 0x80000000)
    {
        DWORD UU;
        UU = 0 - U;
        U = (a[3] < UU);
        c[3] = a[3] - UU;
    }
    else
    {
        c[3] = a[3] + U;
        U = (c[3] < a[3]);
    }

    c[3] += A;
    U += (c[3] < A);
    U += UA;
    c[3] += a[11];
    U += (c[3] < a[11]);
    c[3] += a[12];
    U += (c[3] < a[12]);
    U -= (c[3] < a[14]);
    c[3] -= a[14];
    U -= (c[3] < a[15]);
    c[3] -= a[15];
    U -= (c[3] < E);
    U -= UE;
    c[3] -= E;

    if(U & 0x80000000)
    {
        DWORD UU;
        UU = 0 - U;
        U = (a[4] < UU);
        c[4] = a[4] - UU;
    }
    else
    {
        c[4] = a[4] + U;
        U = (c[4] < a[4]);
    }

    c[4] += B;
    U += (c[4] < B);
    U += UB;
    U -= (c[4] < a[15]);
    c[4] -= a[15];
    c[4] += a[12];
    U += (c[4] < a[12]);
    c[4] += a[13];
    U += (c[4] < a[13]);
    U -= (c[4] < F);
    U -= UF;
    c[4] -= F;

    if(U & 0x80000000)
    {
        DWORD UU;
        UU = 0 - U;
        U = (a[5] < UU);
        c[5] = a[5] - UU;
    }
    else
    {
        c[5] = a[5] + U;
        U = (c[5] < a[5]);
    }

    c[5] += C;
    U += (c[5] < C);
    U += UC;
    c[5] += a[13];
    U += (c[5] < a[13]);
    c[5] += a[14];
    U += (c[5] < a[14]);
    U -= (c[5] < G);
    U -= UG;
    c[5] -= G;

    if(U & 0x80000000)
    {
        DWORD UU;
        UU = 0 - U;
        U = (a[6] < UU);
        c[6] = a[6] - UU;
    }
    else
    {
        c[6] = a[6] + U;
        U = (c[6] < a[6]);
    }

    c[6] += C;
    U += (c[6] < C);
    U += UC;
    c[6] += a[14];
    U += (c[6] < a[14]);
    c[6] += a[14];
    U += (c[6] < a[14]);
    c[6] += a[15];
    U += (c[6] < a[15]);
    U -= (c[6] < E);
    U -= UE;
    c[6] -= E;

    if(U & 0x80000000)
    {
        DWORD UU;
        UU = 0 - U;
        U = (a[7] < UU);
        c[7] = a[7] - UU;
    }
    else
    {
        c[7] = a[7] + U;
        U = (c[7] < a[7]);
    }

    c[7] += a[15];
    U += (c[7] < a[15]);
    c[7] += a[15];
    U += (c[7] < a[15]);
    c[7] += a[15];
    U += (c[7] < a[15]);
    c[7] += a[8];
    U += (c[7] < a[8]);
    U -= (c[7] < D);
    U -= UD;
    c[7] -= D;

    if(U & 0x80000000)
    {
        while(U)
        {
            MP_Add(c, c, modp);
            U++;
        }
    }
    else if(U)
    {
        while(U)
        {
            MP_Sub(c, c, modp);
            U--;
        }
    }

    if(MP_CMP(c, modp)>=0)
        MP_Sub(c, c, modp);

}

void MP_LShiftMod(DWORD * c, DWORD * a)
{
	DWORD carrier;

	carrier=MP_LShift(c, a);
	if(carrier)
        MP_Sub(c, c, modp);
	else
    {
        if(MP_CMP(c, modp)>=0)
            MP_Sub(c, c, modp);
    }
}

DWORD MP_LShift(DWORD * c, DWORD * a)
{
	DWORD carrier;
	int i, j;
	DWORD temp;
    UINT32 b = 1;

	j=DWORD_BITS-b;
	carrier=0;

	for(i=0; i<KEY_LENGTH_DWORDS; i++)
	{
		temp=a[i];								// in case c==a
		c[i]=(temp<<b) | carrier;
		carrier = temp >> j;
	}

	return carrier;
}

void MP_RShift(DWORD * c, DWORD * a)
{
	DWORD carrier;
	int i, j;
	DWORD temp;
	int NumDWORDs=KEY_LENGTH_DWORDS;
	DWORD b = 1;

	j=DWORD_BITS-b;
	carrier=0;

	for(i=NumDWORDs-1; i>=0; i--)
	{
		temp=a[i];				// in case of c==a
		c[i]=(temp>>b) | carrier;
		carrier = temp << j;
	}
}

// Curve specific optimization when p is a pseudo-Mersenns prime, p=2^(KEY_LENGTH_BITS)-omega
void MP_MersennsMultMod(DWORD *c, DWORD *a, DWORD *b)
{
	DWORD cc[2*KEY_LENGTH_DWORDS];

	MP_Mult(cc, a, b);

	MP_FastMod_P256(c, cc);
}



// Curve specific optimization when p is a pseudo-Mersenns prime
void MP_MersennsSquaMod(DWORD *c, DWORD *a)
{
	MP_MersennsMultMod(c, a, a);
}

void MP_AddMod(DWORD *c, DWORD *a, DWORD *b)
{
	DWORD carrier;

	carrier=MP_Add(c, a, b);
	if(carrier)
    {
        MP_Sub(c, c, modp);
    }
	else if(MP_CMP(c, modp)>=0)
    {
        MP_Sub(c, c, modp);
    }
}

void MP_SubMod(DWORD *c, DWORD *a, DWORD *b)
{
	DWORD borrow;

	borrow=MP_Sub(c, a, b);
	if(borrow)
		MP_Add(c, c, modp);
}

void MP_InvMod(DWORD *aminus, DWORD *u, const DWORD* modulus)
{
	DWORD v[KEY_LENGTH_DWORDS];
	DWORD A[KEY_LENGTH_DWORDS+1], C[KEY_LENGTH_DWORDS+1];

	MP_Copy(v, (DWORD*)modulus);
	MP_Init(A);
	MP_Init(C);
	A[0]=1;

	while(!MP_isZero(u))
	{
		while( !(u[0] & 0x01) )					// u is even
		{
			MP_RShift(u, u);
			if( !(A[0] & 0x01) )					// A is even
				MP_RShift(A, A);
			else
			{
				A[KEY_LENGTH_DWORDS]=MP_Add(A, A, (DWORD*)modulus);	// A =A+p
				MP_RShift(A, A);
				A[KEY_LENGTH_DWORDS-1] |= (A[KEY_LENGTH_DWORDS]<<31);
			}
		}

		while( !(v[0] & 0x01) )					// v is even
		{
			MP_RShift(v, v);
			if( !(C[0] & 0x01) )					// C is even
				MP_RShift(C, C);
			else
			{
				C[KEY_LENGTH_DWORDS]=MP_Add(C, C, (DWORD*)modulus);	// C =C+p
				MP_RShift(C, C);
				C[KEY_LENGTH_DWORDS-1] |= (C[KEY_LENGTH_DWORDS]<<31);
			}
		}

		if(MP_CMP(u, v)>=0)
		{
			MP_Sub(u, u, v);
            if(MP_Sub(A, A, C))
                MP_Add(A, A, (DWORD*)modulus);
        }
		else
		{
			MP_Sub(v, v, u);
            if(MP_Sub(C, C, A))
                MP_Add(C, C, (DWORD*)modulus);
		}
	}

	if(MP_CMP(C, (DWORD*)modulus)>=0)
    {
		MP_Sub(aminus, C, (DWORD*)modulus);
    }
	else
    {
		MP_Copy(aminus, C);
    }
}


DWORD MP_LAdd(DWORD *c, DWORD *a, DWORD *b)
{
	int i;
	DWORD carrier;
	DWORD temp;

	carrier=0;
	for(i=0; i<KEY_LENGTH_DWORDS*2; i++)
	{
		temp=a[i]+carrier;
		carrier = (temp<carrier);
		temp += b[i];
		carrier |= (temp<b[i]);
		c[i]=temp;
	}

	return carrier;
}

// Montgomery reduction
void MP_MontReduction(DWORD *q, DWORD* c)
{
    UINT32 a[KEY_LENGTH_DWORDS*2];
    UINT32 y[KEY_LENGTH_DWORDS*2];
    BOOL32 carry = 0;

    // q = c mod r
    memcpy(q, c, KEY_LENGTH_BYTES);

    // y = (c mod r) * nprime
	MP_Mult(y, q, (DWORD*)nprime);

    // q = ((c mod r ) * nprime) mod r
    memcpy(q, y, KEY_LENGTH_BYTES);

    // y = q * n
	MP_Mult(y, q, modn);

    // a = c + q * n
    if(MP_LAdd(a, c, y))
        carry = 1;

    // q = (c + qn) / r
    memcpy(q, a+KEY_LENGTH_DWORDS, KEY_LENGTH_BYTES);

    // if q > n then q -= n
    if(carry || MP_CMP(q, modn) > 0)
        MP_Sub(q, q, modn);
}

void MP_MultMont(DWORD *c, DWORD *a, DWORD *b)
{
	DWORD cc[2*KEY_LENGTH_DWORDS];
	MP_Mult(cc, a, b);
    MP_MontReduction(c, cc);
}
