/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Evan Caplinger, Ronin Lombardino
* Student IDs:: 924990024, 924363164
* GitHub-Name:: RookAteMySSD
* Group-Name:: Team #1 Victory Royal
* Project:: Basic File System
*
* File:: fsFreeSpace.h
*
* Description:: Free space management functions.
*
**************************************************************/

#ifndef __FSFREESPACE_H__
#define __FSFREESPACE_H__

#define FAT_RESERVED 0xffffffff
#define FAT_EOF 0xfffffffe

#include <stdint.h>
#include <fsDirectory.h>

#include "fsVCB.h"

// initialize FAT, write to disk, update vcb
int initFAT(vcb* pVCB);

// load FAT into memory
int loadFAT(vcb* pVCB);

// allocate blocks of given size
uint32_t allocateBlocks(uint32_t numBlocks);

// free blocks from startBlock to EOF
int freeBlocks(uint32_t startBlock);

// resize allocated blocks
int resizeBlocks(uint32_t startBlock, uint32_t newSize);

// get blocknum of certain offset
uint32_t getBlockOfFile(uint32_t startBlock, uint32_t offset);

// get the next block in FAT (or EOF if startBlock is at EOF)
uint32_t getNextBlock(uint32_t startBlock);

#endif
