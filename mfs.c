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

// int fs_isFile(char * filename)
// {
    
//     return 1;
// }

// int fs_isDir(char * pathname)
// {
    
//     return 1;
// }

// int fs_delete(char* filename)
// {
    
// }
