#include "ext2.h"

/* read_inode: Fills an inode data structure with the data from one
   inode in disk. Determines the block group number and index within
   the group from the inode number, then reads the inode from the
   inode table in the corresponding group. Saves the inode data in
   buffer 'buffer'.

   Parameters:
     volume: pointer to volume.
     inode_no: Number of the inode to read from disk.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns a positive value. In case of error,
     returns -1.
 */
ssize_t read_inode (volume_t *volume, uint32_t inode_no, inode_t *buffer) {

//    if (inode_no <= EXT2_BAD_INO || inode_no >= EXT2_INVALID_BLOCK_NUMBER) {
//        return -1;
//    }

    if (inode_no == 0 || inode_no > volume->super.s_inodes_count) return -1;

//    printf("%x\n", volume->super.s_inodes_per_group);
//    block group = (inode – 1) / INODES_PER_GROUP
    unsigned int blockNumber = (inode_no - 1) / volume->super.s_inodes_per_group;
//    index = (inode – 1) % INODES_PER_GROUP
    unsigned int inodeIndex = (inode_no - 1) % volume->super.s_inodes_per_group;
//    printf("%d\n", blockNumber);
    unsigned int containingBlock = (inodeIndex * volume->super.s_inode_size);
//    From the Block Group Descriptor, extract the location of the block group's inode table.
    unsigned int inodeTable = volume->groups[blockNumber].bg_inode_table;
//    Determine the index of the inode in the inode table.
//    Index the inode table (taking into account non-standard inode size).

    /* TO BE COMPLETED BY THE STUDENT */
    return read_block(volume, inodeTable, containingBlock, volume->super.s_inode_size, buffer);
}

uint32_t readInode(volume_t *volume, uint32_t blockNumber, uint32_t offset) {

    uint32_t actualOffset = offset << 2;
    uint32_t buffer;

    uint32_t iNodeIndex = read_block(volume, blockNumber, actualOffset, 1 << 2, &buffer);

    return (iNodeIndex > 0 && iNodeIndex != EXT2_INVALID_BLOCK_NUMBER) ? buffer : EXT2_INVALID_BLOCK_NUMBER;
}

/* read_inode_block_no: Returns the block number containing the data
   associated to a particular index. For indices 0-11, returns the
   direct block number; for larger indices, returns the block number
   at the corresponding indirect block.

   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure where data is to be sourced.
     index: Index to the block number to be searched.

   Returns:
     In case of success, returns the block number to be used for the
     corresponding entry. This block number may be 0 (zero) in case of
     sparse files. In case of error, returns
     EXT2_INVALID_BLOCK_NUMBER.
 */
uint32_t get_inode_block_no (volume_t *volume, inode_t *inode, uint64_t block_idx) {

    /* TO BE COMPLETED BY THE STUDENT */
    uint32_t singly = volume->block_size >> 2; // 1024 entries
    uint32_t doubly = singly * singly;
    //Direct Block            Indirect Block       Doubly       Triply?
    //[0      -           11][12                 ][13         ][14]

    int startIndexOfIndirectBlk = (1 + 2) << 2;
    uint32_t startIndexOfDoublyIndirectBlk = startIndexOfIndirectBlk + singly;
    uint32_t startIndexOfTriplyIndirectBlk = startIndexOfDoublyIndirectBlk + doubly;
    uint32_t EndIndexOfIndirectBlkRng = startIndexOfTriplyIndirectBlk + singly * doubly;

    //direct Block
    if (block_idx < startIndexOfIndirectBlk) return inode->i_block[block_idx];

    //1-indirect

    if (block_idx >= startIndexOfIndirectBlk && block_idx < startIndexOfDoublyIndirectBlk) {

        uint32_t offset = block_idx - startIndexOfIndirectBlk;
        return                 readInode(volume, inode->i_block_1ind,       offset);
    }

    //2-indirect
    if (block_idx >= startIndexOfDoublyIndirectBlk && block_idx < startIndexOfTriplyIndirectBlk) {

        uint32_t offset = block_idx - startIndexOfDoublyIndirectBlk;
        uint32_t blockIndex1 = readInode(volume, inode->i_block_2ind, offset / singly);
        return                 readInode(volume, blockIndex1,         offset % singly);
    }

        //3-indirect
    else if (block_idx >= startIndexOfTriplyIndirectBlk && block_idx < EndIndexOfIndirectBlkRng) {

        uint32_t offset = block_idx - startIndexOfTriplyIndirectBlk;
        uint32_t blockIndex1 = readInode(volume, inode->i_block_3ind, offset / doubly);

        uint32_t remainderOffset = (offset / singly) % singly;
        uint32_t blockIndex2 = readInode(volume, blockIndex1,         remainderOffset);
        return                 readInode(volume, blockIndex2,         offset % singly);
    }
    //Invalid
    return EXT2_INVALID_BLOCK_NUMBER;
}



/* read_file_block: Returns the content of a specific file, limited to
   a single block.

   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the file.
     offset: Offset, in bytes from the start of the file, of the data
             to be read.
     max_size: Maximum number of bytes to read from the block.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns the number of bytes read from the
     disk. In case of error, returns -1.
 */
ssize_t read_file_block(volume_t *volume, inode_t *inode, uint64_t offset, uint64_t max_size, void *buffer) {

    uint64_t blockNumber = get_inode_block_no(volume, inode, offset / volume->block_size);
    uint64_t actualOffset = offset % volume->block_size;
    uint64_t fileLimit = inode_file_size(volume, inode);

    if (offset >= fileLimit) max_size = 0;

    if (offset + max_size >= fileLimit) max_size = fileLimit - offset;

    if (actualOffset + max_size >= volume->block_size) max_size = volume->block_size - actualOffset;

    return read_block(volume, blockNumber, actualOffset, max_size, buffer);
}

/* read_file_content: Returns the content of a specific file, limited
   to the size of the file only. May need to read more than one block,
   with data not necessarily stored in contiguous blocks.

   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the file.
     offset: Offset, in bytes from the start of the file, of the data
             to be read.
     max_size: Maximum number of bytes to read from the file.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns the number of bytes read from the
     disk. In case of error, returns -1.
 */
ssize_t read_file_content (volume_t *volume, inode_t *inode, uint64_t offset, uint64_t max_size, void *buffer) {

    uint32_t read_so_far = 0;

    if (offset + max_size > inode_file_size(volume, inode))
        max_size = inode_file_size(volume, inode) - offset;

    while (read_so_far < max_size) {
        int rv = read_file_block(volume, inode, offset + read_so_far,
                                 max_size - read_so_far, buffer + read_so_far);
        if (rv <= 0) return rv;
        read_so_far += rv;
    }
    return read_so_far;
}
