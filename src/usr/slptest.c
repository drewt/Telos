#include <telos/process.h>
#include <telos/print.h>

void sleep_proc (void *arg) {
    sysreport ("Sleeping\n");
    syssleep ((unsigned int) arg);
    sysreport ("Waking\n");
}

void slp_test (void *arg) {
    syscreate (sleep_proc, (void*) 100);
    syscreate (sleep_proc, (void*) 990);
    syscreate (sleep_proc, (void*) 1440);
    syssleep (2000);

    syscreate (sleep_proc, (void*) 1440);
    syscreate (sleep_proc, (void*) 990);
    syscreate (sleep_proc, (void*) 100);
    syssleep (2000);
}
