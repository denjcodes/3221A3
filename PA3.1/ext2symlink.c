#include "ext2.h"

#include <string.h>

/* read_symlink_target: Reads the content of the target of a symbolic link.
   
   Parameters:
     volume: Pointer to volume.
     inode: Pointer to inode structure for the symbolic link.
     buffer: Pointer to location where the target of the symbolic link
             is to be stored. The target will always be represented as
             a null-terminated string.
     size: Number of bytes allocated for the target of the symbolic
           link. If the target of the symbolic link is longer than (or
           equal to) `size` bytes, then the first `size-1` characters
           are copied to the buffer, the last byte is set to null (to
           terminate the string), and the remainder of the target is
           truncated/dropped.

   Returns:
     In case of success, returns the length of the target string. If
     the inode is not a symbolic link, or there is an error reading
     the target data, returns 0 (zero).
 */
int32_t read_symlink_target(volume_t *volume, inode_t *inode, char *buffer, size_t size) {

  if (!inode_is_symlink(inode))
    return 0;
  
  /* TO BE COMPLETED BY THE STUDENT */
  return 0;
}
