#include "ext2.h"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fsuid.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#define EXT2_OFFSET_SUPERBLOCK 1024

/* open_volume_file: Opens the specified file and reads the initial
   EXT2 data contained in the file, including the boot sector, file
   allocation table and root directory.

   Parameters:
     filename: Name of the file containing the volume data.
   Returns:
     A pointer to a newly allocated volume_t data structure with
     all fields initialized according to the data in the volume file,
     or NULL if the file is invalid or data is missing.
 */
void *checkMalloc(int size){

    void *ptr =      malloc(size);

    if (ptr == NULL) {
        perror("Malloc failed");
    }

    return ptr;
}

volume_t *open_volume_file(const char *filename) {

    int fd = open(filename, O_RDONLY);
    if (fd == -1) return NULL;

    struct stat vol_st;

    if (fstat(fd, &vol_st) == -1) {
        close(fd);
        return NULL;
    }


    volume_t *volume = checkMalloc(sizeof(volume_t) * 1);
    superblock_t *superBlock = checkMalloc(sizeof(superblock_t) * 1);

    volume->fd = fd;
    volume->volume_size = vol_st.st_size;
//    volume->block_size = vol_st.st_blksize;

    if (volume->volume_size < EXT2_OFFSET_SUPERBLOCK * 2) {
        free(superBlock);
        return NULL;
    }

    //https://www.nongnu.org/ext2-doc/ext2.html

    lseek(fd, EXT2_OFFSET_SUPERBLOCK, SEEK_SET);

    if (read(fd, superBlock, sizeof(superblock_t) * 1) < 0) {
        free(superBlock);
        return NULL;
    }

    if (EXT2_SUPER_MAGIC != superBlock->s_magic) {
        free(superBlock);
        return NULL;
    }

    volume->super = *superBlock;
    volume->block_size = EXT2_OFFSET_SUPERBLOCK << superBlock->s_log_block_size;
    volume->volume_size = superBlock->s_blocks_count * volume->block_size;
    volume->num_groups = 1 + (superBlock->s_blocks_count - 1) / superBlock->s_blocks_per_group;

    //  printf("block size: %d\n", volume->block_size);
    //  printf("volume size: %d\n", volume->volume_size);
    //  printf("num groups: %d\n", volume->num_groups);

    group_desc_t *groupDescription = checkMalloc(sizeof(group_desc_t) * volume->num_groups);

    if (volume->block_size != 1024)
        lseek(fd, volume->block_size, SEEK_SET);
    else
        lseek(fd, volume->block_size * 2, SEEK_SET);

    if (read(fd, groupDescription, sizeof(group_desc_t) * volume->num_groups) < 0) {
        free(superBlock);
        free(groupDescription);
        return NULL;
    }

    volume->groups = groupDescription;

    /* TO BE COMPLETED BY THE STUDENT */

//    free(groupDescription);
    free(superBlock);
    return volume;
}

/* close_volume_file: Frees and closes all resources used by a EXT2 volume.

   Parameters:
     volume: pointer to volume to be freed.
 */
void close_volume_file(volume_t *volume) {

    close(volume->fd);
    free(volume->groups);
    free(volume);

}

/* read_block: Reads data from one or more blocks. Saves the resulting
   data in buffer 'buffer'. This function also supports sparse data,
   where a block number equal to 0 sets the value of the corresponding
   buffer to all zeros without reading a block from the volume.

   Parameters:
     volume: pointer to volume.
     block_no: Block number where start of data is located.
     offset: Offset from beginning of the block to start reading
             from. May be larger than a block size.
     size: Number of bytes to read. May be larger than a block size.
     buffer: Pointer to location where data is to be stored.

   Returns:
     In case of success, returns the number of bytes read from the
     disk. In case of error, returns -1.
 */
ssize_t read_block(volume_t *volume, uint32_t block_no, uint32_t offset, uint32_t size, void *buffer) {

    /* TO BE COMPLETED BY THE STUDENT */

    unsigned int actualOffset = volume->block_size * block_no + offset;

    if (block_no == 0)  {
        memset(buffer, 0, size); // <-- Better implementation
        return size;
    }
    else if (actualOffset + size >= volume->volume_size) {
        size = volume->volume_size - actualOffset;
        if (size == 0) return 0;
    }

    lseek(volume->fd, actualOffset, SEEK_SET);
    return (block_no == EXT2_INVALID_BLOCK_NUMBER) ?
           -1 : read(volume->fd, buffer, size);
}