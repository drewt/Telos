#include <stdio.h>
#include <unistd.h>
#include <telos/console.h> /* ioctl commands */

void consoletest (int argc, char *argv[])
{
    int fd;

    puts ("This is standard output");
    sleep (100);

    if ((fd = open ("/dev/cons1", 0)) == -1) {
        puts ("error opening console 1");
        return;
    }

    if (ioctl (fd, CONSOLE_IOCTL_SWITCH, 1) == -1) {
        puts ("error switching to console 1");
        return;
    }
    puts ("Printing to hidden console!");

    if (write (fd, "This is console 1\n", 18) == -1) {
        puts ("error writing to console 1");
        return;
    }

    sleep (100);

    if (ioctl (fd, CONSOLE_IOCTL_SWITCH, 0) == -1) {
        puts ("error switching to console 0");
        return;
    }

    puts ("Back to standard output");
}
