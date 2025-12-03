/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Alejandro Cruz-Garcia, Ronin Lombardino, Evan Caplinger, Alex Tamayo
* Student IDs:: 923799497, 924363164, 924990024, 921199718
* GitHub-Name:: RookAteMySSD
* Group-Name:: Team #1 Victory Royal
* Project:: Basic File System
*
* File:: fsVCB.c
*
* Description:: Definition and functions for Volume Control
*               Block (VCB).
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "fsVCB.h"
#include "fsFreeSpace.h"
#include "fsDirectory.h"

vcb globalVCB;

vcb* getGlobalVCB() {
    return &globalVCB;
}

int initVCB(vcb* pVCB, uint64_t numberOfBlocks, uint64_t blockSize) {
    // create signature
    pVCB->signature = FS_VCB_MAGIC;

    // initialize values given to us
    pVCB->numBlocks = numberOfBlocks;
    pVCB->blockSize = blockSize;

    // initialize freespace system
    int r = initFAT(pVCB);
    if (r != 0) {
        printf("could not init fat\n");
        return r;
    }

    // initialize root directory entry
    DE* rootEntry = createDir(FS_ROOT_COUNT, NULL, pVCB->blockSize);
    if (rootEntry == NULL) {
        printf("could not init root\n");
        return -1;
    }
    pVCB->rootStart = rootEntry->location;
    pVCB->rootSize = rootEntry->size;
    
    // free allocated memory for the root directory
    free(rootEntry);

    // write vcb to disk at block 0
    r = LBAwrite(pVCB, 1, 0);
    if (r != 1) {
        printf("could not write vcb\n");
        return -1;
    }

    return 0;
}