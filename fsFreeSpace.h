/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Evan Caplinger, 
* Student IDs:: 924990024, 
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

#include "fsVCB.h"

// initialize FAT, write to disk, update vcb
int initFAT(vcb* pVCB);

// load FAT into memory
int loadFAT(vcb* pVCB);

// free blocks from startBlock to EOF
int freeBlocks(uint32_t startBlock);

// get blocknum of certain offset
uint32_t getBlockOfFile(uint32_t startBlock, uint32_t offset);

#endif
