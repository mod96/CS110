#include <fcntl.h>
// for open
#include <unistd.h>
// for read, write, close
#include <stdio.h>
#include <sys/types.h> // for umask
#include <sys/stat.h>  // for umask
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "%s <source-file> <destination-file>.\n", argv[0]);
        return 1;
    }
    int fdin = open(argv[1], O_RDONLY);
    int fdout = open(argv[2], O_WRONLY | O_CREAT | O_EXCL, 0644);
    char buffer[1024];
    while (true)
    {
        ssize_t bytesRead = read(fdin, buffer, sizeof(buffer));
        if (bytesRead == 0)
            break;
        size_t bytesWritten = 0;
        while (bytesWritten < bytesRead)
        {
            bytesWritten += write(fdout, buffer + bytesWritten, bytesRead - bytesWritten);
        }
    }
    close(fdin);
    close(fdout);
    return 0;
}