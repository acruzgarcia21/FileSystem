/**************************************************************
* Class::  CSC-415-02 Fall 2025
* Name:: Alejandro Cruz-Garcia, Ronin Lombardino, Evan Caplinger, Alex Tamayo
* Student IDs:: 923799497, 924363164, 924990024, 921199718
* GitHub-Name:: RookAteMySSD
* Group-Name:: Team #1 Victory Royal
* Project:: Basic File System
*
* File:: fsshell.c
*
* Description:: Main driver for file system assignment.
*
* Make sure to set the #defined on the CMDxxxx_ON from 0 to 1 
* when you are ready to test that feature
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <getopt.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"

#define PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define SINGLE_QUOTE	0x27
#define DOUBLE_QUOTE	0x22
#define BUFFERLEN		200
#define DIRMAX_LEN		4096

/****   SET THESE TO 1 WHEN READY TO TEST THAT COMMAND ****/
#define CMDLS_ON	1
#define CMDCP_ON	1
#define CMDMV_ON	1
#define CMDMD_ON	1
#define CMDRM_ON	1
#define CMDCP2L_ON	1
#define CMDCP2FS_ON	1
#define CMDCD_ON	1
#define CMDPWD_ON	1
#define CMDTOUCH_ON	1
#define CMDCAT_ON	1


typedef struct dispatch_t
	{
	char * command;
	int (*func)(int, char**);
	char * description;
	} dispatch_t, * dispatch_p;


int cmd_ls (int argcnt, char *argvec[]);
int cmd_cp (int argcnt, char *argvec[]);
int cmd_mv (int argcnt, char *argvec[]);
int cmd_md (int argcnt, char *argvec[]);
int cmd_rm (int argcnt, char *argvec[]);
int cmd_touch (int argcnt, char *argvec[]);
int cmd_cat (int argcnt, char *argvec[]);
int cmd_cp2l (int argcnt, char *argvec[]);
int cmd_cp2fs (int argcnt, char *argvec[]);
int cmd_cd (int argcnt, char *argvec[]);
int cmd_pwd (int argcnt, char *argvec[]);
int cmd_history (int argcnt, char *argvec[]);
int cmd_help (int argcnt, char *argvec[]);

dispatch_t dispatchTable[] = {
	{"ls", cmd_ls, "Lists the file in a directory"},
	{"cp", cmd_cp, "Copies a file - source [dest]"},
	{"mv", cmd_mv, "Moves a file - source dest"},
	{"md", cmd_md, "Make a new directory"},
	{"rm", cmd_rm, "Removes a file or directory"},
        {"touch",cmd_touch, "Touches/Creates a file"},
        {"cat", cmd_cat, "Limited version of cat that displace the file to the console"},
	{"cp2l", cmd_cp2l, "Copies a file from the test file system to the linux file system"},
	{"cp2fs", cmd_cp2fs, "Copies a file from the Linux file system to the test file system"},
	{"cd", cmd_cd, "Changes directory"},
	{"pwd", cmd_pwd, "Prints the working directory"},
	{"history", cmd_history, "Prints out the history"},
	{"help", cmd_help, "Prints out help"}
};

static int dispatchcount = sizeof (dispatchTable) / sizeof (dispatch_t);

// Display files for use by ls command
int displayFiles (fdDir * dirp, int flall, int fllong)
	{
#if (CMDLS_ON == 1)				
	if (dirp == NULL)	//get out if error
		return (-1);
	
	struct fs_diriteminfo * di;
	struct fs_stat statbuf;
	
	di = fs_readdir (dirp);
	printf("\n");
	while (di != NULL) 
		{
		if ((di->d_name[0] != '.') || (flall)) //if not all and starts with '.' it is hidden
			{
			if (fllong)
				{
				fs_stat (di->d_name, &statbuf);
				printf ("%s    %9ld   %s\n", fs_isDir(di->d_name)?"D":"-", statbuf.st_size, di->d_name);
				}
			else
				{
				printf ("%s\n", di->d_name);
				}
			}
		di = fs_readdir (dirp);
		}
	fs_closedir (dirp);
#endif
	return 0;
	}
	

/****************************************************
*  ls commmand
****************************************************/
int cmd_ls (int argcnt, char *argvec[])
	{
#if (CMDLS_ON == 1)				
	int option_index;
	int c;
	int fllong;
	int flall;
	char cwd[DIRMAX_LEN];
		
	static struct option long_options[] = 
		{
		/* These options set their assigned flags to value and return 0 */
		/* These options don't set flags and return the value */	 
		{"long",	no_argument, 0, 'l'},  
		{"all",		no_argument, 0, 'a'},
		{"help",	no_argument, 0, 'h'},
		{0,			0,       0,  0 }
		};
		
	option_index = 0;
#ifdef __GNU_LIBRARY__
    // WORKAROUND
    // Setting "optind" to 0 triggers initialization of getopt private
    // structure (holds pointers on data between calls). This helps
    // to avoid possible memory violation, because data passed to getopt_long()
    // could be freed between parse() calls.
    optind = 0;
#else
    // "optind" is used between getopt() calls to get next argument for parsing and should be
    // initialized before each parsing loop.
    optind = 1;
#endif
	fllong = 0;
	flall = 0;

	while (1)
		{	
		c = getopt_long(argcnt, argvec, "alh",
				long_options, &option_index);
				
		if (c == -1)
		   break;

		switch (c) {
			case 0:			//flag was set, ignore
			   printf("Unknown option %s", long_options[option_index].name);
			   if (optarg)
				   printf(" with arg %s", optarg);
			   printf("\n");
				break;
				
			case 'a':
				flall = 1;
				break;
				
			case 'l':
				fllong = 1;
				break;
				
			case 'h':
			default:
				printf ("Usage: ls [--all-a] [--long/-l] [pathname]\n");
				return (-1);
				break;
			}
		}
	
	
	if (optind < argcnt)
		{
		//processing arguments after options
		for (int k = optind; k < argcnt; k++)
			{
			if (fs_isDir(argvec[k]))
				{
				fdDir * dirp;
				dirp = fs_opendir (argvec[k]);
				displayFiles (dirp, flall, fllong);
				}
			else // it is just a file ?
				{
				if (fs_isFile (argvec[k]))
					{
					//no support for long format here
					printf ("%s\n", argvec[k]);
					}
				else
					{
					printf ("%s is not found\n", argvec[k]);
					}
				}
			}		
		}
	else   // no pathname/filename specified - use cwd
		{
		char * path = fs_getcwd(cwd, DIRMAX_LEN);	//get current working directory
		fdDir * dirp;
		dirp = fs_opendir (path);
		return (displayFiles (dirp, flall, fllong));
		}
#endif
	return 0;
	}


	
/****************************************************
*  Touch file commmand
****************************************************/

int cmd_touch (int argcnt, char *argvec[])
        {
#if (CMDTOUCH_ON == 1)     
        int testfs_src_fd;
        char * src;

        switch (argcnt)
                {
                case 2: //only one name provided
                        src = argvec[1];
                        break;

                default:
                        printf("Usage: touch srcfile\n");
                        return (-1);
                }


        testfs_src_fd = b_open (src, O_WRONLY | O_CREAT);
        if (testfs_src_fd < 0)
	    return (testfs_src_fd);	//return with error

        b_close (testfs_src_fd);
#endif
        return 0;
        }


/***************************************************
* Cat Command
***************************************************/

int cmd_cat (int argcnt, char *argvec[])
        {
#if (CMDCAT_ON == 1)     
        int testfs_src_fd;
        char * src;
        int readcnt;
        char buf[BUFFERLEN];

        switch (argcnt)
                {
                case 2: //only one name provided
                        src = argvec[1];
                        break;

                default:
                        printf("Usage: cat srcfile\n");
                        return (-1);
                }


        testfs_src_fd = b_open (src, O_RDONLY);

        if (testfs_src_fd < 0)
            {
	    printf ("Failed to open file system file: %s\n", src);
            return (testfs_src_fd);
            }


        do 
                {
                readcnt = b_read (testfs_src_fd, buf, BUFFERLEN-1);
                buf[readcnt] = '\0';
                printf("%s", buf);
                } while (readcnt == BUFFERLEN-1);
        b_close (testfs_src_fd);
#endif
        return 0;
        }




/****************************************************
*  Copy file commmand
****************************************************/
	
int cmd_cp (int argcnt, char *argvec[])
	{
#if (CMDCP_ON == 1)	
	int testfs_src_fd;
	int testfs_dest_fd;
	char * src;
	char * dest;
	int readcnt;
	char buf[BUFFERLEN];
	
	switch (argcnt)
		{
		case 2:	//only one name provided
			src = argvec[1];
			dest = src;
			break;
			
		case 3:
			src = argvec[1];
			dest = argvec[2];
			break;
		
		default:
			printf("Usage: cp srcfile [destfile]\n");
			return (-1);
		}
	
	
	testfs_src_fd = b_open (src, O_RDONLY);
	testfs_dest_fd = b_open (dest, O_WRONLY | O_CREAT | O_TRUNC);
	do 
		{
		readcnt = b_read (testfs_src_fd, buf, BUFFERLEN);
		b_write (testfs_dest_fd, buf, readcnt);
		} while (readcnt == BUFFERLEN);
	b_close (testfs_src_fd);
	b_close (testfs_dest_fd);
#endif
	return 0;
	}
	
/****************************************************
*  Move file commmand & helper functions
****************************************************/

/**
 * extract the final component of a path
 */
static void extractFileName(const char* path, char* outName) {
    if (!path || !outName) return;
    
    // Find the last '/' in the path
    const char* lastSlash = strrchr(path, '/');
    if (!lastSlash) {
        // No slash found - path is just a filename
        strcpy(outName, path);
    } else {
        // Copy everything after the last slash
        strcpy(outName, lastSlash + 1);
    }
}

/**
 * read the size of a directory from its "." entry
 */
static uint32_t getDirectorySize(uint32_t dirLoc) {
    vcb* pVCB = getGlobalVCB();
    
    // Allocate space for one block
    DE* firstBlock = malloc(pVCB->blockSize);
    if (!firstBlock) return 0;
    
    // Read the first block which contains the "." entry
    if (LBAread(firstBlock, 1, dirLoc) != 1) {
        free(firstBlock);
        return 0;
    }
    
    // The "." entry is always at index 0
    uint32_t size = firstBlock[0].size;
    free(firstBlock);
    return size;
}

/**
 * remove an entry from parent WITHOUT freeing its blocks
 */
static int detachEntry(DE* parent, const char* name, DE* outEntry) {
    if (!parent || !name) return -1;
    
    vcb* pVCB = getGlobalVCB();
    int entryCount = parent[0].size / sizeof(DE);
    
    // find the entry by name
    int foundIndex = -1;
    for (int i = 0; i < entryCount; i++) {
        if ((parent[i].flags & DE_IS_USED) && 
            strcmp(parent[i].name, name) == 0) {
            foundIndex = i;
            break;
        }
    }
    
    if (foundIndex == -1) return -1;  // Entry not found
    
    // save a copy of the entry if requested (for later attachment)
    if (outEntry) {
        memcpy(outEntry, &parent[foundIndex], sizeof(DE));
    }
    
    // mark the entry as unused but don't free its blocks
    parent[foundIndex].flags = 0;
    parent[foundIndex].name[0] = '\0';
    parent[0].modified = getCurrentTime();
    
    // write the updated parent directory back to disk
    uint32_t parentLoc = parent[0].location;
    uint32_t parentSize = parent[0].size;
    int blocksWritten = writeBlocksToDisk((char*)parent, parentLoc,
                                          (parentSize + pVCB->blockSize - 1) / pVCB->blockSize);
    
    return (blocksWritten <= 0) ? -1 : 0;
}

/**
 * add an entry to a parent directory
 */
static int attachEntry(DE* parent, const char* name, DE* entry) {
    if (!parent || !name || !entry) return -1;
    
    vcb* pVCB = getGlobalVCB();
    int entryCount = parent[0].size / sizeof(DE);
    
    // find an empty slot in the directory
    int emptySlot = -1;
    for (int i = 0; i < entryCount; i++) {
        if (!(parent[i].flags & DE_IS_USED) || parent[i].name[0] == '\0') {
            emptySlot = i;
            break;
        }
    }
    
    if (emptySlot == -1) return -1;  // Directory is full
    
    // copy the entry into the empty slot
    memcpy(&parent[emptySlot], entry, sizeof(DE));
    
    // set the name (allows renaming during move)
    strcpy(parent[emptySlot].name, name);
    
    // Mark as used
    parent[emptySlot].flags |= DE_IS_USED;
    parent[0].modified = getCurrentTime();
    
    // write the updated parent directory back to disk
    uint32_t parentLoc = parent[0].location;
    uint32_t parentSize = parent[0].size;
    int blocksWritten = writeBlocksToDisk((char*)parent, parentLoc,
                                          (parentSize + pVCB->blockSize - 1) / pVCB->blockSize);
    
    return (blocksWritten <= 0) ? -1 : 0;
}

/**
 * rename an entry within the same directory
 */
static int renameInPlace(DE* parent, const char* oldName, const char* newName) {
    if (!parent || !oldName || !newName) return -1;
    
    vcb* pVCB = getGlobalVCB();
    int entryCount = parent[0].size / sizeof(DE);
    
    // find the entry by old name
    int foundIndex = -1;
    for (int i = 0; i < entryCount; i++) {
        if ((parent[i].flags & DE_IS_USED) && 
            strcmp(parent[i].name, oldName) == 0) {
            foundIndex = i;
            break;
        }
    }
    
    if (foundIndex == -1) return -1;  // Entry not found
    
    // simply change the name - everything else stays the same
    strcpy(parent[foundIndex].name, newName);
    parent[0].modified = getCurrentTime();
    
    // write the updated directory back to disk
    uint32_t parentLoc = parent[0].location;
    uint32_t parentSize = parent[0].size;
    int blocksWritten = writeBlocksToDisk((char*)parent, parentLoc,
                                          (parentSize + pVCB->blockSize - 1) / pVCB->blockSize);
    
    return (blocksWritten <= 0) ? -1 : 0;
}

/**
 * update the ".." entry in a moved directory
 */
static int updateDotDot(uint32_t dirLoc, uint32_t dirSize, 
                        uint32_t newParentLoc, uint32_t newParentSize) {
    vcb* pVCB = getGlobalVCB();
    
    // load the entire directory that was moved
    DE* dir = loadDirectory(dirLoc, dirSize, pVCB->blockSize);
    if (!dir) return -1;
    
    int entryCount = dirSize / sizeof(DE);
    int dotdotIndex = -1;
    
    // find the ".." entry (should be at index 1, but search to be safe)
    for (int i = 0; i < entryCount; i++) {
        if ((dir[i].flags & DE_IS_USED) && strcmp(dir[i].name, "..") == 0) {
            dotdotIndex = i;
            break;
        }
    }
    
    if (dotdotIndex == -1) {
        free(dir);
        return -1;  // every directory should have ".."
    }
    
    // update ".." to point to the new parent
    dir[dotdotIndex].location = newParentLoc;
    dir[dotdotIndex].size = newParentSize;
    
    // write the updated directory back to disk
    int blocksWritten = writeBlocksToDisk((char*)dir, dirLoc,
                                          (dirSize + pVCB->blockSize - 1) / pVCB->blockSize);
    free(dir);
    
    return (blocksWritten <= 0) ? -1 : 0;
}

/**
 * check if one directory is an ancestor of another
 */
static int isAncestorOf(uint32_t potentialAncestorLoc, uint32_t targetLoc) {
    vcb* pVCB = getGlobalVCB();
    
    // check if they're the same, it's not an ancestor relationship
    if (potentialAncestorLoc == targetLoc) return 0;
    
    uint32_t currentLoc = targetLoc;
    uint32_t rootLoc = pVCB->rootStart;
    int maxIterations = 100;  // prevent infinite loops
    int iterations = 0;
    
    // move up the directory tree using ".." entries
    while (currentLoc != rootLoc && iterations < maxIterations) {
        iterations++;
        
        // get the size of the current directory
        uint32_t currentSize = getDirectorySize(currentLoc);
        if (currentSize == 0) return -1;  // error reading directory
        
        // load the current directory
        DE* currentDir = loadDirectory(currentLoc, currentSize, pVCB->blockSize);
        if (!currentDir) return -1;
        
        int entryCount = currentSize / sizeof(DE);
        int dotdotIndex = -1;
        
        // find the ".." entry to get parent location
        for (int i = 0; i < entryCount; i++) {
            if ((currentDir[i].flags & DE_IS_USED) && 
                strcmp(currentDir[i].name, "..") == 0) {
                dotdotIndex = i;
                break;
            }
        }
        
        if (dotdotIndex == -1) {
            free(currentDir);
            return -1;  // every directory should have ".."
        }
        
        // get parent location from ".."
        uint32_t parentLoc = currentDir[dotdotIndex].location;
        free(currentDir);
        
        // check if this parent is the potential ancestor
        if (parentLoc == potentialAncestorLoc) return 1;  // Found it!
        
        // move up to parent and continue
        currentLoc = parentLoc;
    }
    
    // reached root without finding the ancestor
    return 0;
}

/**
 * remove an entry and free its blocks
 */
static int deleteEntry(DE* parent, const char* name) {
    if (!parent || !name) return -1;
    
    vcb* pVCB = getGlobalVCB();
    int entryCount = parent[0].size / sizeof(DE);
    
    // find the entry to delete
    int foundIndex = -1;
    for (int i = 0; i < entryCount; i++) {
        if ((parent[i].flags & DE_IS_USED) && 
            strcmp(parent[i].name, name) == 0) {
            foundIndex = i;
            break;
        }
    }
    
    if (foundIndex == -1) return -1;  // entry not found
    
    // save the block location before clearing the entry
    uint32_t locationToFree = parent[foundIndex].location;
    
    // mark entry as unused
    parent[foundIndex].flags = 0;
    parent[foundIndex].name[0] = '\0';
    parent[0].modified = getCurrentTime();
    
    // write the updated parent directory back to disk
    uint32_t parentLoc = parent[0].location;
    uint32_t parentSize = parent[0].size;
    int blocksWritten = writeBlocksToDisk((char*)parent, parentLoc,
                                          (parentSize + pVCB->blockSize - 1) / pVCB->blockSize);
    
    if (blocksWritten <= 0) return -1;
    
    // free the blocks (but only if valid location)
    // empty files created with touch have location=0 and no blocks to free
    if (locationToFree != 0 && 
        locationToFree != FAT_EOF && 
        locationToFree != FAT_RESERVED) {
        freeBlocks(locationToFree);
    }
    
    return 0;
}

/****************************************************
*  Move file command Main Implementation
****************************************************/
int cmd_mv (int argcnt, char *argvec[])
{
#if (CMDMV_ON == 1)	
    // STEP 1: Validate Arguments 
    if (argcnt != 3) {
        printf("Usage: mv source destination\n");
        return -1;
    }

    char* source = argvec[1];
    char* dest = argvec[2];

    // check for NULL or empty strings
    if (!source || !dest || strlen(source) == 0 || strlen(dest) == 0) {
        printf("mv: invalid arguments\n");
        return -1;
    }

    // STEP 2: reject special directories
    // cannot move ".", "..", or root 
    char sourceFileName[DE_NAME_MAX];
    extractFileName(source, sourceFileName);
    
    if (strcmp(sourceFileName, ".") == 0 || 
        strcmp(sourceFileName, "..") == 0 ||
        strcmp(source, "/") == 0) {
        printf("mv: cannot move '%s'\n", source);
        return -1;
    }

    vcb* pVCB = getGlobalVCB();

    // STEP 3: Parse Source Path 
    ppinfo sourcePPI;
    if (parsePath(source, &sourcePPI) != 0 || sourcePPI.index == -1) {
        printf("mv: cannot stat '%s': No such file or directory\n", source);
        return -1;
    }

    // save source information
    DE sourceEntry;
    memcpy(&sourceEntry, &sourcePPI.parent[sourcePPI.index], sizeof(DE));
    DE* sourceParent = sourcePPI.parent;  // directory containing source
    char* sourceName = sourcePPI.lastElementName;  // name of source file/dir

    // STEP 4: parse destination path 
    ppinfo destPPI;
    int destResult = parsePath(dest, &destPPI);
    
    // destResult == -2 means parent directory doesn't exist
    if (destResult == -2) {
        free(sourceParent);
        free(sourceName);
        printf("mv: cannot move '%s' to '%s': No such file or directory\n", source, dest);
        return -1;
    }

    // determine if destination exists and if it's a directory
    int destExists = (destResult == 0 && destPPI.index != -1);
    int destIsDir = destExists && (destPPI.parent[destPPI.index].flags & DE_IS_DIR);
    
    DE* destParent = destPPI.parent;  // directory containing destination
    char* destName = destPPI.lastElementName;  // name of destination

    // STEP 5: check for self move
    if (destExists && !destIsDir &&
        sourceParent[0].location == destParent[0].location &&
        strcmp(sourceName, destName) == 0) {

        free(sourceParent);
        free(destParent);
        free(sourceName);
        free(destName);
        return 0;  // nothing to do
    }

    // STEP 6: handle move into directory
    if (destIsDir) {
        uint32_t finalDestLoc = destPPI.parent[destPPI.index].location;
        uint32_t finalDestSize = destPPI.parent[destPPI.index].size;

        // check for directory loops (can't move dir into its own subdirectory)
        if (sourceEntry.flags & DE_IS_DIR) {
            if (sourceEntry.location == finalDestLoc ||
                isAncestorOf(sourceEntry.location, finalDestLoc) == 1) {
                free(sourceParent);
                free(destParent);
                free(sourceName);
                free(destName);
                printf("mv: cannot move a directory into itself\n");
                return -1;
            }
        }

        // Load the destination directory
        DE* finalDestDir = loadDirectory(finalDestLoc, finalDestSize, pVCB->blockSize);
        if (!finalDestDir) {
            free(sourceParent);
            free(destParent);
            free(sourceName);
            free(destName);
            printf("mv: cannot access destination directory\n");
            return -1;
        }

        // Check if an entry with source's name already exists in destination
        int finalDestEntryCount = finalDestSize / sizeof(DE);
        DE* existingEntry = findEntryInDirectory(finalDestDir, finalDestEntryCount, sourceName);

        if (existingEntry) {
            if (existingEntry->flags & DE_IS_DIR) {
                // Can't overwrite a directory
                free(finalDestDir);
                free(sourceParent);
                free(destParent);
                free(sourceName);
                free(destName);
                printf("mv: cannot overwrite directory\n");
                return -1;
            } else {
                // Overwrite the existing file
                deleteEntry(finalDestDir, sourceName);
                free(finalDestDir);
                // Reload after deletion
                finalDestDir = loadDirectory(finalDestLoc, finalDestSize, pVCB->blockSize);
            }
        }

        // Perform the move: detach from source parent, attach to destination
        DE entryToMove;
        if (detachEntry(sourceParent, sourceName, &entryToMove) != 0) {
            free(finalDestDir);
            free(sourceParent);
            free(destParent);
            free(sourceName);
            free(destName);
            printf("mv: failed to detach source\n");
            return -1;
        }

        if (attachEntry(finalDestDir, sourceName, &entryToMove) != 0) {
            free(finalDestDir);
            free(sourceParent);
            free(destParent);
            free(sourceName);
            free(destName);
            printf("mv: failed to attach to destination\n");
            return -1;
        }

        // If we moved a directory, update its ".." entry to point to new parent
        if (entryToMove.flags & DE_IS_DIR) {
            updateDotDot(entryToMove.location, entryToMove.size,
                       finalDestLoc, finalDestSize);
        }

        // Cleanup and success
        free(finalDestDir);
        free(sourceParent);
        free(destParent);
        free(sourceName);
        free(destName);
        return 0;
    }

    // STEP 7: Determine if Same or Different Directory 
    // this affects how we handle the move
    int sameDirectory = (sourceParent[0].location == destParent[0].location);

    // STEP 8: Handle Overwriting Existing File 
    if (destExists && !destIsDir) {
        // IMPORTANT: Save parent info BEFORE any operations
        // we'll need this for reloading after deletion
        uint32_t destParentLoc = destParent[0].location;
        uint32_t destParentSize = destParent[0].size;
    
        // delete the existing destination file
        if (deleteEntry(destParent, destName) != 0) {
            free(sourceParent);
            free(destParent);
            free(sourceName);
            free(destName);
            printf("mv: failed to overwrite destination\n");
            return -1;
        }

        // reload destination parent (it was modified by deleteEntry)
        free(destParent);
        destParent = loadDirectory(destParentLoc, destParentSize, pVCB->blockSize);

        if (!destParent) {
            free(sourceParent);
            free(sourceName);
            free(destName);
            printf("mv: failed to reload destination directory\n");
            return -1;
        }
    
        // if same directory, also reload source parent!
        // both sourceParent and destParent pointed to the same directory in memory.
        // after freeing and reloading destParent, sourceParent becomes a stale pointer.
        // without this, we'd get duplicate entries.
        if (sameDirectory) {
            free(sourceParent);
            sourceParent = loadDirectory(destParentLoc, destParentSize, pVCB->blockSize);

            if (!sourceParent) {
                free(destParent);
                free(sourceName);
                free(destName);
                printf("mv: failed to reload source directory\n");
                return -1;
            }
        }
    }

    // STEP 9: check directory loops
    // prevent moving a directory into its own subdirectory
    if (sourceEntry.flags & DE_IS_DIR) {
        if (isAncestorOf(sourceEntry.location, destParent[0].location) == 1) {
            free(sourceParent);
            free(destParent);
            free(sourceName);
            free(destName);
            printf("mv: cannot move directory into subdirectory of itself\n");
            return -1;
        }
    }

    // STEP 10: move
    
    if (sameDirectory) {
        // rename in same directory
        // ex: mv /dir/old /dir/new
        // change the name, no need to move entry slots
        if (renameInPlace(sourceParent, sourceName, destName) != 0) {
            free(sourceParent);
            free(destParent);
            free(sourceName);
            free(destName);
            printf("mv: rename failed\n");
            return -1;
        }
    } else {
        // move to different directory
        // ex: mv /dir1/file /dir2/newname
        // detach from source parent, attach to destination parent
        DE entryToMove;
        if (detachEntry(sourceParent, sourceName, &entryToMove) != 0 ||
            attachEntry(destParent, destName, &entryToMove) != 0) {
            free(sourceParent);
            free(destParent);
            free(sourceName);
            free(destName);
            printf("mv: move failed\n");
            return -1;
        }

        // if we moved a directory, update its ".." entry
        if (entryToMove.flags & DE_IS_DIR) {
            updateDotDot(entryToMove.location, entryToMove.size,
                       destParent[0].location, destParent[0].size);
        }
    }
    
    // STEP 11: cleanup
    free(sourceParent);
    free(destParent);
    free(sourceName);
    free(destName);
#endif
    return 0;
}


/****************************************************
*  Make Directory commmand
****************************************************/
// Make Directory	
int cmd_md (int argcnt, char *argvec[])
	{
#if (CMDMD_ON == 1)				
	if (argcnt != 2)
		{
		printf("Usage: md pathname\n");
		return -1;
		}
	else
		{
		return(fs_mkdir(argvec[1], 0777));
		}
#endif
	return -1;
	}
	
/****************************************************
*  Remove directory or file commmand
****************************************************/
int cmd_rm (int argcnt, char *argvec[])
	{
#if (CMDRM_ON == 1)
	if (argcnt != 2)
		{
		printf ("Usage: rm path\n");
		return -1;
		}
		
	char * path = argvec[1];	
	
	//must determine if file or directory
	if (fs_isDir (path))
		{
		return (fs_rmdir (path));
		}		
	if (fs_isFile (path))
		{
		return (fs_delete(path));
		}	
		
	printf("The path %s is neither a file not a directory\n", path);
#endif
	return -1;
	}
	
/****************************************************
*  Copy file from test file system to Linux commmand
****************************************************/
int cmd_cp2l (int argcnt, char *argvec[])
	{
#if (CMDCP2L_ON == 1)				
	int testfs_fd;
	int linux_fd;
	char * src;
	char * dest;
	int readcnt;
	char buf[BUFFERLEN];
	
	switch (argcnt)
		{
		case 2:	//only one name provided
			src = argvec[1];
			dest = src;
			break;
			
		case 3:
			src = argvec[1];
			dest = argvec[2];
			break;
		
		default:
			printf("Usage: cp2l srcfile [Linuxdestfile]\n");
			return (-1);
		}
	
	
	testfs_fd = b_open (src, O_RDONLY);
	linux_fd = open (dest, O_WRONLY | O_CREAT | O_TRUNC, PERMISSIONS);
	do 
		{
		readcnt = b_read (testfs_fd, buf, BUFFERLEN);
		write (linux_fd, buf, readcnt);
		} while (readcnt == BUFFERLEN);
	b_close (testfs_fd);
	close (linux_fd);
#endif
	return 0;
	}
	
/****************************************************
*  Copy file from Linux to test file system commmand
****************************************************/
int cmd_cp2fs (int argcnt, char *argvec[])
	{
#if (CMDCP2FS_ON == 1)				
	int testfs_fd;
	int linux_fd;
	char * src;
	char * dest;
	int readcnt;
	char buf[BUFFERLEN];
	
	switch (argcnt)
		{
		case 2:	//only one name provided
			src = argvec[1];
			dest = src;
			break;
			
		case 3:
			src = argvec[1];
			dest = argvec[2];
			break;
		
		default:
			printf("Usage: cp2fs Linuxsrcfile [destfile]\n");
			return (-1);
		}
	
	
	testfs_fd = b_open (dest, O_WRONLY | O_CREAT | O_TRUNC);
	linux_fd = open (src, O_RDONLY);
	do 
		{
		readcnt = read (linux_fd, buf, BUFFERLEN);
		b_write (testfs_fd, buf, readcnt);
		} while (readcnt == BUFFERLEN);
	b_close (testfs_fd);
	close (linux_fd);
#endif
	return 0;
	}
	
/****************************************************
*  cd commmand
****************************************************/
int cmd_cd (int argcnt, char *argvec[])
	{
#if (CMDCD_ON == 1)	
	if (argcnt != 2)
		{
		printf ("Usage: cd path\n");
		return (-1);
		}
	char * path = argvec[1];	//argument
	
	if (path[0] == '"')
		{
		if (path[strlen(path)-1] == '"')
			{
			//remove quotes from string
			path = path + 1;
			path[strlen(path) - 1] = 0;
			}
		}
	int ret = fs_setcwd (path);
	if (ret != 0)	//error
		{
		printf ("Could not change path to %s\n", path);
		return (ret);
		}			
#endif
	return 0;
	}
	
/****************************************************
*  PWD commmand
****************************************************/
int cmd_pwd (int argcnt, char *argvec[])
	{
#if (CMDPWD_ON == 1)
	char * dir_buf = malloc (DIRMAX_LEN +1);
	char * ptr;	
	ptr = fs_getcwd (dir_buf, DIRMAX_LEN);	
	if (ptr == NULL)			//an error occurred
		{
		printf ("An error occurred while trying to get the current working directory\n");
		}
	else
		{
		printf ("%s\n", ptr);
		}
	free (dir_buf);
	dir_buf = NULL;
	ptr = NULL;
	
#endif
	return 0;
	}

/****************************************************
*  History commmand
****************************************************/
int cmd_history (int argcnt, char *argvec[])
	{
	HIST_ENTRY * he;
	int i = 0;
	
	
	for (i = history_base; i <= history_length; i++)
		{
		he = history_get(i);
		
		if (he != NULL)
			{
			printf ("%s\n", he->line);
			}
		}
	return 0;
	}
	
/****************************************************
*  Help commmand
****************************************************/
int cmd_help (int argcnt, char *argvec[])
	{
	for (int i = 0; i < dispatchcount; i++)
		{
		printf ("%s\t%s\n", dispatchTable[i].command, dispatchTable[i].description);
		}
	return 0;
	} 

void processcommand (char * cmd)
	{
	int cmdLen;
	char ** cmdv;	//command vector
	int cmdc;		//command count
	int i, j;
	
	cmdLen = strlen(cmd);
	cmdv = (char **) malloc (sizeof(char *) * ((cmdLen/2)+2));
	cmdc = 0;
	
	cmdv[cmdc] = cmd;
	++cmdc;
	
	for (i = 0; i < cmdLen; i++)
		{
		switch (cmd[i])
			{
			case ' ':
				cmd[i] = 0;	//NULL terminate prior string
				while (cmd[i+1] == ' ')  // null at end will prevent from overshooting string
					{
					i++;
					}
				if ((i+1) < cmdLen)	//there is still more
					{
					cmdv[cmdc] = &cmd[i+1];
					++cmdc;
					}
				break;
				
			case '\\':
				++i;	//skip next character
				break;
				
			case DOUBLE_QUOTE:	//ignore everything till next quote (unless unterminated)
				for (j = i+1; j < cmdLen; j++)
					{
					if (cmd[j] == '\\')
						{
						++j;	//skip next character
						}	
					else if (cmd[j] == DOUBLE_QUOTE)
						{
						break;
						}
					}
				if (j >= cmdLen)
					{
					printf("Unterminated string\n");
					free(cmdv);
					cmdv = NULL;
					return;
					}
				i=j;
				break;
				
			case SINGLE_QUOTE:
				for (j = i+1; j < cmdLen; j++)
					{
					if (cmd[j] == '\\')
						{
						++j;	//skip next character
						}	
					else if (cmd[j] == SINGLE_QUOTE)
						{
						break;
						}
					}
				if (j >= cmdLen)
					{
					printf("Unterminated string\n");
					free(cmdv);
					cmdv = NULL;
					return;
					}
				i=j;
				break;
			default:
				break;
			}
		}
		
#ifdef COMMAND_DEBUG
	for (i = 0; i < cmdc; i++)
		{
		printf("%s: length %d\n", cmdv[i], strlen(cmdv[i]));
		}
#endif		
	cmdv[cmdc] = 0;		//just be safe - null terminate array of arguments
	
	for (i = 0; i < dispatchcount; i++)
		{
		if (strcmp(dispatchTable[i].command, cmdv[0]) == 0)
			{
			dispatchTable[i].func(cmdc,cmdv);
			free (cmdv);
			cmdv = NULL;
			return;
			}
		}
	printf("%s is not a regonized command.\n", cmdv[0]);
	cmd_help(cmdc, cmdv);	
	free (cmdv);
	cmdv = NULL;
	}



int main (int argc, char * argv[])
	{
	char * cmdin;
	char * cmd;
	HIST_ENTRY *he;
	char * filename;
	uint64_t volumeSize;
	uint64_t blockSize;
    int retVal;
    
	if (argc > 3)
		{
		filename = argv[1];
		volumeSize = atoll (argv[2]);
		blockSize = atoll (argv[3]);
		}
	else
		{
		printf ("Usage: fsLowDriver volumeFileName volumeSize blockSize\n");
		return -1;
		}
		
	retVal = startPartitionSystem (filename, &volumeSize, &blockSize);	
	printf("Opened %s, Volume Size: %llu;  BlockSize: %llu; Return %d\n", filename, (ull_t)volumeSize, (ull_t)blockSize, retVal);

	if (retVal != PART_NOERROR)
		{
		printf ("Start Partition Failed:  %d\n", retVal);
		return (retVal);
		}
		
	retVal = initFileSystem (volumeSize / blockSize, blockSize);
	
	if (retVal != 0)
		{
		printf ("Initialize File System Failed:  %d\n", retVal);
		closePartitionSystem();
		return (retVal);
		}

	if (argc > 4)
		if(strcmp("lowtest", argv[4]) == 0)
			runFSLowTest();


	using_history();
	stifle_history(200);	//max history entries
        printf ("|---------------------------------|\n");
        printf ("|------- Command ------|- Status -|\n");
#if (CMDLS_ON == 1)
        printf ("| ls                   |    ON    |\n");
#else
        printf ("| ls                   |    OFF   |\n");  
#endif
#if (CMDCD_ON == 1)
        printf ("| cd                   |    ON    |\n");  
#else
        printf ("| cd                   |    OFF   |\n");  
#endif
#if (CMDMD_ON == 1)
        printf ("| md                   |    ON    |\n");  
#else
        printf ("| md                   |    OFF   |\n");  
#endif
#if (CMDPWD_ON == 1)
        printf ("| pwd                  |    ON    |\n");  
#else
        printf ("| pwd                  |    OFF   |\n");  
#endif
#if (CMDTOUCH_ON == 1)
        printf ("| touch                |    ON    |\n");  
#else
        printf ("| touch                |    OFF   |\n");  
#endif
#if (CMDCAT_ON == 1)
        printf ("| cat                  |    ON    |\n");  
#else
        printf ("| cat                  |    OFF   |\n");  
#endif
#if (CMDRM_ON == 1)
        printf ("| rm                   |    ON    |\n");  
#else
        printf ("| rm                   |    OFF   |\n");  
#endif
#if (CMDCP_ON == 1)
        printf ("| cp                   |    ON    |\n");  
#else
        printf ("| cp                   |    OFF   |\n");  
#endif
#if (CMDMV_ON == 1)
        printf ("| mv                   |    ON    |\n");  
#else
        printf ("| mv                   |    OFF   |\n");  
#endif
#if (CMDCP2FS_ON == 1)
        printf ("| cp2fs                |    ON    |\n");  
#else
        printf ("| cp2fs                |    OFF   |\n");  
#endif
#if (CMDCP2L_ON == 1)
        printf ("| cp2l                 |    ON    |\n");  
#else
        printf ("| cp2l                 |    OFF   |\n");
#endif
        printf ("|---------------------------------|\n");

	
	while (1)
		{
		cmdin = readline("Prompt > ");
#ifdef COMMAND_DEBUG
		printf ("%s\n", cmdin);
#endif
		
		cmd = malloc (strlen(cmdin) + 30);
		strcpy (cmd, cmdin);
		free (cmdin);
		cmdin = NULL;
		
		if (strcmp (cmd, "exit") == 0)
			{
			free (cmd);
			cmd = NULL;
			exitFileSystem();
			closePartitionSystem();
			clear_history();
			rl_clear_history ();
			// exit while loop and terminate shell
			break;
			}
			
		if ((cmd != NULL) && (strlen(cmd) > 0))
			{
			he = history_get(history_length);
			if (!((he != NULL) && (strcmp(he->line, cmd)==0)))
				{
				add_history(cmd);
				}
			processcommand (cmd);
			}
				
		free (cmd);
		cmd = NULL;		
		} // end while
	}
