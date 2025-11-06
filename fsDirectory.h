/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Alejandro Cruz-Garcia, Ronin Lombardino
* Student IDs:: 923799497, 924363164
* GitHub-Name:: RookAteMySSD
* Group-Name:: Team #1 Victory Royal
* Project:: Basic File System
*
* File:: fsDirectory.h
*
* Description:: Directory struct and function declarations
*
**************************************************************/

#ifndef FS_DIRECTORY_H
#define FS_DIRECTORY_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include "fsLow.h"
#include "fsFreeSpace.h"

// size of name field in struct; value chosen to eliminate internal padding
#define DE_NAME_MAX 47

// Flag bit masks
#define DE_IS_DIR 0x01  // 0000 0001 - directory
#define DE_IS_USED 0x80 // 1000 0000 - used entry

typedef struct 
{
    char name[DE_NAME_MAX];
    uint8_t flags;      // bitmap for DE properties:
                        // DE_IS_USED (bit 0), DE_IS_DIR (bit 7)
    uint64_t created;   // created timestamp
    uint64_t accessed;  // accessed timestamp
    uint64_t modified;  // modified timestamp
    uint32_t size;      // size in bytes
    uint32_t location;  // start block
} DE;

// Initializer for root directory/any directory
DE* createDir(int count, const DE* parent, int blockSize);
// write a file to disk
int writeFileToDisk(char* data, DE* entry);
// write raw blocks to disk in allocated blocks; return number of blocks
// that were successfully written
int writeBlocksToDisk(char* data, uint32_t startBlock, uint32_t numBlocks);
// get current time as a uint32_t
uint64_t getCurrentTime();
// Find an entry name withing a loaded directory
DE* findEntryInDirectory(DE* dir, int entryCount, const char* name);
// Load a directory table to disk (reads DE array from given location)
DE* loadDirectory(uint32_t startBlock, uint32_t size, uint32_t blockSize);

#endif