/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Evan Caplinger, 
* Student IDs:: 924990024, 
* GitHub-Name:: RookAteMySSD
* Group-Name:: Team #1 Victory Royal
* Project:: Basic File System
*
* File:: fsInit.c
*
* Description:: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"
#include "fsFreeSpace.h"
#include "fsVCB.h"


int initFileSystem ( uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */

	// get a pointer to the global vcb
	vcb* ourVCB = _getGlobalVCB();

	// read VCB into VCB struct via a buffer
	char* vcbBuf = malloc(blockSize);
	if (vcbBuf == NULL) {
		perror("malloc");
		return -1;
	}

	uint32_t got = LBAread(vcbBuf, 1, 0);
	if(got == 1)
	{
		memcpy(ourVCB, vcbBuf, sizeof(vcb));
		free(vcbBuf);
		vcbBuf = NULL;

		if(ourVCB->signature == FS_VCB_MAGIC && ourVCB->blockSize == blockSize)
		{
			if(loadFAT(ourVCB) != 0)
			{	
				printf("loadFAT failed!\n");
				return -1;
			}
			return 0; // mounted existing FS
		}
	}
	else
	{
		free(vcbBuf);
		vcbBuf = NULL;
	}
	
	if(initVCB(ourVCB, numberOfBlocks, blockSize) != 0)
	{
		printf("initVCB failed\n");
		return -1;
	}
	return 0;
	}
	
	// int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	// {
	// printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	// /* TODO: Add any code you need to initialize your file system. */

	// // get a pointer to the global vcb
	// vcb* ourVCB = _getGlobalVCB();

	// // read VCB into VCB struct via a buffer
	// char* vcbBuf = malloc(blockSize);
	// if (vcbBuf == NULL) {
	// 	perror("malloc");
	// 	return -1;
	// }

	// // copy data from buffer into vcb and free allocated buffer space
	// memcpy(ourVCB, vcbBuf, sizeof(vcb));
	// free(vcbBuf);
	// vcbBuf = NULL;

	// // check signature, initialize if incorrect
	// if (ourVCB->signature != FS_VCB_MAGIC) {
	// 	int r = initVCB(ourVCB, numberOfBlocks, blockSize);
	// 	if (r != 0) {
	// 		printf("could not init vcb\n");
	// 		return r;
	// 	}
	// }

	// return 0;
	// }
	
void exitFileSystem ()
	{
		closePartitionSystem();
		printf ("System exiting\n");
	}
