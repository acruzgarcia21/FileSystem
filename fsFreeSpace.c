/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Evan Caplinger, Ronin Lombardino 
* Student IDs:: 924990024, 924363164
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
vcb* global_pVCB = NULL;

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

    //store the VCB pointer globally
    global_pVCB = pVCB;

    // set all other blocks to point to the next block
    for (int i = fatNumBlocks + 1; i < numBlocks - 1; i++) {
        fat[i] = i + 1;
    }

    // set the final block to EOF
    fat[numBlocks - 1] = FAT_EOF;

    // reserve the rest of the blocks in the FAT
    for (int i = numBlocks; i < fatNumBlocks * blockSize / sizeof(uint32_t); i++) {
        fat[i] = FAT_RESERVED;
    }

    // write FAT to disk
    uint64_t result = LBAwrite((void *)fat, fatNumBlocks, pVCB->fatStart);
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

    //store the VCB pointer globally
    global_pVCB = pVCB;

    return 0;
}

uint32_t allocateBlocks(uint32_t numBlocks) {
    uint32_t currentBlock = global_pVCB->firstFreeBlock;
    
    //iterate through the FAT form the head
    for (int i = 0; i < numBlocks; i++) {
        //if the current block is our EOF sentinel, return EOF sentinel
        if (currentBlock == FAT_EOF) {
            return FAT_EOF;
        }
        currentBlock = fat[currentBlock];
    }

    //store the start block as an index
    uint32_t startBlock = global_pVCB->firstFreeBlock;
    //set the index of the currentBlock's next block to the FreeBlock start
    global_pVCB->firstFreeBlock = fat[currentBlock];
    //set the currentBlock to EOF
    fat[currentBlock] = FAT_EOF;
    //return the start block index
    return startBlock;
    
    return 0;
}

int freeBlocks(uint32_t startBlock) {
    // ensure that fat is initialized (mounted)
    if (global_pVCB == NULL || fat == NULL) {
        return -1;
    }

    // add blocks from file to free block list
    fat[global_pVCB->lastFreeBlock] = startBlock;

    // find the end of the file we just freed
    uint32_t fileEnd;
    do {
        fileEnd = fat[fileEnd];

        // handle case where we run into reserved blocks
        if (fat[fileEnd] == FAT_RESERVED) {
            return -1;
        }
    } while (fat[fileEnd != FAT_EOF]);

    // set new last free block in VCB
    global_pVCB->lastFreeBlock = fileEnd;

    return 0;
}

int resizeBlocks(uint32_t startBlock, int newSize) {

    // if the size is 0, call freeBlocks with startBlock and return
    if (newSize == 0) {
        return freeBlocks(startBlock);
    }

    uint32_t currentBlock = startBlock;
    //iterate through the FAT with the start block until we hit EOF or size
    for (int i = 0; i < newSize; i++) {
        //if we hit EOF, break
        if (fat[currentBlock] == FAT_EOF) {
            //if we hit EOF first, call allocateBlocks with the diffrence and append it to the end
            fat[currentBlock] = allocateBlocks(newSize - i);
            if (fat[currentBlock] == FAT_EOF) {
                return FAT_EOF;
            }
            return 0;
        }
        currentBlock = fat[currentBlock];
    }

    //if we hit size first, call freeBlocks on the next block after size and update the EOF sentinel
    freeBlocks(fat[currentBlock]);
    fat[currentBlock] = FAT_EOF;
    return 0;
}

uint32_t getBlockOfFile(uint32_t startBlock, uint32_t offset) {
    //set currentBlock to startBlock
    uint32_t currentBlock = startBlock;

    //iterate through the FAT offset times
    for (uint32_t i = 0; i < offset; i++) {
        //if we hit EOF, return EOF
        if (fat[currentBlock] == FAT_EOF) {
            return FAT_EOF;
        }
        currentBlock = fat[currentBlock];
    }
    //return the currentBlock
    return currentBlock;
}