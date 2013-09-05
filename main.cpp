/*
    
    Copyright 2013 sweetbomber (ffrogger0@yahoo.com)

    This program is not part of LOnC, but rather a sample
    application that uses it.

*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <dirent.h>

#include "codec.h"

#define MAX_PATH 256
#define DEFAULT_BLOCK_SIZE 256
#define Q_CONSTANT 10
#define EPSILON_CONSTANT 0.01
/*----------------------------------------------------------------------------*/
typedef struct options_struct {
	char outputFile[MAX_PATH];
	char inputFile[MAX_PATH];
	bool verbose;
	uint32_t blockSize;
	bool usage;
	bool encode;
	bool decode;
} options_t;


bool verbose;

/* Get program options. Return != 0 if usage screen is to be shown. */
int getOptions(int argc, char *argv[], options_t *options) {

	bool showUsage = false;
	int n;
	/* Set defaults */
	options->outputFile[0] = 0;
	options->inputFile[0] = 0;
	options->verbose = false;
	options->blockSize = DEFAULT_BLOCK_SIZE;
	options->usage = false;
	options->encode = false;
	options->decode = false;

  for(n = 1 ; n < argc ; n++)
    if(!strcmp("-bs", argv[n]))
    {
      options->blockSize = atoi(argv[n+1]);
      if(options->blockSize == 0) {
				fprintf(stderr, "Warning (blockSize): 0 value found. Using default: %d\n",
																												DEFAULT_BLOCK_SIZE);
				options->blockSize = DEFAULT_BLOCK_SIZE;
			}
      break;
    }

  for(n = 1 ; n < argc ; n++)
    if(strcmp("-o", argv[n]) == 0)
    {
			strncpy(options->outputFile, argv[n+1], MAX_PATH);
      break;
    }
	if(options->outputFile[0] == 0) {
		fprintf(stderr, "Warning (outputFile): No output file specified. Using default.\n");
		strcpy(options->outputFile, "output_");
	}
	
	for(n = 1 ; n < argc ; n++)
		if(!strcmp("-v", argv[n]))
		{
			options->verbose = true;
      break;
    }

	for(n = 1 ; n < argc ; n++)
		if(!strcmp("--help", argv[n]))
		{
			options->usage = true;
      break;
    }

	for(n = 1 ; n < argc ; n++)
		if(!strcmp("-d", argv[n]))
		{
			options->decode = true;
      break;
    }

	for(n = 1 ; n < argc ; n++)
		if(!strcmp("-e", argv[n]))
		{
			options->encode = true;
      break;
    }


  for(n = 1 ; n < argc ; n++)
    if(strcmp("-i", argv[n]) == 0)
    {
			strncpy(options->inputFile, argv[n+1], MAX_PATH);
      break;
    }
	if(options->inputFile[0] == 0) {
		fprintf(stderr, "Warning (inputFile): No input file specified.\n");
	}
	
	/* Evaluate if usage should be shown */
	if(options->inputFile[0] == 0)
		showUsage = true;

	if(options->usage)
		showUsage = true;

  return showUsage;	
}

/* Print program usage options */
void displayUsage() {
	fprintf(stderr, "Usage: raptor  [ -o  encodedFileName (encoding) or outputFile (decoding)  (def.: output_ ) ]\n");
	fprintf(stderr, "               [ -i  inputFile (encoding) or inputPrefix (decoding) ]\n");
	fprintf(stderr, "               [ -bs blockSize (def: %d)]\n", DEFAULT_BLOCK_SIZE);
	fprintf(stderr, "               [ -e  encode ]\n");
	fprintf(stderr, "               [ -d  decode ]\n");
	fprintf(stderr, "               [ -v  verbose    (def: no)]\n");
	fprintf(stderr, "               [ --help Usage screen ]\n");
	fprintf(stderr, "\n");
}

/* Save header file */
int saveHeader(char *outputPrefix, uint32_t outputSize) {
	FILE *outputFid;
	char outputFileName[MAX_PATH];	
	
	/* Generate filename */
	sprintf(outputFileName, "%sheader.blk", outputPrefix);

	/* Open file for writing */
	if(!(outputFid = fopen(outputFileName, "w"))) {
		fprintf(stderr, "Error!\n");
		return 1;
	}
	
	/* Write data to file */
	fprintf(outputFid, "%d\n", outputSize);

	fclose(outputFid);

	return 0;
}

/* Load header file */
int loadHeader(char *outputPrefix, uint32_t *fileSize) {
	FILE *headerFid;
	char headerFileName[MAX_PATH];	
	
	/* Generate filename */
	sprintf(headerFileName, "%sheader.blk", outputPrefix);

	/* Open file for reading */
	if(!(headerFid = fopen(headerFileName, "r"))) {
		fprintf(stderr, "Error opening header file!\n");
		return 1;
	}
	
	/* Read data from file */
	fscanf(headerFid, "%d\n", fileSize);

	fclose(headerFid);

	return 0;
}

int saveBlock(char *outputPrefix, blockID_t blockID, uint8_t *outputBuffer, uint32_t outputSize) {
	FILE *outputFid;
	char outputFileName[MAX_PATH];	
	
	/* Generate filename */
	sprintf(outputFileName, "%s%08X.blk", outputPrefix, blockID);

	/* Open file for writing */
	if(!(outputFid = fopen(outputFileName, "w"))) {
		fprintf(stderr, "Error!\n");
		return 1;
	}
	
	/* Write data to file */
	fwrite(outputBuffer, sizeof(int8_t), outputSize, outputFid);

	fclose(outputFid);

	return 0;
}

int loadBlock(char *inputFileName, blockID_t *blockID, uint8_t *outputBuffer, uint32_t outputSize) {
	// TODO: It is all wrong (still from saveBlock)
	FILE *inputFid;
	
	/* Open file for writing */
	if(!(inputFid = fopen(inputFileName, "r"))) {
		fprintf(stderr, "Error!\n");
		return 1;
	}
	
	/* Read data from file */
	fread(blockID, sizeof(blockID_t), 1, inputFid);
	fread(outputBuffer, sizeof(int8_t), outputSize, inputFid);

	fclose(inputFid);

	return 0;
}

int encodeData(OcdEncoder &encoder, char *outputPrefix) {
	uint8_t *checkBlock;
	uint8_t *outputBuffer;
	uint32_t outputSize;
	blockID_t	*blockID;
	uint32_t counter;

	
	/* Allocate memory for the output buffer.
	   It contains the blockID and checkBlock data. REMOVE DEFAULT BLOCK SIZE*/
	outputSize = sizeof(blockID_t) + DEFAULT_BLOCK_SIZE*sizeof(uint8_t);
	outputBuffer = (uint8_t *) malloc(outputSize);
	if(outputBuffer == NULL) {
		fprintf(stderr, "Memory error. encodeData -> outputBuffer\n");
		return 1;
	}

	blockID = (blockID_t *) outputBuffer;
	checkBlock = outputBuffer + sizeof(blockID_t);

	if(verbose)
		fprintf(stderr, "Generating %d blocks. Total: %d. Theoretic minimum %f\n", 3*encoder.GetNumberOfIntermediateBlocks(),
																				   encoder.GetNumberOfIntermediateBlocks(),
																				   (1.0 + EPSILON_CONSTANT)*encoder.GetNumberOfIntermediateBlocks());

	/* Generate twice the number of blocks */
	*blockID = encoder.GenerateBlockID(time(NULL));
	
	for(counter = 0; counter < 3*encoder.GetNumberOfIntermediateBlocks(); counter++) {
		/* Completion ratio */	
		fprintf(stderr, "%02d%%\n", (counter*100)/(3*encoder.GetNumberOfIntermediateBlocks()));

		/* Generate a checkBlock ID, using the previous one as seed */
		*blockID = encoder.GenerateBlockID((uint32_t) *blockID);

		/* Encode a checkBlock */
		encoder.Encode(checkBlock, *blockID);

		/* Save file */
		saveBlock(outputPrefix, *blockID, outputBuffer, outputSize);
	}
	return 0;
}

void encode(options_t options) {
	FILE *inputFid;
	uint8_t *inputBuffer;
	uint32_t inputBufferSize;

	/* Open input file and inspect its size */
	if(!(inputFid = fopen(options.inputFile, "r"))) {
		fprintf(stderr, "Unable to open file. \"%s\"\n", options.inputFile);
		exit(1);
	}

  fseek (inputFid , 0 , SEEK_END);
  inputBufferSize = (uint32_t ) ftell (inputFid);
  rewind (inputFid);

	if(verbose)
		fprintf(stderr, "Encoding file size: %d bytes\n", inputBufferSize);

	/* Load file into cache */
	if((inputBuffer = (uint8_t *)malloc(sizeof(uint8_t)*inputBufferSize)) == NULL) {
		fprintf(stderr, "Error allocating memory: %d bytes\n", (uint32_t) inputBufferSize);
		exit(1);
	}
	
	if(verbose)
		fprintf(stderr, "Caching input file...");

	if(fread(inputBuffer, sizeof(uint8_t), inputBufferSize, inputFid) != inputBufferSize) {
		fprintf(stderr, "Error reading file\n");
		exit(1);
	}
	fclose(inputFid);

	if(verbose)
		fprintf(stderr, "done.\n");

	/* Create encoder Instance */
	OcdEncoder encoder(inputBuffer, inputBufferSize, options.blockSize, Q_CONSTANT, EPSILON_CONSTANT);

	if(verbose)
		fprintf(stderr, "Starting file encoding.\n");

	/* Save/Create file header, which stores the source data size */
	saveHeader(options.outputFile, inputBufferSize);

	encodeData(encoder, options.outputFile);

	if(verbose)
		fprintf(stderr, "Finished file encoding.\n");

	free(inputBuffer);
}

int decodeData(OcdDecoder &decoder, uint8_t *outputBuffer, uint32_t outputSize, char *inputPrefix) {
	DIR *dir;
	struct dirent *ent;
	uint8_t *checkBlock;
	blockID_t blockID;
	bool finishedDecoding;
	uint32_t checkBlockCounter;

	checkBlock = (uint8_t *) malloc(sizeof(uint8_t)*DEFAULT_BLOCK_SIZE);

	/* Load a file at a time */
	if((dir = opendir (".")) == NULL) {
	  fprintf(stderr, "Cannot open directory! Big-crunch is coming!\n");
	  return 1;
	}

	/* Decodes block after block, until the file is fully decoded */
	finishedDecoding = false;
	checkBlockCounter = 0;
	while (((ent = readdir (dir)) != NULL) && (finishedDecoding == false)) {
		if((strncmp(ent->d_name, inputPrefix, strlen(inputPrefix)) == 0) &&
		   (strcasestr(ent->d_name, ".blk") != NULL) &&
		   (strcasestr(ent->d_name, "header") == NULL)) {

			/* ent->d_name contains the name of a valid checkBlock */
			/* Load checkBlock and its ID */
			loadBlock(ent->d_name, &blockID, checkBlock, DEFAULT_BLOCK_SIZE);
			checkBlockCounter++;

			/* Send the block for decoding */
			decoder.Decode(checkBlock, blockID);

			fprintf(stderr, "Remaining blocks %d\n", decoder.GetNumberOfRemainingBlocks());
			fflush(stdout);

			if(decoder.GetNumberOfRemainingBlocks() == 0) {
				decoder.FinishDecoding(outputBuffer);
				finishedDecoding = true;
			}
		}
	}
	closedir (dir);

	decoder.FinishDecoding(outputBuffer);
	if(verbose)
		fprintf(stderr, "Fully decoded %d source blocks (intermediate: %d) file using %d\n", decoder.GetNumberOfSourceBlocks(),
																   decoder.GetNumberOfIntermediateBlocks(),
																   checkBlockCounter);

	return 0;
}

void decode(options_t options) {
	uint32_t outputFileSize;
	uint8_t *outputBuffer = NULL;
	FILE *outputFid;

	/* Read final file size from header */
	loadHeader(options.inputFile, &outputFileSize);

	if(verbose)
		fprintf(stderr, "Decoding file size: %d bytes\n", outputFileSize);

	/* Prepare buffer */
	if((outputBuffer = (uint8_t *) malloc(sizeof(uint8_t)*outputFileSize)) == NULL) {
		fprintf(stderr, "Memory error\n");
		return;
	}

	/* Create decoder Instance */
	OcdDecoder decoder(outputFileSize, options.blockSize, Q_CONSTANT, EPSILON_CONSTANT);

	if(verbose)
		fprintf(stderr, "Starting file decoding.\n");

	decodeData(decoder, outputBuffer, outputFileSize, options.inputFile);

	if(verbose)
		fprintf(stderr, "Finished file decoding.\n");

	if(verbose)
		fprintf(stderr, "Writing data to file...");

	if(!(outputFid = fopen(options.outputFile, "w"))) {
		fprintf(stderr, "Unable to open file. \"%s\"\n", options.outputFile);
		exit(1);
	}
	
	if(fwrite(outputBuffer, sizeof(uint8_t), outputFileSize, outputFid) != outputFileSize) {
		fprintf(stderr, "Error writing file!\n");
		exit(1);
	}

	if(verbose)
		fprintf(stderr, "done.\n");

	fclose(outputFid);
	free(outputBuffer);
}

int main(int argc, char *argv[]) {
  /* Needed to measure CPU time */
  clock_t tic, tac, start;
  double cpuTimeUsed;

  /* Needed to measure memory usage */
  struct mallinfo mem;

	options_t options;

  start = clock();

  if(getOptions(argc, argv, &options)) {
		displayUsage();
		exit(1);
	}

	verbose = options.verbose;

	if(!options.encode && !options.decode)
		fprintf(stderr, "Nothing to do.\n");

	if(options.encode)
		encode(options);

	if(options.decode)
		decode(options);

  return 0;
}

