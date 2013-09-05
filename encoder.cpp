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

#include <boost/random/uniform_real.hpp>
#include <math.h>
#include "codec.h"


OcdEncoder::OcdEncoder(const uint8_t *inputBuffer,
					   uint32_t inputBufferSize,
					   uint32_t blockSize,
					   uint32_t q,
					   double epsilon) {

	mRhoList.clear();

	mBlockSize = blockSize;
	mQ = q;
	SetEpsilon(epsilon);

	/* Calculate total memory, including auxiliary blocks */
	mNumberOfSourceBlocks = ((inputBufferSize + mBlockSize - 1)/mBlockSize);
	mNumberOfAuxiliaryBlocks = mNumberOfSourceBlocks*(mQ*mEpsilon*0.55);
	mNumberOfIntermediateBlocks = mNumberOfSourceBlocks + mNumberOfAuxiliaryBlocks;

	/* Redirect source data */
	mSourceBuffer.buffer = (uint8_t *)inputBuffer;
	mSourceBuffer.bufferSize = inputBufferSize;

	/* Allocate required memory */
	mIntermediateBuffer.bufferSize = mNumberOfIntermediateBlocks*mBlockSize;
	mIntermediateBuffer.buffer = (uint8_t *) malloc(sizeof(uint8_t)*mIntermediateBuffer.bufferSize);

	/* Copy source data to intermediate data, and fill remaining space */
	memcpy(mIntermediateBuffer.buffer, inputBuffer, inputBufferSize);
	memset((mIntermediateBuffer.buffer) + inputBufferSize, 0, mIntermediateBuffer.bufferSize - inputBufferSize);

	PreEncode();
}

OcdEncoder::~OcdEncoder() {
	free(mIntermediateBuffer.buffer);
	mRhoList.clear();
}

void OcdEncoder::PreEncode() {
	uint32_t blockIdx, qCounter;
	uint8_t *srcBlock, *dstBlock;
	
	SeedCodec(1); /* Ill probably let the seed to be a free parameter */

	for(blockIdx = 0; blockIdx < mNumberOfSourceBlocks; blockIdx++) {
		GetBlock(mIntermediateBuffer.buffer, blockIdx, &srcBlock);

		for(qCounter = 0; qCounter < mQ; qCounter++) {
			GetBlock(mIntermediateBuffer.buffer, mNumberOfSourceBlocks + Random(mNumberOfAuxiliaryBlocks), &dstBlock);
			BlockXor(dstBlock, srcBlock);
		}
	}
}

void OcdEncoder::Encode(uint8_t *checkBlock, blockID_t blockID) {
	uint32_t degree, i;
	uint8_t *srcBlock;

	SeedCodec(blockID);

	degree = GetDegree();

	/* Clear checkBlock */
	memset(checkBlock, 0, mBlockSize);

	/* Pick 'degree' random blocks, and xor them. */
	for(i = 0; i < degree; i++) {
		/* Get a block */
		GetBlock(mIntermediateBuffer.buffer, Random(mNumberOfIntermediateBlocks), &srcBlock);

		/* Xor srcBlock with checkblock, and store the result in the later */
		BlockXor(checkBlock, srcBlock);
	}
	printf("ID: %08X  deg: %d\n", blockID, degree);
}
