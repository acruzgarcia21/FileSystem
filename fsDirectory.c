/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Alejandro Cruz-Garcia, Ronin Lombardino
* Student IDs:: 923799497, 924363164
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
#include <time.h>

int writeFileToDisk(char* data, DE* entry) {
    // get global VCB
    vcb* globalVCB = _getGlobalVCB();
    if (globalVCB == NULL || entry == NULL) return -1;

    // We'll use the block size defined by the file system
    uint32_t blockSize = globalVCB->blockSize;
    // Total bytes to write is based on the files actual size
    uint32_t bytesToWrite = entry->size;
    // Figure out how many blocks the file spans
    uint32_t blocksToWrite = (bytesToWrite + blockSize - 1) / blockSize;

    // Temp buffer for final partial block
    uint8_t* temp = (uint8_t*)malloc(blockSize);
    if(!temp) return -1;

    // Loop through each logical block of the file
    for(uint32_t i = 0; i < blocksToWrite; i++)
    {
        // Get the disk block number from the FAT
        // FAT maps the file's logical blocks to scattered physical blocks
        uint32_t blockNum = getBlockOfFile(entry->location, i);
        if(blockNum == FAT_EOF || blockNum == FAT_RESERVED)
        {
            free(temp);
            return -1;
        }

        // Figure out where in memory this chunk of data starts
        uint32_t offset = i * blockSize;
        // Figure out how many bytes there are left to write
        uint32_t chunk = (offset + blockSize <= blocksToWrite) 
                         ? blockSize 
                         : (blocksToWrite - offset);
        
        // Store the result from LBAwrite
        int result;
        // If we have a full block of data left, write it directly
        if(chunk == blockSize)
        {
            result = (int)LBAwrite(data + offset, 1, blockNum);
        }
        else
        {
            // Otherwise, we're at the last partial block
            // Fill the temp buffer with zeros first to avoid garbage data and avoid overread
            // Copy only the valid portion of data into the temp buffer
            // Then write that padded block to the disk
            memset(temp, 0, blockSize);
            memcpy(temp, data + offset, chunk);
            result = (int)LBAwrite(temp, 1, blockNum);
        }
        if(result != 1)
        {
            free(temp);
            return -1;
        }
    }
    free(temp);
    return 0;
}
/* 
*  Builds a new directory table in RAM, reserves enough disk blocks via the FAT
*  allocator, fills "." and "..", marks all remaining entries as unused (reserved capacity).
*  and then writes the directory table to disk using the fat mapping
*/
DE* createDir(int count, const DE* parent, int blockSize)
{
    // Ensure we have at least 2 entries for "." and "..""
    if(count < 2) count = 2; 

    // Compute how many bytes and blocks the new directory will occupy
    // We round up to a whole number of blocks so that we have reserved capacity
    // for future entries and not garbage data
    uint32_t bytesNeeded = (uint32_t)count * (uint32_t)sizeof(DE);
    int blocksNeeded = (bytesNeeded + blockSize - 1) / blockSize; 
    int bytesToMalloc = blocksNeeded * blockSize;
    int actualEntries = bytesToMalloc / (int)sizeof(DE);

    DE* dir = malloc(bytesToMalloc);
    if(!dir) return NULL;

    // Initialize reserved entries as unused
    // This keeps capacity pre-allocated so the directory 
    // can add files later without growing immediately
    for(int i = 2; i < actualEntries; i++)
    {
        dir[i].flags    = 0;
        dir[i].name[0]  = '\0';
        dir[i].size     = 0;
        dir[i].location = 0;
        dir[i].created  = 0;
        dir[i].accessed = 0;
        dir[i].modified = 0;
    }

    uint32_t location = allocateBlocks(blocksNeeded);
    if(location == 0)
    {
        free(dir);
        return NULL;
    }

    uint32_t now = getCurrentTime();
    // "." points to this directory's region
    strcpy(dir[0].name, ".");
    dir[0].flags    = (DE_IS_USED | DE_IS_DIR);
    dir[0].size     = (uint32_t)actualEntries * (uint32_t)sizeof(DE);
    dir[0].location = location;
    dir[0].created  = now;
    dir[0].accessed = now;
    dir[0].modified = now;

    // Determine the parent entry (root uses itself for "..")
    const DE* par = parent ? parent : &dir[0];
    // ".." mirrors the parents directory
    strcpy(dir[1].name, "..");
    dir[1].flags    = (DE_IS_USED | DE_IS_DIR);
    dir[1].size     = par->size;
    dir[1].location = par->location;
    dir[1].created  = par->created;
    dir[1].accessed = par->accessed;
    dir[1].modified = par->modified;

    // Persist the directory table to disk using the FAT mapping.
    // We pass the table as raw bytes (char*) and the entry that
    // contains size/location (dir[0]) so the writer knows how much to write
    // and where the FAT chain begins
    if(writeFileToDisk((char*)dir, &dir[0]) != 0)
    {
        free(dir);
        return NULL;
    }
    return dir;
}

//Get the current time in secionds as a uint32_t
uint32_t getCurrentTime() {
    time_t now = time(NULL);
    return (uint32_t)now;
}