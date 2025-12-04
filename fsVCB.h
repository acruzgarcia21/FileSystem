/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Alejandro Cruz-Garcia, Ronin Lombardino, Evan Caplinger, Alex Tamayo
* Student IDs:: 923799497, 924363164, 924990024, 921199718
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

#define FS_VCB_MAGIC 0x8BADBEEF
#define FS_ROOT_COUNT 51

typedef struct {
    // signature (should be magic word)
    uint32_t signature;

    // volume characteristics
    uint32_t blockSize;
    uint32_t numBlocks;

    // free space variables
    uint32_t fatStart;
    uint32_t fatNumBlocks;
    uint32_t firstFreeBlock;
    uint32_t lastFreeBlock;

    // root dir info
    uint32_t rootStart;
    uint32_t rootSize;
} vcb;

// returns a pointer to a global instance of the VCB
vcb* getGlobalVCB();

// initialize the VCB
int initVCB(vcb* pVCB, uint64_t numberOfBlocks, uint64_t blockSize);

#endif
