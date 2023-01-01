
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DIR_MAX_LEN 14

/**
 * Returns the inode number associated with the specified pathname.  This need only
 * handle absolute paths.  Returns a negative number (-1 is fine) if an error is
 * encountered.
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname)
{
	return 0;
}
