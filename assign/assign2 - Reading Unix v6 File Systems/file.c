#include <stdio.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

/**
 * Fetches the specified file block from the specified inode.
 * Returns the number of valid bytes in the block, -1 on error.
 */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf)
{
	return 0;
}
