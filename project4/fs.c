#include "fs.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "disk.h"

#define FS_MAGIC 0xf0f03410
#define INODES_PER_BLOCK 128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

// Returns the number of dedicated inode blocks given the disk size in blocks
#define NUM_INODE_BLOCKS(disk_size_in_blocks) (1 + (disk_size_in_blocks / 10))

struct fs_superblock {
    int magic;           // Magic bytes
    int nblocks;         // Size of the disk in number of blocks
    int ninodeblocks;    // Number of blocks dedicated to inodes
    int ninodes;         // Number of dedicated inodes
};

struct fs_inode {
    int isvalid;                       // 1 if valid (in use), 0 otherwise
    int size;                          // Size of file in bytes
    int direct[POINTERS_PER_INODE];    // Direct data block numbers (0 if invalid)
    int indirect;                      // Indirect data block number (0 if invalid)
};

union fs_block {
    struct fs_superblock super;                 // Superblock
    struct fs_inode inode[INODES_PER_BLOCK];    // Block of inodes
    int pointers[POINTERS_PER_BLOCK];           // Indirect block of direct data block numbers
    char data[DISK_BLOCK_SIZE];                 // Data block
};

static union fs_block superblock;

/**
 * + Freemap should be allocated in fs_mount (via malloc)
 *   based on the number of blocks (nblocks) in your super block
 *
 * + freemap can then be indexed like an array for determining whether or
 *   not a block is free... e.g.
 *      if(!freemap[123])
 *          printf("Block 123 is NOT free\n");
 */
static int *freemap = NULL;

void fs_debug() {
    union fs_block block;

    disk_read(0, block.data);

    printf("superblock:\n");
    printf("    %d blocks\n", block.super.nblocks);
    printf("    %d inode blocks\n", block.super.ninodeblocks);
    printf("    %d inodes\n", block.super.ninodes);
}

int fs_format() {
    const int nblocks = disk_size();
    const int ninodeblocks = NUM_INODE_BLOCKS(nblocks);
    const int ninodes = ninodeblocks * INODES_PER_BLOCK;

    // Empty and write the new superblock
    memset(&superblock, 0, sizeof(union fs_block));
    superblock.super.magic = FS_MAGIC;
    superblock.super.ninodeblocks = ninodeblocks;
    superblock.super.ninodes = ninodes;
    superblock.super.nblocks = nblocks;

    // Write superblock to block 0
    disk_write(0, superblock.data);

    // Empty the inodes
    union fs_block empty_block;
    memset(&empty_block, 0, sizeof(union fs_block));
    for (int i = 1; i <= ninodeblocks; i++) {
        disk_write(i, empty_block.data);
    }

    return 1;
}

int fs_mount() {
    // Read data from superblock
    disk_read(0, superblock.data);

    // Check if magic number is valid
    if (superblock.super.magic != FS_MAGIC) {
        return 0;
    }
    // Check if there is alread a free map
    if (freemap != NULL) {
        free(freemap);
    }
    // Allocate a new free map
    const int nblocks = superblock.super.nblocks;
    freemap = malloc(sizeof(int) * nblocks);
    if (freemap == NULL) {
        return 0;
    }
    // Set all blocks as free
    for (int i = 0; i < nblocks; i++) {
        freemap[i] = 0;
    }
    // Set superblock as used
    freemap[0] = 1;
    return 1;
}

int fs_unmount() {
    if (freemap != NULL) {
        free(freemap);
    }
    return 1;
}

int fs_create() {
    // Iterate through freemap and find first free inode
    for (int i = 1; i < superblock.super.nblocks; i++) {
        if (freemap[i] != 0) {
            // Get disk and inode block index
            const int disk_idx = (i / INODES_PER_BLOCK) + 1;
            const int inode_idx = i % INODES_PER_BLOCK;
            // Read inode block from disk 
            union fs_block block;
            disk_read(disk_idx, block.data);
            // Update inode
            block.inode[inode_idx].isvalid = 1;
            block.inode[inode_idx].size = 0;
            // Write inode block back to disk
            disk_write(disk_idx, block.data);
            // Update freemap
            freemap[i] = 1;
            return i;
        }
    }
    return -1;
}

int fs_delete(int inumber) {
    return 0;
}

int fs_getsize(int inumber) {
    return -1;
}

int fs_read(int inumber, char *data, int length, int offset) {
    return 0;
}

int fs_write(int inumber, const char *data, int length, int offset) {
    return 0;
}
