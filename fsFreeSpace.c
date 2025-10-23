/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Evan Caplinger, 
* Student IDs:: 924990024, 
* GitHub-Name:: RookAteMySSD
* Group-Name:: Team #1 Victory Royal
* Project:: Basic File System
*
* File:: fsFreeSpace.c
*
* Description:: Free space management functions.
*
**************************************************************/

#include "fsFreeSpace.h"
#include "fsLow.h"
#include <stddef.h>

uint32_t* fat = NULL;

int initFAT(vcb* pVCB) {
    // copy over values from VCB
    uint32_t numBlocks = pVCB->numBlocks;
    uint32_t blockSize = pVCB->blockSize;

    // allocate space for FAT
    uint32_t fatNumBlocks = (numBlocks * sizeof(uint32_t) + blockSize - 1) / blockSize;
    fat = malloc(fatNumBlocks * blockSize);
    if (fat == NULL) {
        return -1;
    }

    // reserve VCB and FAT blocks
    fat[0] = FAT_RESERVED;
    for (int i = 1; i <= fatNumBlocks; i++) {
        fat[i] = FAT_RESERVED;
    }

    // set VCB values
    pVCB->fatStart = 1;
    pVCB->fatNumBlocks = fatNumBlocks;
    pVCB->firstFreeBlock = fatNumBlocks + 1;
    pVCB->lastFreeBlock = numBlocks - 1;

    // set all other blocks to point to the next block
    for (int i = fatNumBlocks + 1; i < numBlocks - 1; i++) {
        fat[i] = i + 1;
    }

    // set the final block to EOF
    fat[numBlocks - 1] = FAT_EOF;

    // reserve the rest of the blocks in the FAT
    for (int i = numBlocks; i < fatNumBlocks * blockSize; i++) {
        fat[i] = FAT_RESERVED;
    }

    // write FAT to disk
    uint64_t result = LBAwrite((void *)fat, fatNumBlocks, pVCB->fatStart);;
    if (result != 0) {
        return -1;
    }

    return 0;
}

int loadFAT(vcb* pVCB) {
    // check if FAT is somehow already allocated
    if (fat != NULL) {
        return -1;
    }

    // allocate space for FAT
    fat = malloc(pVCB->fatNumBlocks * pVCB->blockSize);
    if (fat == NULL) {
        return -1;
    }

    // load FAT into memory
    uint64_t result = LBAread((void *)fat, pVCB->fatNumBlocks, pVCB->fatStart);
    if (result != 0) {
        return -1;
    }

    return 0;
}

int freeBlocks(uint32_t startBlock, vcb* pVCB) {
    return 0;
}
