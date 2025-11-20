/**************************************************************
* Class::  CSC-415-0# Fall 2025
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: b_io.c
*
* Description:: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "b_io.h"
#include "fsVCB.h"
#include "fsDirectory.h"
#include "fsLow.h"
#include "fsFreeSpace.h"

#define MAXFCBS 20

typedef struct b_fcb
	{
	/** TODO add al the information you need in the file control block **/
	char * buf;			 //holds the open file buffer
	int index;			 //holds the current position in the buffer
	int buflen;			 //holds how many valid bytes are in the buffer

	uint32_t startBlock; // first data block (FAT head)
	uint32_t fileSize;   // Current file size in bytes
	uint32_t filePOS;	 // Byte Cursor within file
	int bufBlockIdx;	 // Which logical file block is cached in buf, -1 if none
	int bufDirty; 		 // 1 if buffer must be flushed to disk
	int flags;			 // O_RDONLY, O_WRONLY, O_RDWR, O_APPEND
	uint32_t blockSize;  // VCB block size

	int inUse;			 // Mark FCB used

	DE* parentDirectory;	// the parent directory loaded into memory so that
							// we can change modified time, e.g.
	int directoryIndex;		// index within the parent directory
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//

	// handle some bad flag cases
	int count = 0;
	if (flags & O_RDONLY) count++;
	if (flags & O_WRONLY) count++;
	if (flags & O_RDWR) count++;
	if (count != 1) {
		fprintf(stderr,
			    "(b_open) Must specify at most 1 of flags \
				O_RDONLY, O_WRONLY, O_RDWR.");
		return -1;
	}
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();				// get our own file descriptor
										// check for error - all used FCB's

	// get current time (for later)
	uint64_t currentTime = getCurrentTime();
	
	// check for error
	if (returnFd == -1) {
		fprintf(stderr, "(b_open) Out of file control blocks!\n");
		return -1;
	}

	b_fcb* f = &fcbArray[returnFd];
	f->blockSize = _getGlobalVCB()->blockSize;

	// parse path and get directory entry
	ppinfo ppi;
	int r;
	r = ParsePath(filename, &ppi);
	if (r != 0) {
		fprintf(stderr,
				"(b_open) Could not parse filename %s; return code = %d\n",
				filename, r);
		return r;
	}

	// handle O_CREAT; create file if it does not exist
	if (ppi.index == -1) {
		if (flags & O_CREAT) {
			// create file and get index
			int newIndex = createFile(filename, ppi.parent);
			if (newIndex == -1) {
				free(ppi.parent);
				return -1;
			}

			// reload ppi parent directory
			uint32_t reloadLocation = ppi.parent[0].location;
			uint32_t reloadSize = ppi.parent[0].size;
			free(ppi.parent);
			ppi.parent = loadDirectory(reloadLocation, reloadSize, f->blockSize);
			if (ppi.parent == NULL) {
				return -1;
			}

			// assign new index
			ppi.parent = newIndex;
		} else {
			// file does not exist and we are not creating it, so we can't
			// do anything and must exit
			free(ppi.parent);
			return -1;
		}
	}

	// get pointer to the relevant directory entry
	DE* fileEntry = &ppi.parent[ppi.index];

	// handle O_TRUNC
	if (flags & O_TRUNC) {
		// free disk space, set size to 0, and write DE to disk
		freeBlocks(fileEntry->location);
		fileEntry->size = 0;
		fileEntry->modified = currentTime;

		writeBlocksToDisk((char *)ppi.parent,
						  ppi.parent[0].location,
						  (ppi.parent[0].size + f->blockSize - 1) / f->blockSize);
	}

	// handle O_APPEND
	if (flags & O_APPEND) {
		// set filePOS to size of file
		f->filePOS = ppi.parent[ppi.index].size;
	} else {
		// otherwise, start at 0
		f->filePOS = 0;
	}

	// populate buffer, but not if we are on a block boundary!
	if (f->filePOS / f->blockSize != 0) {
		loadBlock(f, f->filePOS / f->blockSize);
	}

	// set access flags
	f->flags = flags;

	// set file accessed timestamp and write DE back to disk
	fileEntry->accessed = currentTime;

	// if we get to this point, all is valid, so mark FCB as used
	f->inUse = 1;
	
	return (returnFd);						// all set
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}

// Reads the logical block 'lbIdx' of this file into f->buf and updates f->buflen.
// Returns 0 on success, -1 on error/EOF
int loadBlock(b_fcb* f, uint32_t lbIdx)
{
	uint32_t lba = getBlockOfFile(f->startBlock, lbIdx);
		
	if(lba == FAT_EOF || lba == FAT_RESERVED) return -1;
	if(LBAread(f->buf, 1, lba) != 1) return -1;

	// Compute how many bytes are valid in this block (EOF)
	uint32_t blocksStartByte = lbIdx * f->blockSize;
	uint32_t bytesLeft = (f->fileSize > blocksStartByte) ? (f->fileSize - blocksStartByte) : 0;
	f->buflen = (bytesLeft < f->blockSize) ? (int)bytesLeft : (int)f->blockSize;
	f->index = 0;
	f->bufBlockIdx = (int)lbIdx;
	return 0;
}

// Flush dirty buffer block back to disk
int flushBlock(b_fcb* f)
{
	if(!f->bufDirty || f->bufBlockIdx < 0) return 0;
	uint32_t lba = getBlockOfFile(f->startBlock, (uint32_t)f->bufBlockIdx);

	if(lba == FAT_EOF || lba == FAT_RESERVED) return -1;
	if(LBAwrite(f->buf, 1, lba) != 1) return -1;
	f->bufDirty = 0;
	return 0;
}

// Make sure the block for filePOS exists and is loaded into f->buf
int ensureBlockForWrite(b_fcb* f)
{
	uint32_t lbIdx = (uint32_t)(f->filePOS / f->blockSize);

	// If buffer already holds this block, good
	if(f->bufBlockIdx == (int)lbIdx) return 0;

	// Try loading the block (it exists already)
	if(loadBlock(f, lbIdx) == 0) return 0;

	// If loading failed we may need to grow the FAT chain.
	// How many blocks does the file currently have
	uint32_t currentBlocks = 
		(f->fileSize + f->blockSize - 1) / f->blockSize;

	// if lbIdx is inside current logical size, this is a real error
	if(lbIdx < currentBlocks) return -1;

	// Otherwise we need to grow: Make sure the chain is at least lbIdx + 1 blocks
	uint32_t newByteSize = (lbIdx + 1) * f->blockSize;

	// Resize the chain
	int r = resizeBlocks(f->startBlock, newByteSize);
	if(r == FAT_EOF || r < 0) return -1;

	// Reload the block after growth
	if(loadBlock(f, lbIdx) < 0) return -1;

	return 0;
}

// Write to the buffer where you position is
// look out for flags
// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); 				 //invalid file descriptor
	}

	b_fcb* f = &fcbArray[fd];
	if(f->buf == NULL) return -1;   // Not open/initialized
	if(count <= 0) return 0;

	int total = 0;

	while(total < count)
	{
		// Ensure the correct FAT block exists & is loaded
		if(ensureBlockForWrite(f) < 0)
			return (total ? total : -1);

		// Determine the offset inside this block
		f->index = (int)(f->filePOS % f->blockSize);
		if(f->index > (int)f->blockSize) 
		{
			f->index = (int)f->blockSize;
		}

		int space = (int)f->blockSize - f->index;  // Free space in this block
		int want  = count - total;				   // Bytes we still need to write
		int n     = (space < want) ? space : want; // Bytes to write this iteration

		// Copy user bytes into buffer
		memcpy(f->buf + f->index, buffer + total, n);

		// Mark dirty and advance pointers
		f->bufDirty = 1;
		f->index    += n;
		f->filePOS  += (uint32_t)n;
		total       += n;
		
		// If file grew past old size, update size
		if(f->filePOS > f->fileSize) 
		{
			f->fileSize = f->filePOS;
		}

		// If block is full, flush now
		if(f->index == (int)f->blockSize)
		{
			if(flushBlock(f) < 0)
				return (total ? total : -1);

			// Force ensureBlockWrite() to load the next block
			f->bufBlockIdx = -1; 
		}
	}
	return total;
}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); 					//invalid file descriptor
	}

	b_fcb* f = &fcbArray[fd];
	if(f->buf == NULL) return -1;					  // Not open
	if(count <= 0) return 0;					      // Nothing to do
	if((uint32_t)f->filePOS >= f->fileSize) return 0; // EOF

	int total = 0;

	while(total < count)
	{
		// Part 1: Use any bytes left in our current buffer 
		// Make sure the buffer we hold corresponds to the logical block of filePOS
		uint32_t needLbIdx = (uint32_t)(f->filePOS / f->blockSize);
		if(f->bufBlockIdx != (int)needLbIdx)
		{
			if(loadBlock(f, needLbIdx) < 0) break;
		}

		// Copy the current buffer first (may be 0 if exavtly at a boundry)
		int offsetInBlock = (int)(f->filePOS % f->blockSize);
		int available = f->buflen - offsetInBlock;
		if(available > 0)
		{
			int want = count - total;
			int toCopy = (available < want) ? available : want;
			memcpy(buffer + total, f->buf + offsetInBlock, (size_t)toCopy);
			f->filePOS += toCopy;
			total += toCopy;

			// If we satisfied the request or reached EOF, we're done
			if(total >= count || (uint32_t)f->filePOS >= f->fileSize) break;
			
			// If we still want more but we're not block-algned yet, continue the block;

		}

		// Part 2: Fast path for full blocks (direct into caller buf)
		// We are either at a block boundry or the current buffer had no usable bytes
		// If the caller still wants >= one full block, read whole blocks straight into buffer
		if((f->filePOS % f->blockSize) == 0 && (count - total) >= (int)f->blockSize)
		{
			// How many whole blocks can we deliver without passing EOF?
			uint32_t bytesLeftInFile = (f->fileSize > (uint32_t)f->filePOS)
									 ? (f->fileSize - (uint32_t)f->filePOS) : 0;
			int maxBlocksByFile = (int)(bytesLeftInFile / f->blockSize);
			int maxBlocksByWant = (count - total) / (int)f->blockSize;
			int blocksToRead = (maxBlocksByFile < maxBlocksByWant) 
							 ? maxBlocksByFile : maxBlocksByWant; 

			if(blocksToRead <= 0) break;

			uint32_t baseLBIdx = (uint32_t)(f->filePOS / f->blockSize);
			int actualReadBlocks = 0;

			// Walk the FAT chain one block at a time
			for(int i = 0; i < blocksToRead; i++)
			{
				uint32_t lba = getBlockOfFile(f->startBlock, baseLBIdx + (uint32_t)i);
				if(lba == FAT_EOF || lba == FAT_RESERVED) break;

				if(LBAread(buffer + total + i * (int)f->blockSize, 1, lba) != 1) break;

				actualReadBlocks++;
			}

			if(actualReadBlocks <= 0) break; // Couldn't deliver more

			int bytes = actualReadBlocks * (int)f->blockSize;
			f->filePOS += bytes;
			total += bytes;
			f->bufBlockIdx = -1; // Force a reload when we fall back to buffered reads

			if((uint32_t)f->filePOS >= f->fileSize || total >= count) break;

			// After the fast path, loop back; Part 1 will pick up the tail if any
			continue;
		}

		// Part 3: Tail that's smaller than a block;
		// Load (or reload) the exact block that contains the current filePOS
		// and let the next iteration's part 1 copy the remaining tail
		if(loadBlock(f, (uint32_t)f->filePOS / f->blockSize) < 0) break;

		// Loop continues
		// Part 1 will copy the leftover < blocksize
	}
		
	return total;
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd) {
	// handle a bad input
	if (fd < 0 || fd >= MAXFCBS) {
		fprintf(stderr,
				"(b_close) File descriptor must be between 0 and %d.\n",
				MAXFCBS);
		return -1;
	}

	// get pointer to our fcb
	b_fcb* f = &fcbArray[fd];

	// if entry is already closed, report an issue
	if (!f->inUse) {
		fprintf(stderr, "(b_close) Attempted to close an already closed file.\n");
		return -1;
	}

	// free the data buffer
	free(f->buf);
	f->buf = NULL;

	// free the parent directory
	free(f->parentDirectory);
	f->parentDirectory = NULL;

	// mark as unused
	f->inUse = 0;

	return 0;
}
