You will primarily be writing code in four different files (and we suggest you tackle them in this order):
- inode.c  (manipulating, using the information in inode)
- file.c  (one more layer for using file)

then, you can run some sanity checks and finish other two.
- directory.c
- pathname.c

## 1. Sector reading

The basic idea of the assignment is to write code that will be able to locate and read files in the file system. You will, for example, be using a function we've written for you to read sectors from the disk image:
```c
/**
 * Reads the specified sector (e.g. block) from the disk.  Returns the number of bytes read,
 * or -1 on error.
 */
int diskimg_readsector(int fd, int sectorNum, void *buf);
```
file descriptor, sector number, buffer.

Sometimes, buf will be an array of inodes, sometimes it will be a buffer that holds actual file data. In any case, the function will always read DISKIMG_SECTOR_SIZE number of bytes, and it is your job to determine the relevance of those bytes.

It is critical that you carefully read through the header files for this assignment (they are actually relatively short). There are key constants (e.g., ROOT_INUMBER, struct direntv6, etc.) that are defined for you to use, and reading them will give you an idea about where to start for parts of the assignment.

## 2. Tricky function

One function that can be tricky to write is the following:
```c
/**
 * Gets the location of the specified file block of the specified inode.
 * Returns the disk block number on success, -1 on error.
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum);
```
- The unixfilesystem struct is defined and initialized for you.
- The inode struct will be populated already
- The blockNum is the number, in linear order, of the block you are looking for in a particular file.

For example, let's say the inode indicates that the file it refers to has a size of 180,000 bytes. And let's assume that blockNum is 302.

Since this file is large, it's indirect addressing. The 302nd block is going to fall into the *second* indirect block, because each block has 256 block numbers. 

You, therefore, need to use diskimg_readsector to read the sector listed in the 2nd block number (which is in the inode struct), then extract the (302 % 256)th short from that block, and return the value you find there.

## 3. Be aware when reading directory

* You do not have to follow symbolic links (you can ignore them completely)
* You do need to consider directories that are longer than 32 files long (because they will take up more than two blocks on the disk), but this is not a special case! You are building generic functions to read files, so you can rely on them to do the work for you, even for directory file reading.
* Don't forget that a filename is limited to 14 characters, and if it is exactly 14 characters, there is not a trailing '\0' at the end of the name (this to conserve that one byte of data!) So...you might want to be careful about using strcmp for files (maybe use strncmp, instead?)