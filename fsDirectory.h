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

#define DE_NAME_MAX 47

// Flag bit masks
#define DE_IS_DIR 0x01  // 0000 0001 - directory
#define DE_IS_USED 0x80 // 1000 0000 - used entry

typedef struct 
{
    char name[DE_NAME_MAX];
    uint8_t flags;
    uint64_t created;
    uint64_t accessed;
    uint64_t modified;
    uint32_t size;
    uint32_t location; // Starting LBA
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

#endif