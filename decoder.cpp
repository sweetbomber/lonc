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

OcdDecoder::OcdDecoder(uint32_t dataSize, uint32_t blockSize, uint32_t q, double epsilon) {
	mRhoList.clear();
	mBlockStatus.clear();

	if((mBlockSolution = (uint8_t *) malloc(sizeof(uint8_t)*mBlockSize)) == NULL) {
		fprintf(stderr, "Error allocating memory\n");
		return;
	}
	mBlockSize = blockSize;
	mQ = q;
	SetEpsilon(epsilon);

	PreDecode(dataSize);
}

OcdDecoder::~OcdDecoder() {
	std::map<blockID_t, uint8_t *>::iterator checkBlockCache_it;
	/* Free cache */
	for(checkBlockCache_it = mCheckBlockCache.begin();
	    checkBlockCache_it != mCheckBlockCache.end();
	    checkBlockCache_it++) {

		free(checkBlockCache_it->second);
	}

	if(mIntermediateBuffer.buffer != NULL)
		free(mIntermediateBuffer.buffer);

	mCheckBlockCache.clear();
	mRhoList.clear();
	mBlockStatus.clear();
}

void OcdDecoder::PreDecode(uint32_t dataSize) {
	/* Calculate total number of intermediate blocks, including auxiliary blocks */
	mNumberOfSourceBlocks = ((dataSize + mBlockSize - 1)/mBlockSize);
	mNumberOfAuxiliaryBlocks = mNumberOfSourceBlocks*(mQ*mEpsilon*0.55);
	mNumberOfIntermediateBlocks = mNumberOfSourceBlocks + mNumberOfAuxiliaryBlocks;

	mSourceBuffer.bufferSize = dataSize;

	/* Allocate required memory */
	mIntermediateBuffer.bufferSize = (mNumberOfIntermediateBlocks)*mBlockSize;
	mIntermediateBuffer.buffer = (uint8_t *) malloc(sizeof(uint8_t)*mIntermediateBuffer.bufferSize);


	/* Reset buffers */
	memset(mIntermediateBuffer.buffer, 0, sizeof(uint8_t)*mIntermediateBuffer.bufferSize);
	mBlockStatus.assign(mNumberOfIntermediateBlocks, BLOCK_UNSOLVED);

	/* Setup the lists related with check-blocks "remaining" degrees, and relational table */
	mBlockRemainingDegreeList.clear();
	mRelatedBlockIDLists.clear();
	mRelatedBlockIDLists.resize(mNumberOfIntermediateBlocks);
	mNumberOfRemainingBlocks = mNumberOfIntermediateBlocks;
}

void OcdDecoder::FinishDecoding(uint8_t *outputBuffer) {
	/* This is highly preliminary. The outer decoding IS NOT BEING DONE */
	memcpy(outputBuffer, mIntermediateBuffer.buffer, mSourceBuffer.bufferSize);
}

uint32_t OcdDecoder::Decode(uint8_t *checkBlock, blockID_t blockID) {
	uint32_t degree, i;
	uint8_t *cache;
	std::map<blockID_t, uint32_t>::iterator blockIDList_it;
	uint32_t blockIndex;

	/* Checks if the given ID is new */
	blockIDList_it = mBlockRemainingDegreeList.find(blockID);
	if(blockIDList_it != mBlockRemainingDegreeList.end())
		return 0;

	/* Add checkBlock to cache */
	cache = (uint8_t *) malloc(sizeof(uint8_t)*mBlockSize);
	if(cache == NULL) {
		fprintf(stderr, "Memory error\n");
		return -1;
	}
	memcpy(cache, checkBlock, mBlockSize);
	mCheckBlockCache[blockID] = cache;


	/* Register block in respective lists */
	SeedCodec(blockID);
	degree = GetDegree();
	mBlockRemainingDegreeList[blockID] = degree;

	for(i = 0; i < degree; i++) {
		blockIndex = Random(mNumberOfIntermediateBlocks);

		/* Check if block is already solved */
		if(mBlockStatus[blockIndex] == BLOCK_SOLVED) {
			/* Decrease checkBlock degree */
			mBlockRemainingDegreeList[blockID]--;
		}
		else {
			/* Register the checkBlock ID in the blocks it relates to */
			mRelatedBlockIDLists[blockIndex].push_back(blockID);
		}
	}

	/* Release the Kraken! (basicaly, solve the equations) */
	RecursiveDecode(blockID);

	return 0;
}


void OcdDecoder::RecursiveDecode(blockID_t blockID) {
	uint32_t blockIndex, unsolvedBlockIndex;
	uint8_t *solvedBlock; /* Used to point to already known blocks */
	uint32_t degree, degree_it;
	std::list<blockID_t>::iterator blockID_it;
	uint32_t numberOfUnsolvedBlocks;

	/* Check if remaining degree is 1, so that we can derive
	   information from it right now. If not, return immediately. */
	if(mBlockRemainingDegreeList[blockID] != 1)
		return;

	/* Set solution holder to the contents of the checkBlock */
	memcpy(mBlockSolution, mCheckBlockCache[blockID], mBlockSize);

	/* Sweep all blocks related to this check block, and xor them together.
	   additionally, look for the block which has not been solved yet,
	   so that we could know where to commit the solution */
	SeedCodec(blockID);
	degree = GetDegree();

	numberOfUnsolvedBlocks = 0;
	for(degree_it = 0; degree_it < degree; degree_it++) {
		blockIndex = Random(mNumberOfIntermediateBlocks);

		if(mBlockStatus[blockIndex] == BLOCK_SOLVED) {
			/* Get block pointer */
			GetBlock(mIntermediateBuffer.buffer, blockIndex, &solvedBlock);
			BlockXor(mBlockSolution, solvedBlock);
		}
		else {
			numberOfUnsolvedBlocks++;
			unsolvedBlockIndex = blockIndex;
		}
	}

	/* At this point, blockSolution contains the solution for the
	   block at unsolvedBlockIndex. Just commit it */
	GetBlock(mIntermediateBuffer.buffer, unsolvedBlockIndex, &solvedBlock);
	memcpy(solvedBlock, mBlockSolution, mBlockSize);

	/* Now the update stage.
	   First step: Flag solvedBlockIndex as "BLOCK_SOLVED" and update
		numberOfRemainingBlocks */
	mBlockStatus[unsolvedBlockIndex] = BLOCK_SOLVED;
	mNumberOfRemainingBlocks--;

	/* Second step: decrease by one degree all checkBlocks related to
	   the solved one. */
	for(blockID_it = mRelatedBlockIDLists[unsolvedBlockIndex].begin();
		blockID_it != mRelatedBlockIDLists[unsolvedBlockIndex].end();
		blockID_it++) {

		if(mBlockRemainingDegreeList[*blockID_it] > 0)
			mBlockRemainingDegreeList[*blockID_it]--;
	}

	/* Third step: evaluate recursively those blocks which have degree equal to 1 */
	for(blockID_it = mRelatedBlockIDLists[unsolvedBlockIndex].begin();
		blockID_it != mRelatedBlockIDLists[unsolvedBlockIndex].end();
		blockID_it++) {

		if(mBlockRemainingDegreeList[*blockID_it] == 1)
			RecursiveDecode(*blockID_it);
	}

	/* Fourth step (optional): clear list of relational blocks */
	mRelatedBlockIDLists[unsolvedBlockIndex].clear();
}

uint32_t OcdDecoder::GetNumberOfRemainingBlocks() {
	return mNumberOfRemainingBlocks;
}
