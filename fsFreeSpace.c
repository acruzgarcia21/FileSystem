/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Alejandro Cruz-Garcia, Ronin Lombardino, Evan Caplinger, Alex Tamayo
* Student IDs:: 923799497, 924363164, 924990024, 921199718
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
#include <stdlib.h>
#include <stdio.h>

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
        perror("malloc");
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
    uint64_t result = LBAwrite(fat, fatNumBlocks, pVCB->fatStart);
    if (result != fatNumBlocks) {
        return -1;
    }

    return 0;
}

void uninitFAT() {
    free(fat);
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
    if (result != pVCB->fatNumBlocks) {
        return -1;
    }

    //store the VCB pointer globally
    global_pVCB = pVCB;

    return 0;
}

uint32_t allocateBlocks(uint32_t numBlocks) {
    // ensure that fat is initialized (mounted)
    if (global_pVCB == NULL || fat == NULL) {
        printf("File system not properly initialized.\n");
        return -1;
    }

    uint32_t currentBlock = global_pVCB->firstFreeBlock;
    uint32_t nextBlock;
    
    //iterate through the FAT form the head
    for (int i = 0; i < numBlocks; i++) {
        //if the current block is our EOF sentinel, return EOF sentinel
        if (currentBlock == FAT_EOF) {
            printf("Error while traversing freespace list.\n");
            return FAT_EOF;
        }

        nextBlock = fat[currentBlock];
        // if we are at EOF, then set EOF
        if (i == numBlocks - 1) {
            fat[currentBlock] = FAT_EOF;
        }
        currentBlock = nextBlock;
    }

    //store the start block as an index
    uint32_t startBlock = global_pVCB->firstFreeBlock;
    //set the index of the currentBlock's next block to the FreeBlock start
    global_pVCB->firstFreeBlock = currentBlock;

    // write FAT to disk
    uint64_t result = LBAwrite(fat, global_pVCB->fatNumBlocks, global_pVCB->fatStart);
    if (result != global_pVCB->fatNumBlocks) {
        printf("Could not write FAT.\n");
        return -1;
    }

    // write VCB to disk
    result = LBAwrite(global_pVCB, 1, 0);
    if (result != 1) {
        return -1;
    }

    //return the start block index
    return startBlock;
}

int freeBlocks(uint32_t startBlock) {
    // ensure that fat is initialized (mounted)
    if (global_pVCB == NULL || fat == NULL) {
        return -1;
    }

    if (startBlock == 0 || startBlock == FAT_EOF || startBlock == FAT_RESERVED)
    {
        return -1;
    }

    uint32_t numBlocks = global_pVCB->numBlocks;
    // find the end of the file we just freed
    uint32_t fileEnd = startBlock;
    uint32_t prev    = startBlock;
    uint32_t steps = 0;

    // Walk the file's FAT chain until we hit EOF
    while (1)
    {
        uint32_t next = fat[fileEnd];

        if(next == FAT_EOF)
        {
            // fileEnd is the last block in this file chain
            prev = fileEnd;
            break;
        }

        if (next == FAT_RESERVED)
        {
            // Corrupt chain 
            return -1;
        }

        fileEnd = next;

        // Saftey: avoid infinite loops due to corruption
        if(steps++ > numBlocks)
        {
            // Something went wrong
            printf("(freeBlocks): aborting, loop exceeded numBlocks\n");
            return -1;
        }
    }

    // Link this freed chain in front of the current free list
    uint32_t oldFreeHead = global_pVCB->firstFreeBlock;

    // Tail of the freed chain now points at old head
    fat[prev] = oldFreeHead;

    // New head is the start of the freed chain
    global_pVCB->firstFreeBlock = startBlock;

    // If there used to be no free list, fix up lastFreeBlock
    if (oldFreeHead == FAT_EOF || oldFreeHead == FAT_RESERVED || oldFreeHead == 0)
    {
        uint32_t tail = startBlock;
        steps = 0;
        while (fat[tail] != FAT_EOF && steps++ < numBlocks)
        {
            tail = fat[tail];
        }
        global_pVCB->lastFreeBlock = tail;
    }

    // write FAT to disk
    uint64_t result = LBAwrite(fat, global_pVCB->fatNumBlocks, global_pVCB->fatStart);
    if (result != global_pVCB->fatNumBlocks) {
        return -1;
    }

    // write VCB to disk
    result = LBAwrite(global_pVCB, 1, 0);
    if (result != 1) {
        return -1;
    }

    return 0;
}

int resizeBlocks(uint32_t startBlock, uint32_t newSize) {
    // ensure that fat is initialized (mounted)
    if (global_pVCB == NULL || fat == NULL) {
        return -1;
    }

    // if the size is 0, call freeBlocks with startBlock and return
    if (newSize == 0) {
        return freeBlocks(startBlock);
    }

    uint32_t newSizeBlocks = (newSize + global_pVCB->blockSize - 1) / global_pVCB->blockSize;

    uint32_t currentBlock = startBlock;
    //iterate through the FAT with the start block until we hit EOF or size
    for (int i = 0; i < newSizeBlocks; i++) {
        //if we hit EOF, break
        if (fat[currentBlock] == FAT_EOF) {
            //if we hit EOF first, call allocateBlocks with the diffrence and append it to the end
            fat[currentBlock] = allocateBlocks(newSizeBlocks - i);
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

uint32_t getNextBlock(uint32_t startBlock) {
    return fat[startBlock];
}
