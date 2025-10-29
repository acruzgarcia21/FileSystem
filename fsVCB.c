#include "fsVCB.h"
#include "fsFreeSpace.h"
#include "fsDirectory.h"

vcb globalVCB;

vcb* _getGlobalVCB() {
    return &globalVCB;
}

int initVCB(vcb* pVCB, uint64_t numberOfBlocks, uint64_t blockSize) {
    // create signature
    pVCB->signature = FS_VCB_MAGIC;

    // initialize values given to us
    pVCB->numBlocks = numberOfBlocks;
    pVCB->blockSize = blockSize;

    // initialize freespace system
    int r = initFAT(pVCB);
    if (r != 0) {
        return r;
    }

    // initialize root directory entry
    DE* rootEntry = createDir(FS_ROOT_SIZE, NULL, pVCB->blockSize);
    if (rootEntry == NULL) {
        return -1;
    }
    pVCB->rootStart = rootEntry->location;
    pVCB->rootSize = rootEntry->size;
    
    // free allocated memory for the root directory
    free(rootEntry);

    // write vcb to disk at block 0
    r = LBAwrite(pVCB, 1, 0);
    if (r != 0) {
        return -1;
    }

    return 0;
}