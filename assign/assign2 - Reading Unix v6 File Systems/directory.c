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
	return 0;
}
