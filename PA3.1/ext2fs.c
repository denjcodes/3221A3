#include "ext2.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

/* The FUSE version has to be defined before any call to relevant
   includes related to FUSE. */
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif
#define _FILE_OFFSET_BITS  64
#include <fuse.h>

static volume_t *volume;

static void *ext2_init(struct fuse_conn_info *conn);
static void ext2_destroy(void *private_data);
static int ext2_getattr(const char *path, struct stat *stbuf);
static int ext2_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int ext2_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi);
static int ext2_readlink(const char *path, char *buf, size_t size);

static const struct fuse_operations ext2_operations = {
        .init       = ext2_init,
        .destroy    = ext2_destroy,
        .read       = ext2_read,
        .getattr    = ext2_getattr,
        .readdir    = ext2_readdir,
        .readlink   = ext2_readlink,
};

int main(int argc, char *argv[]) {

    char  *volumefile = argv[--argc];
    volume = open_volume_file(volumefile);
    argv[argc] = NULL;

    if (!volume) {
        fprintf(stderr, "Invalid volume file: '%s'.\n", volumefile);
        exit(1);
    }

    fuse_main(argc, argv, &ext2_operations, volume);

    return 0;
}

/* ext2_init: Function called when the FUSE file system is mounted.
 */
static void *ext2_init(struct fuse_conn_info *conn) {

    printf("init()\n");

    return NULL;
}

/* ext2_destroy: Function called before the FUSE file system is
   unmounted.
 */
static void ext2_destroy(void *private_data) {

    printf("destroy()\n");

    close_volume_file(volume);
}

/* ext2_getattr: Function called when a process requests the metadata
   of a file. Metadata includes the file type, size, and
   creation/modification dates, among others (check man 2
   fstat). Typically FUSE will call this function before most other
   operations, for the file and all the components of the path.

   Parameters:
     path: Path of the file whose metadata is requested.
     stbuf: Pointer to a struct stat where metadata must be stored.
   Returns:
     In case of success, returns 0, and fills the data in stbuf with
     information about the file. If the file does not exist, should
     return -ENOENT.
 */
static int ext2_getattr(const char *path, struct stat *stbuf) {

    /* TO BE COMPLETED BY THE STUDENT */

    // man 2 fstat >> fstat.txt
    inode_t *sourceInode = malloc(sizeof(inode_t));
    uint32_t iNodeNumber = find_file_from_path(volume, path, sourceInode);

    if (iNodeNumber < 1) return -ENOENT;

    //stbuf->st_dev
    stbuf->st_ino       = iNodeNumber;
    stbuf->st_mode      = sourceInode->i_mode;
    stbuf->st_nlink     = sourceInode->i_links_count;
    stbuf->st_uid       = sourceInode->l_i_uid_high << 16 | sourceInode->i_uid;
    stbuf->st_gid       = sourceInode->l_i_gid_high << 16 | sourceInode->i_gid;
    stbuf->st_size      = inode_file_size(volume, sourceInode);
    stbuf->st_blksize   = volume->block_size;
    stbuf->st_blocks    = sourceInode->i_blocks;

    stbuf->st_atime     = sourceInode->i_atime;
    stbuf->st_mtime     = sourceInode->i_mtime;
    stbuf->st_ctime     = sourceInode->i_ctime;
    free(sourceInode);

    return 0;
}

int freeDouble(dir_entry_t *nextDirectory, inode_t *sourceInode, int value) {
    free(nextDirectory); free(sourceInode);
    return value;
}

/* ext2_readdir: Function called when a process requests the listing
   of a directory.

   Parameters:
     path: Path of the directory whose listing is requested.
     buf: Pointer that must be passed as first parameter to filler
          function.
     filler: Pointer to a function that must be called for every entry
             in the directory.  Will accept four parameters, in this
             order: buf (previous parameter), the filename for the
             entry as a string, a pointer to a struct stat containing
             the metadata of the file (optional, may be passed NULL),
             and an offset for the next call to ext2_readdir
             (optional, may be passed 0).
     offset: Will usually be 0. If a previous call to filler for the
             same path passed a non-zero value as the offset, this
             function will be called again with the provided value as
             the offset parameter. Optional.
     fi: Not used in this implementation of readdir.

   Returns:
     In case of success, returns 0, and calls the filler function for
     each entry in the provided directory. If the directory doesn't
     exist, returns -ENOENT.
 */
static int ext2_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi) {

    inode_t *sourceInode = malloc(sizeof(inode_t));
    uint32_t iNodeNumber = find_file_from_path(volume, path, sourceInode);
    dir_entry_t *nextDirectory = malloc(sizeof(dir_entry_t));
    int directoryInodeNumber;
    int fillerOffset = 0;

    if (iNodeNumber) {
        directoryInodeNumber = next_directory_entry(volume, sourceInode, &offset, nextDirectory);
        if (directoryInodeNumber <= 0) freeDouble(nextDirectory, sourceInode, -ENOTDIR);

        while (directoryInodeNumber > 0) {
            filler(buf, nextDirectory->de_name, NULL, fillerOffset);
            directoryInodeNumber = next_directory_entry(volume, sourceInode, &offset, nextDirectory);
            if (directoryInodeNumber <= 0) freeDouble(nextDirectory, sourceInode, -ENOTDIR);
        }

        freeDouble(nextDirectory, sourceInode, 0);
    }

    freeDouble(nextDirectory, sourceInode, -ENOENT);
}

/* ext2_read: Function called when a process reads data from a file in
   the file system. This function stores, in array 'buf', up to 'size'
   bytes from the file, starting at offset 'offset'. It may store less
   than 'size' bytes only in case of error or if the file is smaller
   than size+offset.

   Parameters:
     path: Path of the open file.
     buf: Pointer where data is expected to be stored.
     size: Maximum number of bytes to be read from the file.
     offset: Byte offset of the first byte to be read from the file.
     fi: Data structure containing information about the file being
         opened. Can be ignored in this implementation.
   Returns:
     In case of success, returns the number of bytes actually read
     from the file--which may be smaller than size, or even zero, if
     (and only if) offset+size is beyond the end of the file. In case
     of error, may return one of these error codes (not all of them
     are required):
       -ENOENT: If the file does not exist;
       -EISDIR: If the path corresponds to a directory;
       -EIO: If there was an I/O error trying to obtain the data.
 */
static int ext2_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi) {

    inode_t *sourceInode = malloc(sizeof(inode_t));
    uint32_t iNodeNumber = find_file_from_path(volume, path, sourceInode);


    if ((sourceInode->i_mode >> 12) == 4) return -EISDIR;
    if (iNodeNumber == 0) return -ENOENT;


    int readBytes = read_file_content(volume, sourceInode, offset, size, buf);
    if (readBytes == 0) return -1;

    free(sourceInode);

    /* TO BE COMPLETED BY THE STUDENT */
    return -EIO;
}

/* ext2_readlink: Function called when FUSE needs to obtain the target of
   a symbolic link. The target is stored in buffer 'buf', which stores
   up to 'size' bytes, as a NULL-terminated string. If the target is
   too long to fit in the buffer, the target should be truncated.

   Parameters:
     path: Path of the symbolic link file.
     buf: Pointer where symbolic link target is expected to be stored.
     size: Maximum number of bytes to be stored into the buffer,
           including the NULL byte.
   Returns:
     In case of success, returns 0 (zero). In case of error, may
     return one of these error codes (not all of them are required):
       -ENOENT: If the file does not exist;
       -EINVAL: If the path does not correspond to a symbolic link;
       -EIO: If there was an I/O error trying to obtain the data.
 */
static int ext2_readlink(const char *path, char *buf, size_t size) {

    inode_t *sourceInode = malloc(sizeof(inode_t));
    uint32_t iNodeNumber = find_file_from_path(volume, path, sourceInode);

    if (!inode_is_symlink(sourceInode)) return -EINVAL;

    if (strcmp(buf, "") == 0) return -EIO;

    int stringSize = strlen(sourceInode->i_symlink_target) >= size ? size - 1 : size;
    char stringBuffer[stringSize];

    strcpy(stringBuffer, buf);
    stringBuffer[stringSize] = '\0';

//    Assertion 'correct_buffer == buffer' failed: correct_buffer == "", buffer == ""

    int32_t symRetValue = read_symlink_target(volume, sourceInode, buf, stringSize);
    if (symRetValue >= 0) return 0;

    return -ENOENT;
}