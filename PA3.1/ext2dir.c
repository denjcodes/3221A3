#include "ext2.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

/* next_directory_entry: Reads and returns one entry in a
   directory. Can be called repeatedly for the same inode to obtain
   additional entries.

   Parameters:
     volume: Pointer to volume.
     dir_inode: Pointer to inode structure for the directory.
     offset: Pointer to a variable keeping track of the inode
             offset. This variable must be set to zero before the
             first call to this function. The variable will be updated
             with each subsequent call, so that calling this function
             repeatedly with the same offset location will return
             consecutive directory entries.
     dir_entry: Data structure where the resulting directory entry's
                data will be stored. The de_name data structure will
                contain the name as a null-terminated string.

   Returns:
     On success returns the inode number for the next entry. If the
     inode is not a directory, or there is an error reading the
     directory data, returns -1. If there are no more entries
     in the directory, returns 0 (zero).
 */
int64_t next_directory_entry(volume_t *volume, inode_t *dir_inode,
                             off_t *offset, dir_entry_t *dir_entry) {

    /* TO BE COMPLETED BY THE STUDENT */
    //15   11   7    3  0
    //0004 0000 0000 0000
    if (!inode_is_directory(dir_inode)) return -1;
    if (dir_inode->i_mode >> 12 != 0x4) return -1;

    dir_entry_t *directoryEntry = malloc(sizeof(dir_entry_t));
    ssize_t readBytes = read_file_content(volume, dir_inode, *offset, sizeof(dir_entry_t), directoryEntry);
    uint64_t fileLimit = inode_file_size(volume, dir_inode);

    if (readBytes <= 0) return 0;
    directoryEntry->de_name[directoryEntry->de_name_len] = '\0';

    if (dir_entry != NULL)
        memcpy(dir_entry, directoryEntry, sizeof(dir_entry_t));

    *offset += dir_entry->de_rec_len;
    uint32_t nextInodeNumber = directoryEntry->de_inode_no;

    if (nextInodeNumber == 0)
        return next_directory_entry(volume, dir_inode, offset, dir_entry);

    // offset < 1024
    if (*offset >= inode_file_size(volume, dir_inode))
//        *offset = *offset - *offset % fileLimit;
        *offset -= (*offset % fileLimit);

    free(directoryEntry);

    if (*offset <= fileLimit) return nextInodeNumber;
    return 0;
//  return nextInodeNumber;
}

/* find_file_in_directory: Searches for a file in a directory.

   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the directory.
     name: NULL-terminated string for the name of the file. The file
           name must match this name exactly, including case.
     buffer: If the file is found, and this pointer is set to a
             non-NULL value, this buffer is set to the directory entry
             of the file. If the pointer is NULL, nothing is saved. If
             the file is not found, the value of this buffer is
             unmodified.

   Returns:
     If the file exists in the directory, returns the inode number
     associated to the file. If the inode is not a directory, or there
     is an error reading the directory data, returns -1. If the name
     does not exist, returns 0 (zero).
 */
int64_t find_file_in_directory(volume_t *volume, inode_t *inode, const char *name, dir_entry_t *buffer) {

    /* TO BE COMPLETED BY THE STUDENT */
    off_t offset = 0;

//    if(!inode_is_directory(inode) && !inode_is_regular_file(inode)) return -1;
    if (inode->i_mode >> 12 != 0x4) return -1;

    dir_entry_t *directoryEntry = malloc(sizeof(dir_entry_t));

    while (next_directory_entry(volume, inode, &offset, directoryEntry) > 0){
        char *bufferName = malloc(sizeof(char) * directoryEntry->de_name_len + 1);

        strcpy(bufferName, directoryEntry->de_name);
        bufferName[directoryEntry->de_name_len] = '\0';

        if (strcmp(directoryEntry->de_name, name) == 0) {
            int iNodeNumber = directoryEntry->de_inode_no;

            if (buffer != NULL)
                memcpy(buffer, directoryEntry, sizeof(dir_entry_t));

            free(directoryEntry);
            return iNodeNumber;
        }
    }

    free(directoryEntry);
    return 0;
}

int freeTriple(dir_entry_t *directoryEntry, char *sourcePath, inode_t *sourceInode, int value) {
    free(directoryEntry); free(sourcePath); free(sourceInode);
    return value;
}

/* find_file_from_path: Searches for a file based on its full path.

   Parameters:
     volume: Pointer to volume.
     path: NULL-terminated string for the full absolute path of the
           file. Must start with '/' character. Path components
           (subdirectories) must be delimited by '/'. The root
           directory can be obtained with the string "/".
     dest_inode: If the file is found, and this pointer is set to a
                 non-NULL value, this buffer is set to the inode of
                 the file. If the pointer is NULL, nothing is
                 saved. If the file is not found, the value of this
                 buffer is unmodified.

   Returns:
     If the file exists, returns the inode number associated to the
     file. If the file does not exist, or there is an error reading
     any directory or inode in the path, returns 0 (zero).
 */
uint32_t find_file_from_path(volume_t *volume, const char *path, inode_t *dest_inode) {

    /* TO BE COMPLETED BY THE STUDENT -dj*/
    inode_t *sourceInode = malloc(sizeof(inode_t));
    char *sourcePath = malloc(sizeof(char) * strlen(path) +1);
    dir_entry_t *directoryEntry = malloc(sizeof(dir_entry_t));
    int currentNode = EXT2_ROOT_INO;

    read_inode(volume, EXT2_ROOT_INO, sourceInode);
    strcpy(sourcePath, path);
    sourcePath[strlen(path)] = '\0';

    char *tokenizer = strtok(sourcePath, "/");

    if (tokenizer == NULL) {
        if (dest_inode != NULL)
            memcpy(dest_inode, sourceInode, volume->super.s_inode_size);
        return freeTriple(directoryEntry, sourcePath, sourceInode, EXT2_ROOT_INO);
    }

    for (; tokenizer != NULL && currentNode > 0; tokenizer = strtok(NULL, "/"))
    {
        currentNode = find_file_in_directory(volume, sourceInode, tokenizer, directoryEntry);

        if (currentNode <= 0) return freeTriple(directoryEntry, sourcePath, sourceInode, 0);

        read_inode(volume, currentNode, sourceInode);
    }

    if (currentNode) {
        if (dest_inode != NULL)
            memcpy(dest_inode, sourceInode, volume->super.s_inode_size);
        return freeTriple(directoryEntry, sourcePath, sourceInode, currentNode);
    }

    return freeTriple(directoryEntry, sourcePath, sourceInode, 0);
}