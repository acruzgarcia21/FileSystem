#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    
    int addResult = addEntryToDirectory(parentLocation, parentSize, &newEntry);
    
    free(ppi.parent);
    free(ppi.lastElementName);
    
    if (addResult != 0) {
        freeBlocks(newEntry.location);
        return -1;
    }
    
    return 0;
}
// ==================== DIRECTORY REMOVAL ====================

int fs_rmdir(const char *pathname) {
    if (!pathname || strlen(pathname) == 0) {
        errno = EINVAL;
        return -1;
    }
    
    if (strcmp(pathname, "/") == 0) {
        errno = EBUSY;
        return -1;
    }
    
    vcb* pVCB = _getGlobalVCB();
    CWD* cwd = getCWD();
    ppinfo ppi;
    
    int result = ParsePath(pathname, &ppi);
    
    if (result != 0) {
        errno = (result == -2) ? ENOENT : EIO;
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
    
    // must exist
    if (ppi.index == -1) {
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = ENOENT;
        return -1;
    }
    
    DE* targetEntry = &ppi.parent[ppi.index];
    
    // must be a directory
    if (!isDEaDir(targetEntry)) {
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = ENOTDIR;
        return -1;
    }
    
    // can't be CWD
    if (targetEntry->location == cwd->location) {
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = EBUSY;
        return -1;
    }
    
    // load target to check if empty
    DE* targetDir = loadDirectory(targetEntry->location, targetEntry->size, pVCB->blockSize);
    if (!targetDir) {
        free(ppi.parent);
        free(ppi.lastElementName);
        errno = EIO;
        return -1;
    }
    
    int targetEntryCount = targetEntry->size / sizeof(DE);
    if (!isDirectoryEmpty(targetDir, targetEntryCount)) {
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
    
    free(ppi.parent);
    
    if (removeEntryFromDirectory(parentLocation, parentSize, ppi.lastElementName) != 0) {
        free(ppi.lastElementName);
        return -1;
    }
    
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
        free(ppi.lastElementName);
        return NULL;
    }

    //Load the directory we'll be iterating through
    DE * directory = loadDirectory(ppi.parent[ppi.index].location,
                      ppi.parent[ppi.index].size, _getGlobalVCB()->blockSize);

    if (!directory) {
        free(dirInfo);
        free(ppi.parent);
        free(ppi.lastElementName);
        return NULL;
    }

    //populate the struct
    dirInfo->directory = directory;
    dirInfo->d_reclen = directory->size / sizeof(DE);
    dirInfo->dirEntryPosition = 0;
    dirInfo->di = malloc(sizeof(struct fs_diriteminfo));
    if (!dirInfo->di) {
        free(dirInfo->directory);
        free(dirInfo);
        free(ppi.parent);
        free(ppi.lastElementName);
        return NULL;
    }

    //Free the parent info of the directory we want
    free(ppi.parent);
    free(ppi.lastElementName);

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

    //get the current directory entry
    DE currentEntry = dirp->directory[dirp->dirEntryPosition];

    //populate the fs_diriteminfo struct
    struct fs_diriteminfo * diritem =  dirp->di; //refrence ptr
    diritem->d_reclen = currentEntry.size;
    
    if (currentEntry.flags & DE_IS_DIR) {
        diritem->fileType = FT_DIRECTORY;
    } else {
        diritem->fileType = FT_REGFILE;
    }

    strcpy(diritem->d_name, currentEntry.name);

    //increment position for next read
    dirp->dirEntryPosition += 1;

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

int fs_isFile(char * filename)
{
    // Validate input filename
    if(!filename || strlen(filename) == 0) return 0;

    vcb* vcb = _getGlobalVCB();
    if(!vcb) return 0;

    // Load root directory into memory
    DE* currentDirectory = loadDirectory(vcb->rootStart, vcb->rootSize, vcb->blockSize);
    if(!currentDirectory) return 0;

    uint32_t currentSize = vcb->rootSize;

    // Skip leading '/' if present
    while (*filename == '/') filename ++;

    char* token = strtok(filename, "/");
    DE* entry = NULL;

    // Walk through each path component (directory or file)
    while(token)
    {
        // Compute how many entries exist in the current directory
        int entryCount = (int)(currentSize / sizeof(DE));
        entry = findEntryInDirectory(currentDirectory, entryCount, token);
        if(!entry) 
        {
            // Not found
            free(currentDirectory);
            currentDirectory = NULL;
            return 0;
        }

        // Check if there is another compinent (subdirectory)
        char* next = strtok(NULL, "/");
        if(next)
        {
            // Not a directory. Invalid path
            if(!(entry->flags & DE_IS_DIR))
            {
                free(currentDirectory);
                currentDirectory = NULL;
                return 0;
            }

            // Load next directory from disk
            DE* nextDir = loadDirectory(entry->location, entry->size, vcb->blockSize);
            if(!nextDir)
            {
                free(currentDirectory);
                currentDirectory = NULL;
                return 0;
            }

            // Free current and continue into the next directory
            free(currentDirectory);
            currentDirectory = nextDir;
            // Update size for new directory
            currentSize = entry->size;

            token = next;
        }
        else break;
    }

    // Confirm that final entry is a used file (not a directory)
    int isFile = (entry && (entry->flags & DE_IS_USED) && !(entry->flags & DE_IS_DIR)) ? 1 : 0;
    free(currentDirectory);
    currentDirectory = NULL;
    return isFile;
}

int fs_isDir(char * pathname)
{
    // Validate input path
    if(!pathname || strlen(pathname) == 0) return 0;

    // Get VCB
    vcb* vcb = _getGlobalVCB();
    if(!vcb) return 0;

    // Load root directory
    DE* currentDirectory = loadDirectory(vcb->rootStart, vcb->rootSize, vcb->blockSize);
    if(!currentDirectory) return 0;

    uint32_t currentSize = vcb->rootSize;

    // Skip leading '/' if present
    while (*pathname == '/') pathname++;

    // Tokenize path
    char* token = strtok(pathname, "/");
    DE* entry = NULL;

    // Walk through directories until reaching last component
    while(token)
    {
        int entryCount = (int)(currentSize / sizeof(DE));
        entry = findEntryInDirectory(currentDirectory, entryCount, token);

        if(!entry)
        {
            free(currentDirectory);
            currentDirectory = NULL;
            return 0;
        }

        char* next = strtok(NULL, "/");
        if(next)
        {
            // Must be a directory
            if(!(entry->flags & DE_IS_DIR))
            {
                free(currentDirectory);
                currentDirectory = NULL;
                return 0;
            }

            // Load next subdirectory
            DE* nextDir = loadDirectory(entry->location, entry->size, vcb->blockSize);
            if(!nextDir)
            {
                free(currentDirectory);
                currentDirectory = NULL;
                return 0;
            }

            free(currentDirectory);
            currentDirectory = nextDir;
            // Update size for new directory
            currentSize = entry->size;

            token = next;
        } 
        else 
        {
            break;
        }
    }

    // Check that this final entry is a directory and used
    int isDir = (entry && (entry->flags & DE_IS_USED) && (entry->flags & DE_IS_DIR)) ? 1 : 0;
    free(currentDirectory);
    currentDirectory = NULL;
    return isDir;
}

int fs_delete(char* filename)
{
    // Validate input
    if(!filename || strlen(filename) == 0) return -1;

    // Load VCB
    vcb* vcb = _getGlobalVCB();
    if(!vcb) return -1;

    // Load root directory
    DE* currentDirectory = loadDirectory(vcb->rootStart, vcb->rootSize, vcb->blockSize);
    if(!currentDirectory) return -1;

    // Track the parent directory's size so we know how many entries to scan
    uint32_t parentSize  = vcb->rootSize; 
    uint32_t parentStart = vcb->rootStart;

    // Skip any leading slashes
    while (*filename == '/') filename++;
    
    DE* parentDir   = currentDirectory; // Directory that contains the target
    DE* parentEntry = NULL;             // The DE of the parent dir (within its parent)
    DE* entry       = NULL;

    // Tokenise path
    char* token = strtok(filename, "/");

    // Traverse path until the target file is found
    while(token)
    {
        int entryCount = (int)(parentSize / sizeof(DE));
        entry = findEntryInDirectory(parentDir, entryCount, token);
        if(!entry)
        {
            free(parentDir);
            parentDir = NULL;
            return -1;
        }

        char* next = strtok(NULL, "/");
        if(next)
        {
            // Must be a directory if we have more tokens to walk through
            if(!(entry->flags & DE_IS_DIR))
            {
                free(parentDir);
                parentDir = NULL;
                return -1;
            }

            // Load next directory in chain
            DE* nextDir = loadDirectory(entry->location, entry->size, vcb->blockSize);
            if(!nextDir)
            {
                free(parentDir);
                parentDir = NULL;
                return -1;
            }

            free(parentDir);
            parentDir   = nextDir;
            parentEntry = entry;
            parentStart = entry->location;
            parentSize  = entry->size;

            token = next;
        } 
        else
        {
            break;
        }
    }

    // Safety check: ensure entry exists
    if(!entry)
    {
        free(parentDir);
        parentDir = NULL;
        return -1;
    }
    // Prevent deletion of '.' or '..'
    if (strcmp(entry->name, ".") == 0 || strcmp(entry->name,  "..") == 0)
    {
        free(parentDir);
        parentDir = NULL;
        return -1;
    }

    // Validate that this is an existing file, not a directory
    if(!entry || !(entry->flags & DE_IS_USED) || (entry->flags & DE_IS_DIR))
    {
        free(parentDir);
        parentDir = NULL;
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
    int blocksToWrite = (int)((parentSize + vcb->blockSize - 1) / vcb->blockSize);
    int wrote = writeBlocksToDisk((char*)parentDir, parentStart, blocksToWrite);

    // Verify the write completed fully
    if(wrote != blocksToWrite)
    {
        printf("fs_delete: failed to write directory back to disk (wrote %d of %d)\n", wrote, blocksToWrite);
        free(parentDir);
        parentDir = NULL;
        return -1;
    }

    // Cleanup and exit
    free(parentDir);
    parentDir = NULL;
    return 0;
}