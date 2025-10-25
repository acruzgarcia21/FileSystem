/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Alejandro Cruz-Garcia
* Student IDs:: 923799497
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

#define DE_NAME_MAX 48

// Flag bit masks
#define DE_IS_DIR 0x01  // 0000 0001 - directory
#define DE_IS_USED 0x80 // 1000 0000 - used entry

typedef struct 
{
    char name[DE_NAME_MAX];    
    uint32_t size;
    uint32_t location; // Starting LBA
    uint32_t created;
    uint32_t accessed;
    uint32_t modified;
    uint8_t flags;
} DE;

// Initializer for root directory/any directory
DE* createDir(int count, const DE* parent, int blockSize);

int writeDirToDisk(const DE* dir, int blockSize);

#endif