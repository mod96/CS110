#include <fcntl.h>
// for open
#include <unistd.h>
// for read, write, close
#include <stdio.h>
#include <sys/types.h> // for umask
#include <sys/stat.h>  // for umask
#include <errno.h>
const char *kFilename = "my_file";
const int kFileExistsErr = 17;
int main()
{
    mode_t old_mask = umask(0); // set to 0 to enable all permissions to be set
    printf("The old umask was %03o\n. But now set to 0.\n", old_mask);
    int file_descriptor = open(kFilename, O_WRONLY | O_CREAT | O_EXCL, 0644); // permission = mod & ~umask
    if (file_descriptor == -1)
    {
        printf("There was a problem creating '%s'!\n", kFilename);
        if (errno == kFileExistsErr)
        {
            printf("The file already exists.\n");
        }
        else
        {
            printf("Unknown errorno: %d\n", errno);
        }
        return -1;
    }
    close(file_descriptor);
    return 0;
}