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

	// create buffer to read VCB into
	char* vcbBuf = malloc(blockSize);
	if (vcbBuf == NULL) {
		perror("malloc");
		return -1;
	}

	// read VCB into buffer
	uint32_t got = LBAread(vcbBuf, 1, 0);
	if(got == 1)
	{
		// copy vcb from read buffer into vcb object
		memcpy(ourVCB, vcbBuf, sizeof(vcb));
		free(vcbBuf);
		vcbBuf = NULL;

		// check if we need to format volume; if not, load and exit
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
	else // failed to read VCB; abort
	{
		free(vcbBuf);
		vcbBuf = NULL;
		return -1;
	}
	
	// if we have reached this point, we need to format the volume
	if(initVCB(ourVCB, numberOfBlocks, blockSize) != 0)
	{
		printf("initVCB failed\n");
		return -1;
	}
	return 0;
	}
	
void exitFileSystem ()
	{
		printf ("System exiting\n");
	}
