// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include <string.h>
#include <list>

#define UserStackSize		1024 	// increase this as necessary!

class AddrSpace {
  public:
    AddrSpace();			// Create an address space.
    ~AddrSpace();			// De-allocate an address space

    /// 檢查第 index 個 physical page 是否被使用
    /// Pre: `0 <= index && index < NumPhysPages`
    static bool IsPhyPageUsed(size_t index);

    /// 讓 entry 代表的 virtual page 使用第 index 個 physical page
    /// Pre: `!IsPhyPageUsed(phyPage)`
    /// Post: valid == true, use == false, dirty == false
    static void UseFreePhyPage(size_t phyPage, TranslationEntry* entry);

    static unsigned int SwapOutLastPage();

    void Execute(char *fileName);	// Run the the program
					// stored in the file "executable"

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 

  private:
    /// record which physical pages are used
    static bool usedPhyPage[NumPhysPages];

    // record which pages are in the memory
    static std::list<TranslationEntry*> pageList;

    /// Assume linear page table translation for now!
    TranslationEntry *pageTable;
    /// Number of pages in the virtual address space
    unsigned int numPages;

    bool Load(char *fileName);		// Load the program into memory
					// return false if not found

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

};

#endif // ADDRSPACE_H
