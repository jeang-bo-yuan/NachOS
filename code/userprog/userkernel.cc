// userkernel.cc 
//	Initialization and cleanup routines for the version of the
//	Nachos kernel that supports running user programs.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchconsole.h"
#include "userkernel.h"
#include "synchdisk.h"
#include "threadArrive.h"

//----------------------------------------------------------------------
// UserProgKernel::UserProgKernel
// 	Interpret command line arguments in order to determine flags 
//	for the initialization (see also comments in main.cc)  
//----------------------------------------------------------------------

UserProgKernel::UserProgKernel(int argc, char **argv) 
		: ThreadedKernel(argc, argv)
{
    debugUserProg = FALSE;
	execfileNum=0;
	int prioNum = 0; // how many `-prio` has been parsed
	int burstNum = 0; // how many `-burst` has been parsed
	int arrivNum = 0; // how many `-arriv` has been parsed

    for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-s") == 0) {
			debugUserProg = TRUE;
		}
		else if (strcmp(argv[i], "-e") == 0) {
			execfile[++execfileNum]= argv[i + 1];
		}
		else if (strcmp(argv[i], "-prio") == 0) {
			_priority[++prioNum] = atoi(argv[i + 1]);
		}
		else if (strcmp(argv[i], "-burst") == 0) {
			_burst[++burstNum] = atoi(argv[i + 1]);
		}
		else if (strcmp(argv[i], "-arriv") == 0) {
			_arrive[++arrivNum] = atoi(argv[i + 1]);
		}
		else if (strcmp(argv[i], "-u") == 0) {
			cout << "===========The following argument is defined in userkernel.cc" << endl;
			cout << "Partial usage: nachos [-s]\n";
			cout << "Partial usage: nachos [-u]" << endl;
			cout << "Partial usage: nachos [-e] filename [-prio] priority [-burst] burstTime [-arriv] arriveTime" << endl;
			cout << "\tNote: -arriv only take effects when -sche SRTF is given" << endl;
		}
		else if (strcmp(argv[i], "-h") == 0) {
			cout << "argument 's' is for debugging. Machine status  will be printed " << endl;
			cout << "argument 'e' is for execting file." << endl;
			cout << "atgument 'u' will print all argument usage." << endl;
			cout << "For example:" << endl;
			cout << "	./nachos -s : Print machine status during the machine is on." << endl;
			cout << "	./nachos -e file1 -e file2 : executing file1 and file2."  << endl;
		}
    }
}

//----------------------------------------------------------------------
// UserProgKernel::Initialize
// 	Initialize Nachos global data structures.
//----------------------------------------------------------------------

void
UserProgKernel::Initialize()
{
    ThreadedKernel::Initialize();	// init multithreading

    machine = new Machine(debugUserProg);
    fileSystem = new FileSystem();
#ifdef FILESYS
    synchDisk = new SynchDisk("New SynchDisk");
#endif // FILESYS
}

//----------------------------------------------------------------------
// UserProgKernel::~UserProgKernel
// 	Nachos is halting.  De-allocate global data structures.
//	Automatically calls destructor on base class.
//----------------------------------------------------------------------

UserProgKernel::~UserProgKernel()
{
    delete fileSystem;
    delete machine;
#ifdef FILESYS
    delete synchDisk;
#endif
}


// Execute the program thread
void
ForkExecute(Thread *t)
{
	t->space->Execute(t->getName());
}

//----------------------------------------------------------------------
// UserProgKernel::Run
// 	Run the Nachos kernel.  For now, just run the "halt" program. 
//----------------------------------------------------------------------
void
UserProgKernel::Run()
{

	cout << "Total threads number is " << execfileNum << endl;
	for (int n = 1 ; n <= execfileNum ; n++)
	{
		t[n] = new Thread(execfile[n]);
		t[n]->space = new AddrSpace();
		t[n]->setPriority(_priority[n]);
		t[n]->setBurstTime(_burst[n] * TimerTicks);

		// In SRTF, simulate the arrival of thread
		if (scheduler->getSchedulerType() == SRTF) {
			ThreadArrive::makeThreadArrive(t[n], _arrive[n], (VoidFunctionPtr) &ForkExecute, (void *)t[n]);
		}
		else {
			// Fork and put into readyList
			t[n]->Fork((VoidFunctionPtr) &ForkExecute, (void *)t[n]);

			cout << "Thread " << execfile[n] << " is executing." << endl;
		}
	}
//	Thread *t1 = new Thread(execfile[1]);
//	Thread *t1 = new Thread("../test/test1");
//	Thread *t2 = new Thread("../test/test2");

//    AddrSpace *halt = new AddrSpace();
//	t1->space = new AddrSpace();
//	t2->space = new AddrSpace();

//    halt->Execute("../test/halt");
//	t1->Fork((VoidFunctionPtr) &ForkExecute, (void *)t1);
//	t2->Fork((VoidFunctionPtr) &ForkExecute, (void *)t2);
    ThreadedKernel::Run();
//	cout << "after ThreadedKernel:Run();" << endl;	// unreachable
}

//----------------------------------------------------------------------
// UserProgKernel::SelfTest
//      Test whether this module is working.
//----------------------------------------------------------------------

void
UserProgKernel::SelfTest() {
/*    char ch;

    ThreadedKernel::SelfTest();

    // test out the console device

    cout << "Testing the console device.\n" 
	<< "Typed characters will be echoed, until q is typed.\n"
    	<< "Note newlines are needed to flush input through UNIX.\n";
    cout.flush();

    SynchConsoleInput *input = new SynchConsoleInput(NULL);
    SynchConsoleOutput *output = new SynchConsoleOutput(NULL);

    do {
    	ch = input->GetChar();
    	output->PutChar(ch);   // echo it!
    } while (ch != 'q');

    cout << "\n";

    // self test for running user programs is to run the halt program above
*/




//	cout << "This is self test message from UserProgKernel\n" ;
}
