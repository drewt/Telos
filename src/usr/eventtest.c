#include <telos/process.h>
#include <telos/print.h>
#include <signal.h>

static void sleep_proc (void *arg) {
    syssleep ((unsigned int) arg);
    sysreport ("");
}

static void sleep_test (void) {
    sysputs ("Testing sleep...\nVerify ascending: ");
    syscreate (sleep_proc, (void*) 100);
    syscreate (sleep_proc, (void*) 990);
    syscreate (sleep_proc, (void*) 1440);
    syssleep (2000);

    sysputs ("\nVefify descending: ");
    syscreate (sleep_proc, (void*) 1440);
    syscreate (sleep_proc, (void*) 990);
    syscreate (sleep_proc, (void*) 100);
    syssleep (2000);
    sysputs ("\n");
}

static void alarm_handler (int signo) {
    sysputs ("rm\n");
}

static void alarm_test (void) {
    int sig;
    signal (SIGALRM, alarm_handler);
    sysputs ("Testing alarm... alrm ?= ");
    alarm (2);
    sysputs ("al");
    for (sig = sigwait (); sig != SIGALRM; sig = sigwait ());
}

void event_test (void *arg) {
    sleep_test ();
    alarm_test ();
}
