/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Evan Caplinger, 
* Student IDs:: 924990024, 
* GitHub-Name:: RookAteMySSD
* Group-Name:: Team #1 Victory Royal
* Project:: Basic File System
*
* File:: fsVCB.h
*
* Description:: Definition and functions for Volume Control
*               Block (VCB).
*
**************************************************************/

#ifndef __FSVCB_H__
#define __FSVCB_H__

#include <stdint.h>

/* a point of concern: we have been using uint32_t (or unsigned int) for block
addressing and block size, but in initFileSystem these are uint64_t. worth asking
bierman? -erc */
typedef struct {
    // volume characteristics
    uint32_t blockSize;
    uint32_t numBlocks;

    // free space variables
    uint32_t fatStart;
    uint32_t fatNumBlocks;
    uint32_t firstFreeBlock;
    uint32_t lastFreeBlock;

} vcb;

#endif
