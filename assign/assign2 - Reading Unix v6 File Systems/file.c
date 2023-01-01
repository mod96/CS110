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
	struct inode ino;
	// Reading from fs
	int diskBlockNum;
	if (inode_iget(fs, inumber, &ino) == -1)
		return -1;
	if ((diskBlockNum = inode_indexlookup(fs, &ino, blockNum)) == -1)
		return -1;
	if (diskimg_readsector(fs->dfd, diskBlockNum, buf) != DISKIMG_SECTOR_SIZE)
		return -1;
	// Calculating the valid bytes in the block
	int fileSize = inode_getsize(&ino);
	int fileBlockFinal = fileSize / DISKIMG_SECTOR_SIZE;
	if (blockNum == fileBlockFinal)
		return fileSize % DISKIMG_SECTOR_SIZE;
	else if (blockNum < fileBlockFinal)
		return DISKIMG_SECTOR_SIZE;
	return 0;
}
