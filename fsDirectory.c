/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Alejandro Cruz-Garcia
* Student IDs:: 923799497
* GitHub-Name:: RookAteMySSD
* Group-Name:: Team #1 Victory Royal
* Project:: Basic File System
*
* File:: fsDirectory.c
*
* Description:: Directory struct with a initializer for any directory/
* root directory as well as a write to disk
*
**************************************************************/

#include "fsDirectory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int writeDirToDisk(const DE* dir, int blockSize)
{
    if(!dir || dir->location == 0) return -1;
    uint32_t blocks = (dir->size + blockSize - 1) / blockSize; 
    uint32_t wrote = LBAwrite(dir, blocks, dir->location);
    return (wrote == blocks) ? 0 : -1;
}

int writeFileToDisk(char* data, DE* entry) {
    // get global VCB
    vcb* globalVCB = _getGlobalVCB();
    if (globalVCB == NULL) {
        return -1;
    }

    // set up variables for writing
    uint32_t bytesToWrite = entry->size;
    uint32_t blocksToWrite = (bytesToWrite + globalVCB->blockSize - 1) / globalVCB->blockSize;

    // write all blocks of file to disk
    int r;  // contains result from LBAwrite
    for (int i = 0; i < blocksToWrite; i++) {
        r = LBAwrite(&data[i * globalVCB->blockSize],
                     1,
                     getBlockOfFile(entry->location, i));
        if (r != 0) {   // abort on error
            return -1;
        }
    }

    return 0;
}

DE* createDir(int count, const DE* parent, int blockSize)
{
    if(count < 2) count = 2; // need to have . and ..

    // Compute how many bytes and blocks the new directory will occupy
    uint32_t bytesNeeded = (uint32_t)count * (uint32_t)sizeof(DE);
    int blocksNeeded = (bytesNeeded + blockSize - 1) / blockSize; 
    int bytesToMalloc = blocksNeeded * blockSize;
    int actualEntries = bytesToMalloc / (int)sizeof(DE);

    DE* dir = malloc(bytesToMalloc);
    if(!dir) return NULL;

    // Initialize unused slots
    for(int i = 2; i < actualEntries; i++)
    {
        dir[i].flags = 0;
        dir[i].name[0] = '\0';
    }

    uint32_t location = allocateBlocks(blocksNeeded);
    if(location == 0)
    {
        free(dir);
        return NULL;
    }

    uint32_t currentDT = getCurrentTime();
    strcpy(dir[0].name, ".");
    dir[0].flags = (DE_IS_USED | DE_IS_DIR);
    dir[0].size = (uint32_t)actualEntries * (uint32_t)sizeof(DE);
    dir[0].location = location;
    dir[0].created = currentDT;
    dir[0].accessed = currentDT;
    dir[0].modified = currentDT;

    if(parent == NULL) {parent = &dir[0];}
    strcpy(dir[1].name, "..");
    dir[1].flags = (DE_IS_USED | DE_IS_DIR);
    dir[1].size = (uint32_t)actualEntries * (uint32_t)sizeof(DE);
    dir[1].location = location;
    dir[1].created = currentDT;
    dir[1].accessed = currentDT;
    dir[1].modified = currentDT;

    if(writeDirToDisk(dir, blockSize) != 0)
    {
        free(dir);
        return NULL;
    }
    return dir;
}