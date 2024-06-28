#include <usloss.h>
#include <phase1.h>

#include <stdio.h>
#include <assert.h>

#include "phase1helper.h"



/* we need to run the dispatcher if the clock interrupt fires */
static void trivial_clock_handler(int dev,void *arg)
{
    dispatcher();
}

void startup(int argc, char **argv)
{
    USLOSS_IntVec[USLOSS_CLOCK_INT] = trivial_clock_handler;

    phase1helper_init();
    phase1_init();
    startProcesses();
}



void phase2_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}

void phase3_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}

void phase4_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}



void phase1_dispatcher_wrapper(int target_pid)
{
    dispatcher();
}



void finish      (int argc, char **argv) {}
void test_setup  (int argc, char **argv) {}
void test_cleanup(int argc, char **argv) {}



void TEMP_switchTo(int pid)
{
    USLOSS_Console("This function must never be called!  I simply implemented it to make sure that you had *removed* it from your phase 1 code.\n");
    assert(0);
}

void quit_phase_1a(int status, int switchToPid)
{
    USLOSS_Console("This function must never be called!  I simply implemented it to make sure that you had *removed* it from your phase 1 code.\n");
    assert(0);
}

