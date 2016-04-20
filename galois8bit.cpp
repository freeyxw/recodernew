#include <stdio.h>
#include "galois8bit.h"

#define NUM 8
#define MASK 0x80
#define SIZE 255
#define POLYNOMIAL (1<<4|1<<3|1<<2|1)

UInt8_t GaloisValue[1 << NUM];
UInt8_t GaloisIndex[1 << NUM];

void galoisEightBitInit(void) {
	int i, j = 1;
	for (i = 0; i < (1<<NUM)-1; ++i) {
		GaloisValue[i] = j;
		GaloisIndex[GaloisValue[i]] = i;
		if (j & MASK) {
			j <<= 1;
			j ^= POLYNOMIAL;
		}
		else {
			j <<= 1;
		}
	}
}

UInt8_t galoisAdd(UInt8_t A, UInt8_t B) {
	return A ^ B;
}

UInt8_t galoisSub(UInt8_t A, UInt8_t B) {
	return A ^ B;
}

UInt8_t galoisMul(UInt8_t A, UInt8_t B) {
	if (!A || !B) {
		return 0;
	}
	UInt8_t i = GaloisIndex[A];
	UInt8_t j = GaloisIndex[B];
	UInt8_t index = (i+j)%SIZE;
	return GaloisValue[index];
}

UInt8_t galoisDiv(UInt8_t A, UInt8_t B) {
	if (!A || !B) {
		return 0;
	}
	return galoisMul(A, galoisInv(B));
}

UInt8_t galoisPow(UInt8_t A, UInt8_t B) {
	if (!A) {
		return 0;
	}
	if (!B) {
		return 1;
	}
	UInt8_t i, r = 1;
	for (i = 0; i < B; ++i) {
		r = galoisMul(r, A);
	}
	return r;
}

UInt8_t galoisInv(UInt8_t A) {
	if (!A) {
		return 0;
	}
	UInt8_t j = GaloisIndex[A];
	return GaloisValue[(SIZE-j)%SIZE];
}

