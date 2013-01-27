#include <telos/process.h>
#include <telos/print.h>
#include <signal.h>

static void sleep_proc (int argc, char *argv[]) {
    syssleep (argc);
    sysreport ("");
}

static void sleep_test (void) {
    printf ("Testing sleep...\nVerify ascending: ");
    syscreate (sleep_proc, 100, NULL);
    syscreate (sleep_proc, 990, NULL);
    syscreate (sleep_proc, 1440, NULL);
    syssleep (2000);

    printf ("\nVefify descending: ");
    syscreate (sleep_proc, 1440, NULL);
    syscreate (sleep_proc, 990, NULL);
    syscreate (sleep_proc, 100, NULL);
    syssleep (2000);
    puts ("");
}

static void alarm_handler (int signo) {
    puts ("rm");
}

static void alarm_test (void) {
    int sig;
    signal (SIGALRM, alarm_handler);
    printf ("Testing alarm... alrm ?= ");
    alarm (2);
    printf ("al");
    for (sig = sigwait (); sig != SIGALRM; sig = sigwait ());
}

void event_test (int argc, char *argv[]) {
    sleep_test ();
    alarm_test ();
}
