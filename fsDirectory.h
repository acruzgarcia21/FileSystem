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

#ifndef DS_DIRECTORY_H
#define DS_DIRECTORY_H

#include <stdint.h>
#include "fsFreeSpace.h"

#define DE_NAME_MAX 48

typedef struct 
{
    char name[DE_NAME_MAX];

    int isUsed; // 0 is not used, 1 is used
    int isDir;  // 0 is not a directory, 1 is a directory

    uint32_t created;
    uint32_t accessed;
    uint32_t modified;
    uint32_t size;
    uint32_t location; // Starting LBA
} DE;

// Initializer for root directory/any directory
DE* createDir(int requestedEntries, const DE* parent, int blockSize);

int writeToDisk(const DE* dir, int blockSize);

#endif