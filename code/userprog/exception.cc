// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
	int	type = kernel->machine->ReadRegister(2);
	int	val, val2;

    switch (which) {
	case SyscallException:
	    switch(type) {
		case SC_Halt:
		    DEBUG(dbgAddr, "Shutdown, initiated by user program.\n");
   		    kernel->interrupt->Halt();
		    break;
		case SC_PrintInt:
			val=kernel->machine->ReadRegister(4);
			cout << "Print integer:" <<val << endl;
			return;
/*		case SC_Exec:
			DEBUG(dbgAddr, "Exec\n");
			val = kernel->machine->ReadRegister(4);
			kernel->StringCopy(tmpStr, retVal, 1024);
			cout << "Exec: " << val << endl;
			val = kernel->Exec(val);
			kernel->machine->WriteRegister(2, val);
			return;
*/		case SC_Exit:
			DEBUG(dbgAddr, "Program exit\n");
			val=kernel->machine->ReadRegister(4);
			cout << "return value:" << val << endl;
			kernel->currentThread->Finish();
			break;
		case SC_Sleep:
			val = kernel->machine->ReadRegister(4);
			std::cout << "Sleep Time: " << val << "(ms)" << std::endl;
			kernel->alarm->WaitUntil(val);
			return;

		case SC_Add:
			val = kernel->machine->ReadRegister(4);
			val2 = kernel->machine->ReadRegister(5);
			// write result into r2
			kernel->machine->WriteRegister(2, val + val2);
			return;

		case SC_Sub:
			val = kernel->machine->ReadRegister(4);
			val2 = kernel->machine->ReadRegister(5);
			kernel->machine->WriteRegister(2, val - val2);
			return;

		case SC_Mul:
			val = kernel->machine->ReadRegister(4);
			val2 = kernel->machine->ReadRegister(5);
			kernel->machine->WriteRegister(2, val * val2);
			return;

		case SC_Div:
			val = kernel->machine->ReadRegister(4);
			val2 = kernel->machine->ReadRegister(5);
			if (val2 == 0) {
				cout << "Error: Divide by zero" << endl;
				kernel->machine->WriteRegister(2, 11132021);
			}
			else {
				kernel->machine->WriteRegister(2, val / val2);
			}
			return;

		case SC_Mod:
			val = kernel->machine->ReadRegister(4);
			val2 = kernel->machine->ReadRegister(5);
			if (val2 == 0) {
				cout << "Error: Modded by zero" << endl;
				kernel->machine->WriteRegister(2, 11132021);
			}
			else {
				kernel->machine->WriteRegister(2, val % val2);
			}
			return;

		case SC_Print: {
			const int landmine = (21 % 26) + 'a';
			// ptr = pointer of the string
			int ptr = kernel->machine->ReadRegister(4);
			int len = 0;

			// read 1 byte into val, then move ptr forward
			kernel->machine->ReadMem(ptr++, 1, &val);

			cout << "[B11132021_Print]";
			while (val != '\0') {
				++len;

				if (tolower(val) == landmine)
					cout.put('*');
				else
					cout.put(val);

				kernel->machine->ReadMem(ptr++, 1, &val);
			}

			// write back result
			kernel->machine->WriteRegister(2, len);
			return;
		}

		default:
		    cerr << "Unexpected system call " << type << "\n";
 		    break;
	    }
	    break;
	default:
	    cerr << "Unexpected user mode exception" << which << "\n";
	    break;
    }
    ASSERTNOTREACHED();
}
