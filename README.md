# Custom File System

## Overview
This project is a custom file system implemented in **C**, designed to simulate core file system functionality on top of a virtual block device. The system supports persistent storage, directory management, and basic file operations using a low-level logical block addressing (LBA) interface.

The file system operates on a fixed-size volume and provides a shell-based interface for interacting with files and directories, similar to a simplified UNIX-style file system.

---

## Features
- Volume formatting and initialization
- Persistent on-disk storage
- Free space management (allocation and release)
- Directory creation, removal, and traversal
- File creation, deletion, read, write, and seek operations
- Root directory initialization
- File metadata tracking (size, timestamps, blocks)
- CLI-based shell for interacting with the file system

---

## File System Design
The system is built around several core components:
- **Volume Control Block (VCB)** for storing global file system metadata
- **Free space management** using a custom allocation strategy
- **Directory structures** to track files and subdirectories
- **File descriptors** to manage open files and current offsets

Low-level disk I/O is performed using provided LBA-based read and write functions, allowing the file system to manage its own layout and persistence.

---

## Supported Operations

### File Operations
- Open (`b_open`)
- Read (`b_read`)
- Write (`b_write`)
- Seek (`b_seek`)
- Close (`b_close`)
- Delete files

### Directory Operations
- Create and remove directories
- Open and read directory contents
- Change and retrieve the current working directory
- Determine whether a path refers to a file or directory

---

## Shell Interface
The file system includes an interactive shell used to test and demonstrate functionality. Supported commands include:

- `ls` – list directory contents  
- `cd` – change directory  
- `pwd` – print working directory  
- `md` – create a directory  
- `rm` – remove a file or directory  
- `touch` – create a file  
- `cat` – display file contents (limited)  
- `cp` / `mv` – copy or move files  
- `cp2fs` – copy from Linux filesystem into the custom file system  
- `cp2l` – copy from the custom file system to Linux  
- `help`, `history`

Some commands may have limitations depending on file size or usage patterns.

---

## Build & Run

- You must use `make` to compile the program.
- You must use `make run` (sometimes with RUNOPTIONS) to execute the program
