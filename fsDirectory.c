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
    if (!globalVCB || !entry) return -1;

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
        uint32_t chunk = (offset + blockSize <= bytesToWrite) 
                         ? blockSize 
                         : (bytesToWrite - offset);
        
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

int writeBlocksToDisk(char* data, uint32_t startBlock, uint32_t numBlocks) {
    // get global VCB
    vcb* globalVCB = _getGlobalVCB();
    if (globalVCB == NULL) {
        return -1;
    }

    // write all blocks to disk
    uint32_t writeBlock = startBlock;
    int i;
    for (i = 0; i < numBlocks; i++) {
        // write single block to disk
        int r = LBAwrite(&data[i * globalVCB->blockSize],
                         1,
                         writeBlock);
        if (r != 1) {
            return -1;
        }

        writeBlock = getNextBlock(writeBlock);
        if (writeBlock == FAT_RESERVED) {
            return -1;
        } else if (writeBlock == FAT_EOF) {
            return i + 1;
        }
    }

    return i;
}

int readBlocksFromDisk(char* buffer, uint32_t startBlock, uint32_t numBlocks) {
    // get global VCB
    vcb* pVCB = _getGlobalVCB();
    if (pVCB == NULL) {
        return -1;
    }

    // allocate buffer
    buffer = malloc(numBlocks * pVCB->blockSize);
    if (buffer == NULL) {
        return -1;
    }

    // loop over blocks and read into buffer
    uint32_t readBlock = startBlock;
    int i;
    for (i = 0; i < numBlocks; i++) {
        int r = LBAread(&buffer[i * pVCB->blockSize],
                        1,
                        readBlock);
        if (r != 1) {
            free(buffer);
            return -1;
        }

        readBlock = getNextBlock(readBlock);
        if (readBlock == FAT_RESERVED) {
            free(buffer);
            return -1;
        } else if (readBlock == FAT_EOF) {
            return i + 1;
        }
    }

    return i + 1;
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

    DE* dir = calloc(1, bytesToMalloc);
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
    printf("writing to disk, location = %ld, blocksNeeded = %d\n", dir[0].location, blocksNeeded);
    int r = writeBlocksToDisk((char*)dir, dir[0].location, blocksNeeded);
    if(r != blocksNeeded)
    {
        printf("wanted to write %d blocks to disk, but wrote %d\n", blocksNeeded, r);
        free(dir);
        return NULL;
    }
    return dir;
}

//Get the current time in secionds as a uint32_t
uint64_t getCurrentTime() {
    time_t now = time(NULL);
    return (uint64_t)now;
}

DE* findEntryInDirectory(DE* dir, int entryCount, const char* name)
{
    for(int i = 0; i < entryCount; i++)
    {
        if((dir[i].flags & DE_IS_USED) && strcmp(dir[i].name, name) == 0)
        {
            return &dir[i];
        }
    }
    return NULL;
}

DE* loadDirectory(uint32_t startBlock, uint32_t size, uint32_t blockSize)
{
    if(startBlock == 0 || blockSize == 0) return NULL;

    // How many blocks we need to read to cover 'size' bytes
    uint32_t numBlocks = (size + blockSize - 1) / blockSize;
    if(numBlocks == 0) numBlocks = 1; // Ensure dir is 1 block

    // Allocate a whole number of blocks 
    uint32_t totalBytes = numBlocks * blockSize;
    DE* dir = (DE*)calloc(1, totalBytes);
    if(!dir) return NULL;

    // Walk the FAT chain, reading one block at a time into the buffer
    uint32_t block = startBlock;
    uint8_t* writeptr = (uint8_t*)dir;

    for(uint32_t i = 0; i < numBlocks; i++)
    {
        if(block == FAT_RESERVED || block == 0 || block == FAT_EOF)
        {
            free(dir);
            return NULL;
        }
        
        if(LBAread(writeptr, 1, block) != 1)
        {
            free(dir);
            return NULL;
        }

        writeptr += blockSize;

        // On the last iteration we don't need the next block
        if(i + 1 == numBlocks) break;
        // Follow the fat chain
        block = getNextBlock(block);
    }

    return dir;
}

// ***********************************************
// added by Alex Tamayo 11/6/2025 @ 6pm
// ***********************************************

int findInDirectory(DE* dir, int entryCount, const char* name) {
    for (int i = 0; i < entryCount; i++) {
        if ((dir[i].flags & DE_IS_USED) && strcmp(dir[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int isDEaDir(DE* entry) {
    return (entry->flags & DE_IS_DIR) != 0;
}

int ParsePath(const char* path, ppinfo* ppi) {
    vcb* pVCB = _getGlobalVCB();
    DE* cwd = getcwdInternal();
    
    DE* startParent;
    DE* parent;
    char* saveptr;
    char* token1;
    char* token2;
    
    if (path == NULL) return -1;
    
    // Make copy since strtok_r modifies the string
    char* pathCopy = strdup(path);
    if (!pathCopy) return -1;
    
    // determine starting directory
    if (path[0] == '/') {
        startParent = loadDirectory(pVCB->rootStart, pVCB->rootSize, pVCB->blockSize);
    } else {
        startParent = loadDirectory(cwd->location, cwd->size, pVCB->blockSize);
    }
    
    if (!startParent) {
        free(pathCopy);
        return -1;
    }
    
    parent = startParent;
    int parentEntryCount = (path[0] == '/') ? 
                           (pVCB->rootSize / sizeof(DE)) : 
                           (cwd->size / sizeof(DE));
    
    // get first token
    token1 = strtok_r(pathCopy, "/", &saveptr);
    
    if (token1 == NULL) {
        // empty path or just "/"
        if (path[0] == '/') {
            ppi->parent = parent;
            ppi->lastElementName = NULL;
            ppi->index = -2;  // refers to root itself
            free(pathCopy);
            return 0;
        } else {
            if (parent != startParent) free(parent);
            free(pathCopy);
            return -1;
        }
    }
    
    // traverse using two tokens
    while (1) {
        token2 = strtok_r(NULL, "/", &saveptr);
        
        // find token1 in current directory
        int idx = findInDirectory(parent, parentEntryCount, token1);
        
        if (token2 == NULL) {
            // token1 is the last element, this is what we're looking for
            ppi->parent = parent;
            ppi->lastElementName = strdup(token1);  // must strdup to persist after pathCopy freed
            ppi->index = idx;
            free(pathCopy);
            return 0;
        }
        
        // not the last element, so it must exist and be a directory
        if (idx == -1) {
            if (parent != startParent) free(parent);
            free(pathCopy);
            return -2;  // not found
        }
        
        if (!isDEaDir(&parent[idx])) {
            if (parent != startParent) free(parent);
            free(pathCopy);
            return -3;  // not a directory
        }
        
        // load the next directory in the path
        DE* tempParent = loadDirectory(parent[idx].location, 
                                       parent[idx].size, 
                                       pVCB->blockSize);
        if (tempParent == NULL) {
            if (parent != startParent) free(parent);
            free(pathCopy);
            return -1;
        }
        
        // update for next iteration
        parentEntryCount = parent[idx].size / sizeof(DE);
        
        if (parent != startParent) {
            free(parent);
        }
        
        parent = tempParent;
        token1 = token2;
    }
}

// CWD FUNCTIONS

DE* cwdStack[MAX_PATH_DEPTH];
DE* rootDirectory;
int cwdLevel;

DE* cwdStackCopy[MAX_PATH_DEPTH];
int cwdLevelCopy;

int initCwdAtRoot() {
    // get global VCB
    vcb* pVcb = _getGlobalVCB();

    // load root directory to memory
    rootDirectory = loadDirectory(pVcb->rootStart, pVcb->rootSize, pVcb->blockSize);
    if (rootDirectory == NULL) {
        return -1;
    }

    // set root DE in cwd stack
    cwdStack[0] = &rootDirectory[0];
    cwdLevel = 0;

    // set all others in stack to NULL
    for (int i = 1; i < MAX_PATH_DEPTH; i++) {
        cwdStack[i] = NULL;
    }

    return 0;
}

void freeCwdStack(DE** stackToFree) {
    for (int i = 1; i < MAX_PATH_DEPTH; i++) {
        if (stackToFree[i] != NULL) {
            free(stackToFree[i]);
            stackToFree[i] = NULL;
        }
    }
}

void freeCwdMemory() {
    freeCwdStack(cwdStack);
    freeCwdStack(cwdStackCopy);

    free(rootDirectory);
    rootDirectory = NULL;
}

int saveCwdState() {
    // copy root directory pointer
    cwdStackCopy[0] = &rootDirectory[0];
    cwdLevelCopy = cwdLevel;

    // copy over all DE's
    for (int i = 1; i <= cwdLevel; i++) {
        if (cwdStack[i] != NULL) {
            cwdStackCopy[i] = malloc(sizeof(DE));
            if (cwdStackCopy[i] == NULL) {
                freeCwdStack(cwdStackCopy);
                return -1;
            }

            memcpy(cwdStackCopy[i], cwdStack[i], sizeof(DE));
        }
    }

    return 0;
}

void restoreCwdState() {
    // reset cwd stack
    freeCwdStack(cwdStack);

    memcpy(&cwdStack[1], &cwdStackCopy[1], (MAX_PATH_DEPTH - 1) * sizeof(DE*));
    cwdLevel = cwdLevelCopy;

    freeCwdStack(cwdStackCopy);
}

char* cwdBuildAbsPath() {
    // get length of final path string
    int finalLength = 0;
    for (int i = 0; i <= cwdLevel; i++) {
        if (cwdStack[i] == NULL) {
            break;
        }
        finalLength++;
        finalLength += strlen(cwdStack[i]->name);
    }

    // allocate space for string
    char* ret = malloc(finalLength);
    if (ret == NULL) {
        return NULL;
    }

    // build string
    strcpy(ret, "/");
    size_t retIdx = 1;
    for (int i = 1; i <= cwdLevel; i++) {
        strcpy(&ret[retIdx], cwdStack[i]->name);
        retIdx += strlen(cwdStack[i]->name);
        strcpy(&ret[retIdx++], "/");
    }

    // return path
    return ret;
}

int setcwdInternal(const char* path) {
    vcb* pVcb = _getGlobalVCB();
    if (pVcb == NULL) {
        printf("Could not fetch global VCB instance\n");
        return -1;
    }

    // safety check on path
    if (path == NULL) {
        printf("Path was NULL\n");
        return -1;
    }

    // use parsePath to check for path validity
    ppinfo ppreturn;
    int r = ParsePath(path, &ppreturn);
    if (r != 0) {
        printf("Path was not parseable\n");
        return r;
    }

    // we are tokenizing again
    // NOTE: this contains some duplicate code from parsePath; however, splitting
    // these up is probably the more prudent way to do it for now
    // copy path
    char* pathCopy = strdup(path);
    if (pathCopy == NULL) {
        perror("strdup");
        return -1;
    }

    // if absolute path, pop all directories except root from cwd
    if (path[0] == '/') {
        for (int i = 1; i < MAX_PATH_DEPTH; i++) {
            if (cwdStack[i] != NULL) {
                free(cwdStack[i]);
                cwdStack[i] = NULL;
            }
        }
        cwdLevel = 0;
    }

    char* saveptr;
    char* token;
    token = strtok_r(pathCopy, "/", &saveptr);
    // path is nothing; do not change directory
    if (token == NULL) {
        return 0;
    }

    DE* parent = loadDirectory(cwdStack[cwdLevel]->location,
                               cwdStack[cwdLevel]->size,
                               pVcb->blockSize);
    if (parent == NULL) {
        restoreCwdState();
        free(pathCopy);
        return -1;
    }

    uint32_t parentEntryCount = parent[0].size / sizeof(DE);

    // start walking up the path
    while (1) {
        // if token is .., move to parent by popping one from cwd stack
        if (strcmp(token, "..") == 0) {
            if (cwdLevel != 0) {
                free(cwdStack[cwdLevel]);
                cwdStack[cwdLevel] = NULL;
                cwdLevel--;
                parent = loadDirectory(cwdStack[cwdLevel]->location,
                                       cwdStack[cwdLevel]->size,
                                       pVcb->blockSize);
            }
        }
        // if token is ., do nothing
        else if (strcmp(token, ".") != 0) {
            // find token in current directory
            int idx = findInDirectory(parent, parentEntryCount, token);
            if (idx < 0) {
                restoreCwdState();
                free(pathCopy);
                free(parent);
                printf("could not find token in current directory\n");
                return idx;
            }

            // increment cwd level, but be careful not to overshoot MAX_PATH_DEPTH
            cwdLevel++;
            if (cwdLevel == MAX_PATH_DEPTH) {
                free(pathCopy);
                free(parent);
                printf("Iterated beyond maximum path depth\n");
                restoreCwdState();
                return -1;
            }

            if (!(parent[idx].flags & DE_IS_USED)) {
                free(pathCopy);
                free(parent);
                printf("Attempted to access nonexistent directory\n");
                restoreCwdState();
                return -1;
            }

            if (!(parent[idx].flags & DE_IS_DIR)) {
                free(pathCopy);
                free(parent);
                printf("Attempted to access file as directory\n");
                restoreCwdState();
                return -1;
            }

            cwdStack[cwdLevel] = malloc(sizeof(DE));
            memcpy(cwdStack[cwdLevel], &parent[idx], sizeof(DE));

        }

        token = strtok_r(saveptr, "/", &saveptr);
        if (token == NULL) {
            free(pathCopy);
            free(parent);
            return 0;
        }

        // load new parent directory into memory
        free(parent);
        parent = loadDirectory(cwdStack[cwdLevel]->location,
                                    cwdStack[cwdLevel]->size,
                                    pVcb->blockSize);
        if (parent == NULL) {
            free(pathCopy);
            restoreCwdState();
            printf("Could not load directory\n");
            return -1;
        }
        parentEntryCount = parent[0].size / sizeof(DE);
    }
}

DE* getcwdInternal() {
    return cwdStack[cwdLevel];
}

// helper functions for directory creation

int addEntryToDirectory(DE* parent, DE* newEntry) {
    // get global VCB
    vcb* globalVCB = _getGlobalVCB();
    if (globalVCB == NULL) {
        return -1;
    }
    
    uint32_t numEntries = parent->size / sizeof(DE);

    DE* loadedDir = loadDirectory(parent->location, parent->size, globalVCB->blockSize);
    if (loadedDir == NULL) {
        return -1;
    }

    // find an appropriate spot
    int insertionIdx = 0;
    for (int i = 2; i < numEntries; i++) {
        if (!(parent[i].flags & DE_IS_USED)) {
            insertionIdx = i;
            break;
        }
    }
    
    // if we couldn't insert, allocate more space in directory
    if (insertionIdx == 0) {
        // check if we need to allocate more blocks
        uint32_t numBlocksCurrent = parent->size / globalVCB->blockSize;
        uint32_t newSize = parent->size + sizeof(DE);
        uint32_t numBlocksNew = newSize / globalVCB->blockSize;

        // if yes, allocate the additional disk space
        if (numBlocksNew > numBlocksCurrent) {
            int r = resizeBlocks(parent->location, parent->size + sizeof(DE));
            if (r != 0) {
                free(loadedDir);
                return r;
            }
        }
        // set insertion index to new final entry
        insertionIdx = numEntries;

        // reload directory and set new size
        free(loadedDir);
        loadedDir = loadDirectory(parent->location,
                                  parent->size + sizeof(DE),
                                  globalVCB->blockSize);
        if (loadedDir == NULL) {
            return -1;
        }
        loadedDir[0].size += sizeof(DE);
        parent->size += sizeof(DE);
    }

    // copy in the new entry to its proper place
    memcpy(&loadedDir[insertionIdx], newEntry, sizeof(DE));

    // write blocks to disk
    uint32_t numBlocks = (parent->size + globalVCB->blockSize - 1) / globalVCB->blockSize;
    printf("got to end of addEntryToDirectory...\n");
    int r = writeBlocksToDisk((char *)loadedDir, parent->location, numBlocks);
    if (r != numBlocks) {
        return -1;
    }

    // release resources
    free(loadedDir);

    return 0;
}

int removeEntryFromDirectory(DE* parent, const char* entryName) {
    vcb* pVcb = _getGlobalVCB();
    if (pVcb == NULL) {
        return -1;
    }

    printf("inside removeEntryFromDirectory\n");
    int numEntries = parent->size / sizeof(DE);
    DE* dirToRemove = findEntryInDirectory(parent, numEntries, entryName);
    if (dirToRemove == NULL) {
        return -1;
    }

    // set bit to unused to clear entry
    dirToRemove->flags &= ~DE_IS_USED;

    int numBlocks = parent[0].size / pVcb->blockSize;
    int r = writeBlocksToDisk((char *)parent, parent[0].location, numBlocks);
    if (r != numBlocks) {
        return -1;
    }

    return 0;
}


int isDirectoryEmpty(DE* dir, uint32_t numEntries) {
    // if there are only two entries, it's empty by definition
    if (numEntries == 2) {
        return 1;
    }

    // iterate through entries and return false if we see one that's used
    for (int i = 2; i < numEntries; i++) {
        if ((dir[i].flags & DE_IS_USED) != 0) {
            return 0;
        }
    }

    // reaching this point means we didn't see a used entry, so return true
    return 1;
}

void uninitCwdSystem() {
    freeCwdMemory();
}