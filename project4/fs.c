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

#define MAX_FILE_SIZE ((POINTERS_PER_INODE + POINTERS_PER_BLOCK) * DISK_BLOCK_SIZE)

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

/* Helper Functions */

// Load inumber from disk into inode
void inode_load(int inumber, struct fs_inode *inode) {
    // Calculate disk block index and inode index
    int disk_index = (inumber / INODES_PER_BLOCK) + 1;
    int inode_index = inumber % INODES_PER_BLOCK;

    // Read disk block and extract inode
    union fs_block block;
    disk_read(disk_index, block.data);
    *inode = block.inode[inode_index];
}

// Save inumber and inode data to disk
void inode_save(int inumber, struct fs_inode *inode) {
    // Calculate disk block index and inode index
    int disk_index = (inumber / INODES_PER_BLOCK) + 1;
    int inode_index = inumber % INODES_PER_BLOCK;

    // Read disk block and extract inode
    union fs_block block;
    disk_read(disk_index, block.data);
    block.inode[inode_index] = *inode;

    // Write block back to disk
    disk_write(disk_index, block.data);
}

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

    // Write the new superblock
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

    // Set superblock as used and all data blocks as unused
    freemap[0] = 1;
    for (int i = 1; i < superblock.super.nblocks; i++) {
        freemap[i] = 0;
    }

    // Scan through all inodes
    for (int i = 1; i <= superblock.super.ninodeblocks; i++) {
        union fs_block block;
        disk_read(i, block.data);
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            // Continue if inode is not valid
            if (!block.inode[j].isvalid) {
                continue;
            }
            // Iterate through all direct pointers
            struct fs_inode *inode = &block.inode[j];
            for (int k = 0; k < POINTERS_PER_INODE; k++) {
                int b = inode->direct[j];
                if (b != 0) {
                    freemap[b] = 1;
                }
            }
            // Iterate through indirect block if set
            int id = inode->indirect;
            if (id == 0) {
                continue;
            }
            freemap[id] = 1;
            union fs_block indirect_block;
            disk_read(id, indirect_block.data);
            // Iterate through all blocks pointed to by the indirect block
            for (int k = 0; k < POINTERS_PER_BLOCK; k++) {
                int b = indirect_block.pointers[k];
                if (b != 0) {
                    freemap[b] = 1;
                }
            }
        }
    }

    return 1;
}

int fs_unmount() {
    if (freemap != NULL) {
        free(freemap);
        freemap = NULL;
    }
    return 1;
}

int fs_create() {
    // Iterate through inode blocks and find first free inode
    for (int i = 1; i < superblock.super.ninodeblocks; i++) {
        union fs_block block;
        disk_read(i, block.data);
        // Iterate through all inodes in the data block
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            if (block.inode[j].isvalid == 0) {
                block.inode[j].size = 0;
                disk_write(i, block.data);
                freemap[i] = 1;
                return j;
            }
        }
    }
    return -1;
}

int fs_delete(int inumber) {
    struct fs_inode inode;
    inode_load(inumber, &inode);

    int size = inode.size;

    // Iterate through direct pointers
    for (int i = 0; i < POINTERS_PER_INODE && size > 0; i++) {
        int b = inode.direct[i];
        if (b != 0) {
            freemap[b] = 0;
        }
        size -= DISK_BLOCK_SIZE;
    }

    // Read indirect block from disk
    union fs_block indirect;
    disk_read(inode.indirect, &indirect);
    freemap[inode.indirect] = 0;

    // Iterate through indirect pointers
    for (int i = 0; i < POINTERS_PER_BLOCK && size > 0; i++) {
        int b = indirect.pointers[i];
        if (b != 0) {
            freemap[b] = 0;
        }
        size -= DISK_BLOCK_SIZE;
    }

    // All data should have been cleared
    if (size > 0) {
        return 0;
    }

    // Clear inode block and write to disk
    memset(&inode, 0, sizeof(struct fs_inode));
    inode_save(inumber, &inode);

    return 1;
}

int fs_getsize(int inumber) {
    struct fs_inode inode;
    inode_load(inumber, &inode);
    return inode.isvalid ? inode.size : -1;
}

int fs_read(int inumber, char *data, int length, int offset) {
    // Check if inumber and offset are within bounds
    if (inumber >= superblock.inode || offset >= MAX_FILE_SIZE || length <= 0) {
        return 0;
    }

    struct fs_inode inode;
    inode_load(inumber, &inode);

    // Check if offset in bounds to reduce unnecessary reads
    if (offset >= inode.size) {
        return 0;
    }

    // Adjust length if not within bounds of the file
    if (offset + length > inode.size) {
        length = inode.size - offset;
    }

    int index = offset / DISK_BLOCK_SIZE;
    int *data_ptr = inode.direct;    // Pointer to array of disk blocks
    int direct_index = 1;            // Indirect index or direct index
    int nbytes = 0;                  // nbytes read

    // Read data from disk until length is reached
    while (nbytes != length) {
        union fs_block block;
        // Check if data should be read from indirect pointers
        if (index >= POINTERS_PER_INODE && direct_index) {
            disk_read(inode.indirect, block.data);
            index -= POINTERS_PER_INODE;
            data_ptr = block.data;
            direct_index = 0;    // Index is now for indirect blocks
        }
        // Read data block and copy into data buffer
        union fs_block data_block;
        disk_read(data_ptr[index], data_block.data);
        int size = length - nbytes;
        if (size >= DISK_BLOCK_SIZE) {
            size = DISK_BLOCK_SIZE;
        }
        memcpy(data[nbytes], data_block.data, size);
        nbytes += size;
        index++;
    }

    return nbytes;
}

int fs_write(int inumber, const char *data, int length, int offset) {
    return 0;
}
