/**************************************************************
* Class::  CSC-415-02 Spring 2024
* Name:: Ronin Lombardino, Alexander Tamayo
* Student IDs:: 924363164, 921199718
* GitHub-Name:: RookAteMySSD
* Group-Name:: Team #1 Victory Royal
* Project:: Basic File System
*
* File:: mfs.h
*
* Description:: 
*	
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "mfs.h"
#include "fsVCB.h"
#include "fsDirectory.h"
#include "fsFreeSpace.h"
#include "fsLow.h"

// =============== DIRECTORY CREATION =============
int fs_mkdir(const char *pathname, mode_t mode) {

    // validate input
    if (!pathname || strlen(pathname) == 0) {
        errno = EINVAL;
        return -1;
    }
    
    if (strcmp(pathname, "/") == 0) {
        errno = EEXIST;
        return -1;
    }
    
    vcb* pVCB = _getGlobalVCB();
    ppinfo ppi;
    
    // parse path and navigate to parent
    int result = ParsePath(pathname, &ppi);
    
    if (result == -1) {
        errno = EIO;
        return -1;
    }
    
    if (result == -2) {
        // parent path doesn't exist
        errno = ENOENT;
        return -1;
    }
    
    if (result == -3) {
        // parent path contains non-directory
        errno = ENOTDIR;
        return -1;
    }
    
    // check name length
    if (strlen(ppi.lastElementName) >= DE_NAME_MAX) {
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = ENAMETOOLONG;
        return -1;
    }
    
    // check for "." or ".."
    if (strcmp(ppi.lastElementName, ".") == 0 || 
        strcmp(ppi.lastElementName, "..") == 0) {
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = EINVAL;
        return -1;
    }
    
    // check if already exists
    if (ppi.index != -1) {
        // entry already exists
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = EEXIST;
        return -1;
    }
    
    // create new directory (10 entries: ".", "..", + 8 empty)
    DE* newDir = createDir(10, &ppi.parent[0], pVCB->blockSize);
    if (!newDir) {
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = ENOSPC;
        return -1;
    }
    
    // create entry for parent
    DE newEntry;
    memset(&newEntry, 0, sizeof(DE));
    strcpy(newEntry.name, ppi.lastElementName);
    newEntry.flags = DE_IS_DIR | DE_IS_USED;
    newEntry.size = newDir[0].size;
    newEntry.location = newDir[0].location;
    newEntry.created = newDir[0].created;
    newEntry.accessed = newDir[0].accessed;
    newEntry.modified = newDir[0].modified;
    
    free(newDir);
    
    // add to parent (parent is already loaded!)
    uint32_t parentLocation = ppi.parent[0].location;
    uint32_t parentSize = ppi.parent[0].size;
    
    int addResult = addEntryToDirectory(ppi.parent, &newEntry);
    
    free(ppi.parent);
    free(ppi.lastElementName);
    
    if (addResult < 0) {
        freeBlocks(newEntry.location);
        return -1;
    }
    
    return 0;
}
// ==================== DIRECTORY REMOVAL ====================

int fs_rmdir(const char *pathname) {
    printf("started rmdir\n");

    if (!pathname || strlen(pathname) == 0) {
        printf("pathname was like nothing\n");
        errno = EINVAL;
        return -1;
    }
    
    if (strcmp(pathname, "/") == 0) {
        printf("pathname was root\n");
        errno = EBUSY;
        return -1;
    }
    
    vcb* pVCB = _getGlobalVCB();
    DE* cwd = getcwdInternal();
    ppinfo ppi;
    
    int result = ParsePath(pathname, &ppi);
    
    if (result != 0) {
        printf("couldn't parse path\n");
        errno = (result == -2) ? ENOENT : EIO;
        return -1;
    }
    
    // check for "." or ".."
    if (strcmp(ppi.lastElementName, ".") == 0 || 
        strcmp(ppi.lastElementName, "..") == 0) {
        printf("invalid path\n");
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = EINVAL;
        return -1;
    }
    
    // must exist
    if (ppi.index == -1) {
        printf("does not exist\n");
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = ENOENT;
        return -1;
    }
    
    DE* targetEntry = &ppi.parent[ppi.index];
    
    // must be a directory
    if (!isDEaDir(targetEntry)) {
        printf("not dir\n");
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = ENOTDIR;
        return -1;
    }
    
    // can't be CWD
    if (targetEntry->location == cwd->location) {
        printf("is cwd\n");
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = EBUSY;
        return -1;
    }
    
    // load target to check if empty
    DE* targetDir = loadDirectory(targetEntry->location, targetEntry->size, pVCB->blockSize);
    if (!targetDir) {
        printf("couldn't load\n");
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = EIO;
        return -1;
    }
    
    int targetEntryCount = targetEntry->size / sizeof(DE);
    if (!isDirectoryEmpty(targetDir, targetEntryCount)) {
        printf("wasn't empty\n");
        free(targetDir);
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = ENOTEMPTY;
        return -1;
    }
    
    uint32_t locationToFree = targetEntry->location;
    free(targetDir);
    
    // Remove from parent
    uint32_t parentLocation = ppi.parent[0].location;
    uint32_t parentSize = ppi.parent[0].size;
    
    if (removeEntryFromDirectory(ppi.parent, ppi.lastElementName) != 0) {
        printf("couldn't remove\n");
        free(ppi.parent);
        free(ppi.lastElementName);
        return -1;
    }
    
    free(ppi.parent);
    free(ppi.lastElementName);
    
    // Free blocks
    if (freeBlocks(locationToFree) != 0) {
        errno = EIO;
        return -1;
    }
    
    return 0;
}

//Directory iteration functions
fdDir * fs_opendir(const char *pathname) {

    //validate input and the path are valid
    if (!pathname || strlen(pathname) == 0) {
        return NULL;
    }

    ppinfo ppi;
    int result = ParsePath(pathname, &ppi);

    if (result < 0) {
        return NULL;
    }

    //create our struct
    fdDir* dirInfo = malloc(sizeof(fdDir));
    if (!dirInfo) {
        free(ppi.parent);
        if(ppi.lastElementName) free(ppi.lastElementName);
        return NULL;
    }

    vcb* pVCB =_getGlobalVCB();
    if(!pVCB)
    {
        free(dirInfo);
        free(ppi.parent);
        if(ppi.lastElementName) free(ppi.lastElementName);
        return NULL;
    }

    DE* directory = NULL;

    if(ppi.index == -2)
    {
        // path is the root
        directory = loadDirectory(pVCB->rootStart,
                                  pVCB->rootSize,
                                  pVCB->blockSize);
    }
    else if (ppi.index >= 0)
    {
        // Normal dir path like "/foo" or "/bar"
        DE* entry = &ppi.parent[ppi.index];
        if(!(entry->flags & DE_IS_DIR))
        {
            // Not a directory
            free(dirInfo);
            free(ppi.parent);
            if(ppi.lastElementName) free(ppi.lastElementName);
            return NULL;
        }
        directory = loadDirectory(entry->location,
                                  entry->size,
                                  pVCB->blockSize);
    }
    else
    {
        // ppi.index == -1 : last element not found
        free(dirInfo);
        free(ppi.parent);
        if(ppi.lastElementName) free(ppi.lastElementName);
        return NULL;
    }

    if (!directory) {
        free(dirInfo);
        free(ppi.parent);
        if(ppi.lastElementName) free(ppi.lastElementName);
        return NULL;
    }

    //populate the struct
    dirInfo->directory = directory;
    dirInfo->d_reclen = directory[0].size / sizeof(DE); // Number of entries
    dirInfo->dirEntryPosition = 0;

    dirInfo->di = malloc(sizeof(struct fs_diriteminfo));
    if (!dirInfo->di) {
        free(dirInfo->directory);
        free(dirInfo);
        free(ppi.parent);
        if(ppi.lastElementName) free(ppi.lastElementName);
        return NULL;
    }

    //Free the parent info of the directory we want
    free(ppi.parent);
    if(ppi.lastElementName) free(ppi.lastElementName);

    return dirInfo;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
    //validate input
    if (!dirp) {
        return NULL;
    }

    //if we've reached the end of the directory, return NULL
    if (dirp->dirEntryPosition >= dirp->d_reclen) {
        return NULL;
    }

    DE currentEntry;
    
    //get the current directory entry
    do {
        currentEntry = dirp->directory[dirp->dirEntryPosition];
        dirp->dirEntryPosition++;
    } while (!(currentEntry.flags & DE_IS_USED) 
                && dirp->dirEntryPosition < dirp->d_reclen);

    //populate the fs_diriteminfo struct
    struct fs_diriteminfo * diritem =  dirp->di; //refrence ptr
    diritem->d_reclen = currentEntry.size;
    
    if (currentEntry.flags & DE_IS_DIR) {
        diritem->fileType = FT_DIRECTORY;
    } else {
        diritem->fileType = FT_REGFILE;
    }

    strcpy(diritem->d_name, currentEntry.name);

    return dirp->di;
}

int fs_closedir(fdDir *dirp) {
    //validate input
    if (!dirp) {
        return -1;
    }

    //free allocated memory
    if (dirp->directory) {
        free(dirp->directory);
    }
    if (dirp->di) {
        free(dirp->di);
    }
    free(dirp);

    return 0;
}

int fs_isFile(char* path) 
{
    if (!path) return 0;

    ppinfo ppi = {0};
    int r = ParsePath(path, &ppi);
    if (r != 0) {
        return 0;
    }

    int answer = 0;

    if (ppi.index >= 0)
    {
        DE* entry = &ppi.parent[ppi.index];
        answer = ((entry->flags & DE_IS_USED) && !(entry->flags & DE_IS_DIR)) ? 1 : 0;
    }

    if(ppi.parent) free(ppi.parent);
    if(ppi.lastElementName) free(ppi.lastElementName);

    return answer;
}

int fs_isDir(char* path) 
{
    if (!path) return 0;

    ppinfo ppi = {0};
    int r = ParsePath(path, &ppi);
    if (r != 0) return 0; // Not a dir / not found

    int result = 0;

    if (ppi.index >= 0)
    {
        DE* entry = &ppi.parent[ppi.index];
        result = ((entry->flags & DE_IS_USED) && (entry->flags & DE_IS_DIR)) ? 1 : 0;
    }
    else if (ppi.index == -2) // root
    {
        result = 1;
    }

    if (ppi.parent) free(ppi.parent);
    if (ppi.lastElementName) free(ppi.lastElementName);
    
    return result;
}

int fs_delete(char* filename)
{
    // Validate input
    if(!filename || strlen(filename) == 0) return -1;

    // Load VCB
    vcb* vcb = _getGlobalVCB();
    if(!vcb) return -1;

    ppinfo ppi;
    int r = ParsePath(filename, &ppi);
    if (r != 0) {
        return -1;
    }

    DE* entry = &ppi.parent[ppi.index];

    // Safety check: ensure entry exists
    if(!entry)
    {
        free(ppi.parent);
        ppi.parent = NULL;
        return -1;
    }
    // Prevent deletion of '.' or '..'
    if (strcmp(entry->name, ".") == 0 || strcmp(entry->name,  "..") == 0)
    {
        free(ppi.parent);
        ppi.parent = NULL;
        return -1;
    }

    // Validate that this is an existing file, not a directory
    if(!entry || !(entry->flags & DE_IS_USED) || (entry->flags & DE_IS_DIR))
    {
        free(ppi.parent);
        ppi.parent = NULL;
        return -1;
    }

    // Free the file's blocks and clear its DE
    if(entry->location != 0) freeBlocks(entry->location);

    // Reset directory entry fields
    memset(entry->name, 0, DE_NAME_MAX);
    entry->flags    = 0;
    entry->size     = 0;
    entry->location = 0;
    entry->created  = 0;
    entry->accessed = 0;
    entry->modified = 0;

    // Write parent directory back to disk
    int blocksToWrite = (int)((ppi.parent[0].size + vcb->blockSize - 1) / vcb->blockSize);
    int wrote = writeBlocksToDisk((char*)ppi.parent, ppi.parent[0].location, blocksToWrite);

    // Verify the write completed fully
    if(wrote != blocksToWrite)
    {
        printf("fs_delete: failed to write directory back to disk (wrote %d of %d)\n", wrote, blocksToWrite);
        free(ppi.parent);
        ppi.parent = NULL;
        return -1;
    }

    // Cleanup and exit
    free(ppi.parent);
    ppi.parent = NULL;
    return 0;
}

int fs_setcwd(char *pathname) {
    return setcwdInternal(pathname);
}

char * fs_getcwd(char *pathname, size_t size) {
    char* absPath = cwdBuildAbsPath();
    if (absPath == NULL) {
        return NULL;
    }

    if (strlen(absPath) > size) {
        return NULL;
    }

    strncpy(pathname, absPath, size);
    free(absPath);
    return pathname;
}

int fs_stat(const char *path, struct fs_stat *buf) 
{
    if (!path || !buf) return -1;

    vcb* pVCB = _getGlobalVCB();
    if(!pVCB) return -1;

    ppinfo ppi = {0};
    int r = ParsePath(path, &ppi);
    if (r != 0) {
        return -1;
    }

    DE* entry = NULL;

    if(ppi.index == -2)
    {
        // Path is root
        // ParsePath already loaded the root int ppi.parent
        entry = &ppi.parent[0];
    }
    else if (ppi.index >= 0)
    {
        entry = &ppi.parent[ppi.index];
    }
    else
    {
        if (ppi.parent) free(ppi.parent);
        if (ppi.lastElementName) free(ppi.lastElementName);
        return -1;
    }

    buf->st_size       = entry->size;
    buf->st_blocks     = (entry->size + pVCB->blockSize - 1) / pVCB->blockSize;
    buf->st_accesstime = entry->accessed;
    buf->st_modtime    = entry->modified;
    buf->st_createtime = entry->created;

    if (ppi.parent) free(ppi.parent);
    if (ppi.lastElementName) free(ppi.lastElementName);

    return 0;
}
