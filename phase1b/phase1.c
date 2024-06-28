/*
 * Author: Jake Gridley
 * CLASS: CSC452
 * Date: Feb 8, 2024
 * phase1.c: This program is used to create a simulated OS in which processes are stored and run with the help of the testcases, as well as
 * 			 libraries such as 'phase1helper.h' and 'usloss.h'.  This file contains a PCB struct with all the information needed for a given
 * 			 process.  The data structures are instantiated inside phase1_init and global variables are set such as the Process Table (PTable).
 *			 The main data structures are an array of PCB's each slot has a single PCB, an array of 7 Queues with each priority (1-6) having its
 *			 own queue plus a queue for blocked PCBs, and a tree structure for each PCB holding relationships between parent and children.			 
*/

// libraries included
#include "phase1.h"
#include "phase1helper.h"
#include "usloss.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>


// global variables 
int curProc = 0; // currently running process
int curSize = 1;
int nextPID = 2;
int start = 0;

/*
 * This struct defines one process control block which contains information on a single process
 * such as the current state of the process, the name, the priority, the process ID, and the context.
 * The process table contains all the PCBs that are needed to keep track of.
*/
struct PCB {
	int priority;
	int PID;
	int state; // state vs status? status means blocked or runnable state means...??
	int stacksize;
	int status;  // 0 means runnable, >0 means blocked
	int parentPID;
	char *name;
	char *stack;
	USLOSS_Context context;
	struct PCB *run_queue_next;
	struct PCB *firstChild;
	struct PCB *nextSibling;
};

/*
 * A single RunQueue (7 of them total) one for each priority and a single queue for blocked PCBs
 *
 *
*/
struct RunQueue {
	int priority;
	int size;
	struct PCB *first;
};

// Process Table with MAXPROC slots for PCBs
struct PCB PTable[MAXPROC];

// Array containing all runQueues
struct RunQueue QTable[7];


// Returns the currently running process's PID
int getpid(void) {
	return curProc;
}

// This function ensures the we are in kernel mode and if we arent it will print an error
// The function also disables interrupts and returns the original psr so that it can be reinstated later
unsigned int check_kernel(void) {
	unsigned int psr = USLOSS_PsrGet();
	unsigned int mask = 0b00000001;
	unsigned int kernel = psr & mask;
	if (kernel != 1) {
		USLOSS_Console("ERROR --- CURRENTLY IN USER MODE\n");
		USLOSS_Halt(1);
	}	
	unsigned int mask2 = 0b00000011;
	unsigned int zeroed = psr & (~mask2);
	unsigned int mask3 = 0b00000001;
	unsigned int newPsr = zeroed | mask3;
	USLOSS_PsrSet(newPsr);
	return psr;
}

// this function initializes all the runQueues and is called in phase1_init
void set_queues() {
	struct RunQueue qb;
	struct RunQueue q1;
	struct RunQueue q2;
	struct RunQueue q3;
	struct RunQueue q4;
	struct RunQueue q5;
	struct RunQueue q6;
	qb.priority = 0;
	qb.size = 0;
	qb.first = NULL;
	QTable[0] = qb;
	q1.priority = 1;
	q1.size = 0;
	q1.first = NULL;
	QTable[1] = q1;
	q2.priority = 2;
	q2.size = 0;
	q2.first = NULL;
	QTable[2] = q2;
	q3.priority = 3;
	q3.size = 0;
	q3.first = NULL;
	QTable[3] = q3;
	q4.priority = 4;
	q4.size = 0;
	q4.first = NULL;
	QTable[4] = q4;
	q5.priority = 5;
	q5.size = 0;
	q5.first = NULL;
	QTable[5] = q5;
	q6.priority = 6;
	q6.size = 0;
	q6.first = NULL;
	QTable[6] = q6;
}

// Initialized the data structures needed and creates the initial PCB for 'init main' function
void phase1_init(void) {
	unsigned int oldPsr = check_kernel();
	start = 0;
	set_queues();
	memset(PTable, 0, sizeof(struct PCB) * MAXPROC);	
	struct PCB init;
	init.name = "INIT MAIN";
	init.priority = 6;
	init.PID = 1;
	init.state = 1;
	init.status = 0;
	char *stack = malloc(USLOSS_MIN_STACK);
	init.stack = stack;
	init.stacksize = USLOSS_MIN_STACK;
	PTable[(init.PID % MAXPROC)] = init;
	QTable[6].first = &PTable[init.PID];
	QTable[6].size++;
	russ_ContextInit(1, &PTable[init.PID].context, stack, USLOSS_MIN_STACK, &init_main, NULL);
	//USLOSS_Console("RUnning in phase1 init\n");
	USLOSS_PsrSet(oldPsr);

}

// this function moves the first item in the queue to the back (used for switching processes every 80ms when necessary
void requeue(struct RunQueue q) {
	struct PCB *firstPCB = q.first;

	struct PCB *next = firstPCB->run_queue_next;
	while (next->run_queue_next != NULL) {
		next = next->run_queue_next;
	}
	next->run_queue_next = firstPCB; // syntax may be incorrect

}


// This function runs the correct processes in the RunQueue
void run(struct RunQueue q) {
	//USLOSS_Console("In run...\n");
	struct PCB *firstPCB = q.first;
	//USLOSS_Console("size of q is %d\n", q.size);
	if (q.size == 1) {
		int oldPID = curProc;
		curProc = firstPCB->PID;
		//USLOSS_Console("new proc is %d\n", curProc);
		USLOSS_ContextSwitch(&PTable[(oldPID % MAXPROC)].context, &PTable[(curProc % MAXPROC)].context);
	} else {
		int curPID = curProc;
		int next = firstPCB->PID;
		for (int i = 0; i<q.size; i++) {
			int curTime = currentTime();
			int oldProc = curProc;
			curProc = next;
			USLOSS_ContextSwitch(&PTable[(oldProc % MAXPROC)].context, &PTable[(curProc % MAXPROC)].context);
			
			int newTime = 0;
			while (true) {
				newTime = currentTime();
				if (newTime == curTime+80) {
					break;
				}
			}
			curPID = nextPID;
			nextPID = PTable[(curPID % MAXPROC)].run_queue_next->PID;
			requeue(q);
		}	
	}
}


// keep track of time when context switching
void dispatcher(void) {  // curProc needs to be updated BEFORE contextSwitching
	unsigned int oldPsr = check_kernel();
	//USLOSS_Console("In Dispatcher\n");
	if (curProc == 0) {
		curProc = 1;
		USLOSS_ContextSwitch(NULL, &PTable[1].context);
		return;
	}
	for (int i = 1; i<=6; i++) {
		//USLOSS_Console("QTable %d has size %d\n", i, QTable[i].size);
		if (QTable[i].size > 0) {
			//USLOSS_Console("priority is %d\n", PTable[curProc].priority);
			
			if ((PTable[curProc].priority > i) || (PTable[curProc].status > 0)) { // what about if the curProc has same priority?
				//USLOSS_Console("Enters run\n");
				run(QTable[i]);
			}
			break; // why break? because the processes to run have already been found
		}
	}
	//USLOSS_Console("Exiting dispatcher\n");
	USLOSS_PsrSet(oldPsr);
}


// Adds a child to the currently running process so that a tree structure of processes is formed
void addChild(int pid) {
	if (PTable[(curProc % MAXPROC)].firstChild == NULL) {
		PTable[(curProc % MAXPROC)].firstChild = &PTable[(pid % MAXPROC)];
	} else {
		struct PCB *first = PTable[(curProc % MAXPROC)].firstChild;
		struct PCB *newFirst =  &PTable[(pid % MAXPROC)];
		PTable[(curProc % MAXPROC)].firstChild = newFirst;
		newFirst->nextSibling = first;
	}
}

// this function adds a process to the head of the correct queue
void addToQueue(int priority, int PID) {

	struct RunQueue *q = &QTable[priority];   
    if (PTable[PID].status != 0) {
		q = &QTable[0];
	}
    if (q->first == NULL) {
        q->first = &PTable[PID % MAXPROC];
    } else {
        struct PCB *next = q->first;
        while (next->run_queue_next != NULL) {
            next = next->run_queue_next;
        }
        next->run_queue_next = &PTable[PID % MAXPROC];
    }
	q->size++;
}


// Creates a new PCB with the given arguments and sets this new PCB to be the child of the currently running PCB
int spork(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {  // maybe complete?
	unsigned int oldPsr = check_kernel();
	//USLOSS_Console("SPORK is running\n");
	if (stacksize < USLOSS_MIN_STACK) {
		return -2;
	}
	if ((curSize == 50) || (priority > 5) || (priority < 1) || (func == NULL) || (name == NULL) || (strlen(name) > MAXNAME)) {
		return -1;
	}
	struct PCB child;
	child.name = name;
	child.priority = priority;
	child.PID = nextPID;
	char *stack = malloc(stacksize);	
	child.stacksize = stacksize;
	child.state = 1;
    child.status = 0;	
	child.parentPID = curProc;
	child.stack = stack;
	child.firstChild = NULL;
	child.nextSibling = NULL;
	child.run_queue_next = NULL;
	PTable[(nextPID % MAXPROC)] = child;
	//USLOSS_Console("HERE **********\n");
	addChild(nextPID);
	//USLOSS_Console("HERE **********\n");
	russ_ContextInit(PTable[(nextPID % MAXPROC)].PID, &PTable[(nextPID % MAXPROC)].context, stack, stacksize, func, arg);
	//USLOSS_Console("HERE **********\n");
	addToQueue(priority, nextPID); 
	//USLOSS_Console("HERE **********\n");
	nextPID++;
	dispatcher();
	//dumpProcesses();
	//USLOSS_Console("Exiting spork\n");
	USLOSS_PsrSet(oldPsr);
	return child.PID;
}

// This function is used inside of join to find the first child of a given process that is exitted and ready 
// to be joined back with its parent.  This function is recursive and checks each of the processes children
// until it finds the first one to be exitted.
int checkChildren(int *status, int pidIdx) {
	//USLOSS_Console("in checkchildren with pid of %d\n", pidIdx);
	if (PTable[(pidIdx % MAXPROC)].PID == 0) {
		if (PTable[(pidIdx % MAXPROC)].nextSibling == NULL) {
			return -2;
		} else {
			return checkChildren(status, PTable[(pidIdx % MAXPROC)].nextSibling->PID);
		}
	}
	//USLOSS_Console("status of pid(%d) is %d\n", pidIdx, PTable[pidIdx % MAXPROC].status);
	if (PTable[(pidIdx % MAXPROC)].status > 0) {
		*status = PTable[(pidIdx % MAXPROC)].status;
		return PTable[(pidIdx % MAXPROC)].PID;	
	} else {
		if (PTable[(pidIdx % MAXPROC)].nextSibling != NULL) {	
			return checkChildren(status, PTable[(pidIdx % MAXPROC)].nextSibling->PID);
		}
	}	
}	

//
// This function removes a PCB from the a runqueue when it is blocked
//
void removeFromQueue(int pid) {
	int priority = PTable[pid % MAXPROC].priority;
	struct RunQueue *q = &QTable[priority];
    struct PCB *current = q->first;
    struct PCB *prev = NULL;
    
	//USLOSS_Console("size of q for priority(%d) is %d\n", priority, q->size);
    while (current != NULL) {
        //USLOSS_Console("RUnning here in remove\n");
		if (current->PID == pid) {
            if (prev == NULL) {
                q->first = current->run_queue_next;
            } else {
                prev->run_queue_next = current->run_queue_next;
            }
            q->size--;
			//USLOSS_Console("size of q for priority(%d) is %d\n", priority, q->size);
            return;
        }
        prev = current;
        current = current->run_queue_next;
    }
	//USLOSS_Console("size of q for priority(%d) is %d\n", priority, q->size);


}

//
// needs work, look over spec
// specifically --- if a child has already died then dont block (modify checkChildren)
//
//	THis function sets the status of a processes first dead child and blocks the current process
//
//
int join(int *status) { // blocks until a child quits, if a child is dead then dont block and return immediately 
	unsigned int oldPsr = check_kernel(); 
	//USLOSS_Console("In join....\n");
	//dumpProcesses();
	if (PTable[(curProc % MAXPROC)].firstChild == NULL) {
		return -2;
	}
	if (status == NULL) {
		return -3;
	}
	int val = checkChildren(status, PTable[(curProc % MAXPROC)].firstChild->PID);  // be careful when accessing children, have gotten many seg faults
	if (val == PTable[(curProc % MAXPROC)].firstChild->PID) {
		PTable[(curProc % MAXPROC)].firstChild = PTable[(val % MAXPROC)].nextSibling;
	}


	//USLOSS_Console("val returned by checkchilden is %d\n", val);
	if (val == -2) {
		//blockMe(11);
		PTable[curProc].status = 11;
		removeFromQueue(curProc);
		//dumpProcesses();
		dispatcher();
		int newVal = checkChildren(status, PTable[(curProc % MAXPROC)].firstChild->PID);
		//USLOSS_Console("new value: %d\n", newVal);
		return newVal;
	}
	



	free(PTable[(val % MAXPROC)].stack); // might need to dereference
	memset(&PTable[(val % MAXPROC)], 0, sizeof(struct PCB)); // free stack memory
	USLOSS_PsrSet(oldPsr);
	return val; //pid of child
}


// this function blocks the current process and runs the dispatcher afterwards
void blockMe(int block_status) {
	unsigned int oldPsr = check_kernel();
	if (block_status <= 10) {
		USLOSS_Console("Error in 'blockMe' -- block_status value invalid\n");
		USLOSS_Halt(1);
	}

	// mark PCB as blocked
	PTable[(curProc % MAXPROC)].status = block_status;
	removeFromQueue(curProc);
	//USLOSS_Console("PRocess to remove: %d\n", curProc);
	addToQueue(0, curProc);
	// then run dispatcher...?
	dispatcher();
	USLOSS_PsrSet(oldPsr);
}


// this function quits the current process and assigns the status to the appropriate field
__attribute__((noreturn)) void quit(int status) {
	unsigned int oldPsr = check_kernel();
	// mark curProc as terminated
	//USLOSS_Console("In quit\n");
	//USLOSS_Console("status passed: %d\n", status);	
	PTable[curProc % MAXPROC].status = status;
	removeFromQueue(curProc);
	//USLOSS_Console("unblocking PCB with id: %d\n", PTable[curProc % MAXPROC].parentPID);
	int unblockStatus = unblockProc(PTable[curProc % MAXPROC].parentPID);
	//USLOSS_Console("unblock status: %d\n", unblockStatus);
	// run dispatcher
	int parentID = PTable[curProc % MAXPROC].parentPID;
	int parentPriority = PTable[parentID % MAXPROC].priority;
	//addToQueue(parentPriority, parentID);
	
	//USLOSS_Console("(1) cur proc in quit is %d\n", curProc);
	//dumpProcesses();
	//dispatcher();
	
	//USLOSS_Console("cur proc in quit is %d\n", curProc);

	//USLOSS_Console("Exiting quit...\n");
	USLOSS_PsrSet(oldPsr);
		
}

//
// This function unblocks the process given by 'pid' argument
int unblockProc(int pid) {
	unsigned int oldPsr = check_kernel();
	int curStatus = PTable[(pid % MAXPROC)].status;
	if ((curStatus == 0) || ((curStatus >= 1) && (curStatus <= 10)) || (PTable[(pid % MAXPROC)].PID == 0)) { // <10 correct?
		dispatcher();
		return -2;
	} else {
		PTable[(pid % MAXPROC)].status = 0;
		addToQueue(PTable[pid].priority, pid);
		dispatcher();
		return 0;
	}
	USLOSS_PsrSet(oldPsr);
}

// This function dumps the process table and is useful for debugging
void dumpProcesses(void) {
	unsigned int oldPsr = check_kernel();
	printf("Dumping Processes:\n");
	printf("%-10s\t %-4s\t %-4s\t %-10s\t %-8s\t %-8s\t %-8s \n", "NAME", "PID", "PARENT PID", "PRIORITY", "STATUS", "CONTEXT P", "STATE");
	for (int i = 0; i<MAXPROC; i++) {
		printf("%10s\t %4d\t %4d\t %10d\t %8d\t %12p\t %8d\n", PTable[i].name, PTable[i].PID, PTable[i].parentPID, PTable[i].priority, PTable[i].status, &PTable[i].context, PTable[i].state);
	}	
	USLOSS_PsrSet(oldPsr);
}








