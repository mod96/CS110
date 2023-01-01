#include <stdio.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"

#define DIND 7

const int inodesPerSector = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
const int blockNumPerSector = DISKIMG_SECTOR_SIZE / sizeof(uint16_t);
const int singleLimit = DIND * blockNumPerSector;

/**
 * Fetches the specified inode from the filesystem.
 * Returns 0 on success, -1 on error.
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp)
{
	inumber--; // inode starts from 1
	int sectorNum = INODE_START_SECTOR + (inumber / inodesPerSector);
	int sectorOffset = inumber % inodesPerSector;

	struct inode inodes[inodesPerSector];
	if (diskimg_readsector(fs->dfd, sectorNum, inodes) != DISKIMG_SECTOR_SIZE)
	{
		return -1;
	}
	*inp = inodes[sectorOffset];
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
	if ((inp->i_mode) & ILARG == 0)
	{
		return inp->i_addr[blockNum];
	}
	if (blockNum < singleLimit)
	{
		uint16_t singleInd[blockNumPerSector];
		if (diskimg_readsector(fs->dfd, inp->i_addr[blockNum / blockNumPerSector], singleInd) != DISKIMG_SECTOR_SIZE)
		{
			return -1;
		};
		return singleInd[blockNum % blockNumPerSector];
	}
	else
	{
		blockNum -= singleLimit;
		uint16_t singleInd[blockNumPerSector];
		if (diskimg_readsector(fs->dfd, inp->i_addr[DIND], singleInd) != DISKIMG_SECTOR_SIZE)
		{
			return -1;
		};
		uint16_t doubleInd[blockNumPerSector];
		if (diskimg_readsector(fs->dfd, singleInd[blockNum / blockNumPerSector], doubleInd) != DISKIMG_SECTOR_SIZE)
		{
			return -1;
		}
		return doubleInd[blockNum % blockNumPerSector];
	}
}

/**
 * Computes the size in bytes of the file identified by the given inode
 */
int inode_getsize(struct inode *inp)
{
	return ((inp->i_size0) << 16) + inp->i_size1;
}
