#include <fcntl.h>
// for open
#include <unistd.h>
// for read, write, close
#include <stdio.h>
#include <sys/types.h> // for umask
#include <sys/stat.h>  // for umask
#include <errno.h>

static void writeall(int, const char[], size_t);

int main(int argc, char* argv[]) {
    int fds[argc];
    fds[0] = STDOUT_FILENO;
    for (size_t i = 1; i < argc; i++) 
        fds[i] = open(argv[i], O_WRONLY | O_CREAT | O_EXCL, 0644);

    char buffer[2048];
    while (true) {
        ssize_t numRead = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (numRead == 0) break;
        for (size_t i = 0; i < argc; i++) writeall(fds[i], buffer, numRead);
    }

    for (size_t i = 1; i < argc; i++) close(fds[i]);
    return 0;
}

static void writeall(int fd, const char buffer[], size_t len) {
    size_t numWritten = 0;
    while (numWritten < len) {
        numWritten += write(fd, buffer + numWritten, len - numWritten);
    }
}