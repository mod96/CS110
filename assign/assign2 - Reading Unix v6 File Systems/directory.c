#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * Looks up the specified name (name) in the specified directory (dirinumber).
 * If found, return the directory entry in space addressed by dirEnt.  Returns 0
 * on success and something negative on failure.
 */
int directory_findname(struct unixfilesystem *fs, const char *name, int dirinumber, struct direntv6 *dirEnt)
{
	// check if it's directory
	struct inode ino;
	if (inode_iget(fs, dirinumber, &ino) < 0 || (ino.i_mode & IFMT) != IFDIR)
		return -1;
	// logic
	int blockNum = 0;
	char buf[DISKIMG_SECTOR_SIZE];
	struct direntv6 *dir = (struct direntv6 *)buf;
	int validSize, numEntriesInBlock;
	while (1)
	{
		validSize = file_getblock(fs, dirinumber, blockNum, dir);
		if (validSize <= 0)
			break;
		numEntriesInBlock = validSize / sizeof(struct direntv6);
		for (int i = 0; i < numEntriesInBlock; i++)
		{
			if (strncmp(name, dir[i].d_name, strlen(name)) == 0)
			{
				*dirEnt = dir[i];
				return 0;
			}
		}
		blockNum++;
	}
	return -1;
}
