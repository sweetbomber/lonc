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

#include <map>
#include <list>
#include <vector>

#ifndef __CODEC_H__
#define __CODEC_H__

/* Who cares about an uniform PRNG? */
#define FAST_RANDOM

/* Default character to use as filler */
#define DEFAULT_PADDING 0

#define BLOCK_SOLVED true
#define BLOCK_UNSOLVED false

typedef uint32_t blockID_t;

typedef struct buffer_struct_t {
	uint8_t *buffer;
	uint32_t bufferSize;
} buffer_t;

class OcdCodecBase {
	public: 
		/* Generate a new BlockID, given a seed */
		blockID_t GenerateBlockID(uint32_t seed);

		/* Seeds the encoder, given a blockID as seed*/
		void SeedCodec(blockID_t blockID);

		/* A few accessors */
		uint32_t GetNumberOfSourceBlocks();
		uint32_t GetNumberOfIntermediateBlocks();
		uint32_t GetNumberOfAuxiliaryBlocks();

	protected:
		void CalculateDegreeDistribuition();
		double Rho(uint32_t index, uint32_t F, double e);

		void SetEpsilon(double e);
		uint32_t CalculateMaxDegree(double e);
		uint32_t GetDegree();


		/* Get the block given its index. */
		void GetBlock(uint8_t *data, uint32_t blockIndex, uint8_t **block);

		/* 0 <= PRNG < modulus.  Use SeedCodec() to seed it */
		uint32_t Random(uint32_t modulus);

		/* Exclusive-OR between two blocks. Store result in *dst. */
		void BlockXor(uint8_t *dstBlock, uint8_t *srcBlock);

	/* Atributes */
		uint32_t mNumberOfSourceBlocks; /* Number of blocks related to data */
		uint32_t mNumberOfAuxiliaryBlocks; /* Number of auxiliary blocks (after outer encoding) */
		uint32_t mNumberOfIntermediateBlocks; /* The sum of the two above this one */

		double mEpsilon;
		uint32_t mQ;

		/* Cumulative probability distribuition of block degrees */
		std::vector<double> mRhoList;
		uint32_t mMaxDegree;

		/* sourceBuffer stores the original data, either
		   before outer encoding, or after outer decoding */
		buffer_t mSourceBuffer;

		/* intermediateBuffer stores the outer-encoding data,
		   either before inner encoding, or after outer decoding */
		buffer_t mIntermediateBuffer;

		uint32_t mBlockSize;
};

class OcdEncoder: public OcdCodecBase {
	public:
		OcdEncoder(const uint8_t *inputBuffer, uint32_t inputBufferSize, uint32_t blockSize, uint32_t q, double epsilon);
		~OcdEncoder();

		/* Encode a checkBlock */
		void Encode(uint8_t *checkBlock, blockID_t blockID);

	private:
		/* Perform outer encoding */
		void PreEncode();
};

class OcdDecoder: public OcdCodecBase {
	public:
		OcdDecoder(uint32_t dataSize, uint32_t blockSize, uint32_t q, double epsilon);
		~OcdDecoder();

		/* Decode a checkBlock */
		uint32_t Decode(uint8_t *checkBlock, blockID_t blockID);

		/* Return number of unsolved blocks */
		uint32_t GetNumberOfRemainingBlocks();

		/* Copy result from internal buffer to user's buffer */
		void FinishDecoding(uint8_t *outputBuffer);

	private:
		void PreDecode(uint32_t dataSize);

		void RecursiveDecode(blockID_t blockID);

		/* This associative list stores the "remaining degree" of all received blocks.
		   0 means that the block has already been used */
		std::map<blockID_t, uint32_t> mBlockRemainingDegreeList;

		/* This is an array cointaining one list per each
		   intermediary block.
		   The idea is to keep a list, for each interm. block, that contains the
		   blockID's of other check blocks that also relate to it */
		std::vector< std::list<blockID_t> > mRelatedBlockIDLists;

		/* Used to track, during decoding, the intermediate blocks
		   that are completed */
		std::vector<uint8_t> mBlockStatus;
		uint32_t mNumberOfRemainingBlocks;
		std::map<blockID_t, uint8_t *> mCheckBlockCache;

		uint8_t *mBlockSolution; /* Used to temporarly store the block solution */
};
#endif
//__CODEC_H__
