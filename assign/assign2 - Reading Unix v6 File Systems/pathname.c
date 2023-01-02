
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#define DIR_MAX_LEN 14

/**
 * Returns the inode number associated with the specified pathname.  This need only
 * handle absolute paths.  Returns a negative number (-1 is fine) if an error is
 * encountered.
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname)
{
	// initialize directv6 to ROOT
	struct direntv6 target;
	target.d_inumber = ROOT_INUMBER;
	// copy const char* to char* to use strtok
	char *pName = malloc(strlen(pathname) + 1);
	if (pName)
		strcpy(pName, pathname);
	// logic
	char *p = strtok(pName, "/");
	while (p)
	{
		int dirIno = target.d_inumber;
		if (directory_findname(fs, p, dirIno, &target) < 0)
		{
			return -1;
		}
		p = strtok(NULL, "/");
	}
	return target.d_inumber;
}
