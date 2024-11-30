// translate.h 
//	Data structures for managing the translation from 
//	virtual page # -> physical page #, used for managing
//	physical memory on behalf of user programs.
//
//	The data structures in this file are "dual-use" - they
//	serve both as a page table entry, and as an entry in
//	a software-managed translation lookaside buffer (TLB).
//	Either way, each entry is of the form:
//	<virtual page #, physical page #>.
//
// DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef TLB_H
#define TLB_H

#include "copyright.h"
#include "utility.h"
#include <bitset>
#include <memory>

class SynchDisk;

// The following class defines an entry in a translation table -- either
// in a page table or a TLB.  Each entry defines a mapping from one 
// virtual page to one physical page.
// In addition, there are some extra bits for access control (valid and 
// read-only) and some bits for usage information (use and dirty).

class TranslationEntry {
  public:
    TranslationEntry();
    ~TranslationEntry();

    // from SwapSpace to MainMemory
    void SwapIn();

    // from MainMemory to SwapSpace
    void SwapOut();

    // LRU implement
    static void LRU_Algo(TranslationEntry* entry);

    unsigned int virtualPage;  	// The page number in virtual memory.
    unsigned int physicalPage;  // The page number in real memory (relative to the
			//  start of "mainMemory"
    bool valid;         // If this bit is set, the translation is ignored.
			// (In other words, the entry hasn't been initialized.)
    bool readOnly;	// If this bit is set, the user program is not allowed
			// to modify the contents of the page.
    bool use;           // This bit is set by the hardware every time the
			// page is referenced or modified.
    bool dirty;         // This bit is set by the hardware every time the
			// page is modified.

  private:
    /// @brief swap out時會將記憶體的內容暫存到哪個sector
    unsigned m_sector_number;
    /// @brief 之前有沒有swap out過，有的話swap in才會做事（避免硬碟中的亂碼被swap in）
    bool m_has_swapped_out_before;

    /// @brief 記錄sector是否被使用
    static std::bitset<1024> _IsSectorUsed;
    /// @brief It may be empty
    static unsigned _EmptySector;
    /// @brief 置換空間
    static std::shared_ptr<SynchDisk> _SwapSpace;
};

#endif
