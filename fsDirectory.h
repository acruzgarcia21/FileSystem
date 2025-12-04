/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Alejandro Cruz-Garcia, Ronin Lombardino, Evan Caplinger, Alex Tamayo
* Student IDs:: 923799497, 924363164, 924990024, 921199718
* GitHub-Name:: RookAteMySSD
* Group-Name:: Team #1 Victory Royal
* Project:: Basic File System
*
* File:: fsDirectory.h
*
* Description:: Functions that manipulate directories, including
*               file creation, removal, cwd stuff, etc.
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

#define MAX_PATH_DEPTH 50

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

// added by Alex Tamayo 11/6/2025 @ 6pm
// path parsing info structure
typedef struct ppinfo {
    DE* parent;              // loaded parent directory
    char* lastElementName;   // name of the last element
    int index;               // index in parent (-1 if not found)
} ppinfo;

// Initializer for root directory/any directory
DE* createDir(int count, const DE* parent, int blockSize);
// create a file (and write DE to disk)
int createFile(const char* filename, DE* parent);
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

// Added by Alex Tamayo 11/6/2025 @ 6pm
// parse path and navigate to parent
int parsePath(const char* path, ppinfo* ppi);

// helper functions used by parsePath
int findInDirectory(DE* dir, int entryCount, const char* name);
int isDEaDir(DE* entry);

// cwd functions
// build the string that represents the CWD absolute path
char* cwdBuildAbsPath();

// return a directory entry for the CWD
DE* getcwdInternal();

// internal function used to set CWD; called by fs_setcwd
int setcwdInternal(const char* path);

// initialize the CWD system
int initCwdAtRoot();

// free all allocated memory for CWD
void freeCwdMemory();

// reload the CWD to get new changes - size, etc. (just change to parent and back)
int reloadCwd();

// add a new entry to a directory
int addEntryToDirectory(DE* parent, DE* newEntry);

// int removeEntryFromDirectory(DE* parent, const char* entryName);
int isDirectoryEmpty(DE* dir, uint32_t entryIdx);

#endif
