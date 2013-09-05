/*
    
    Copyright 2013 sweetbomber (ffrogger0@yahoo.com)

    This file is part of the Library of Online-Codes (LOnC).

    The Library of Online-Codes is free software:
		you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Library of Online-Codes is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Library of Online-Codes.
    If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <math.h>
#include "codec.h"


/* Public methods */

void OcdCodecBase::SeedCodec(blockID_t blockID) {
	srand((uint32_t) blockID);
}

blockID_t OcdCodecBase::GenerateBlockID(uint32_t seed) {
	srand(seed);
	return (blockID_t) rand();
}

uint32_t OcdCodecBase::GetNumberOfIntermediateBlocks() {
	return mNumberOfIntermediateBlocks;
}

uint32_t OcdCodecBase::GetNumberOfAuxiliaryBlocks() {
	return mNumberOfAuxiliaryBlocks;
}

uint32_t OcdCodecBase::GetNumberOfSourceBlocks() {
	return mNumberOfSourceBlocks;
}

/* Private methods */

void OcdCodecBase::GetBlock(uint8_t *data, uint32_t blockIndex, uint8_t **block) {
	*block = &(data[blockIndex*mBlockSize]);
}

void OcdCodecBase::CalculateDegreeDistribuition() {
	uint32_t i;
	mRhoList[0] = Rho(1, mMaxDegree, mEpsilon);

	for(i = 2; i < mMaxDegree; i++)
		mRhoList[i-1] = mRhoList[i-2] + Rho(i, mMaxDegree, mEpsilon);
}

uint32_t OcdCodecBase::GetDegree() {
	uint32_t degree;
	double randomValue = ((double) rand())/((double) RAND_MAX);

	for(degree = 1; (randomValue > mRhoList[degree - 1]) && (degree < mMaxDegree); degree++);

	return degree;
}

double OcdCodecBase::Rho(uint32_t index, uint32_t F, double e) {
	double rho1 = 1.0 - (1.0 + 1.0/F)/(1.0 + mEpsilon);

	if(index == 1) {
		return rho1;
	}
	else {
		return (1 - rho1)*F/((F - 1.0)*index*(index - 1.0));
	}
}

void OcdCodecBase::SetEpsilon(double e) {
	mEpsilon = e;
	mRhoList.clear();
	
	mMaxDegree = CalculateMaxDegree(mEpsilon);
	mRhoList.resize(mMaxDegree);
	CalculateDegreeDistribuition();
}

uint32_t OcdCodecBase::CalculateMaxDegree(double e) {
	return (uint32_t) ceil(log(e*e/4)/(log(1 - e/2)));
}

void OcdCodecBase::BlockXor(uint8_t *dstBlock, uint8_t *srcBlock) {
	uint32_t i;
	for(i = 0; i < mBlockSize; i++)
		dstBlock[i] ^= srcBlock[i];
}

uint32_t OcdCodecBase::Random(uint32_t modulus) {
	uint32_t returnValue, mod;

	#ifdef FAST_RANDOM
		return rand() % modulus;
	#else
		/* rand() % modulus does not yield a uniform distibuition.
	   To get an uniform distr., keep generating random values until they fall within desired interval */

		/* Calculate nearest submultiple of RAND_MAX */
		mod = (RAND_MAX + 1);
		while(mod > modulus)
			mod = mod >> 1;
	
		mod = mod << 1;

		/* Shooting random :p */
		for(returnValue = rand() % mod; returnValue >= modulus; returnValue = rand() % mod);

		return returnValue;
	#endif
}
