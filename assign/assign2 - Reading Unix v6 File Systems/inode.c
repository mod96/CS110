#include <stdio.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"

#define INDIR_ADDR 7

/**
 * Fetches the specified inode from the filesystem.
 * Returns 0 on success, -1 on error.
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp)
{
	return 0;
}

/**
 * Given an index of a file block, retrieves the file's actual block number
 * of from the given inode.
 *
 * Returns the disk block number on success, -1 on error.
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum)
{
	return 0;
}

/**
 * Computes the size in bytes of the file identified by the given inode
 */
int inode_getsize(struct inode *inp)
{
	return 0;
}
