// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"

namespace {
/**
 * 這個類別是一個Noff（Nachos Object Code Format）的讀取器
 * 它的用途是從object file中讀取特定Segment
 * 
 * 該讀取器會依序讀取virtual address [0, PageSize)、[PageSize, 2 * PageSize)...的內容
 * 若讀取範圍和被分配到的Segment有重疊，則會從檔案讀取並寫入outData
 */
class NoffReader {
    OpenFile* const FILE;
    const char* name;
    int virtualAddr;		/* location of segment in virt addr space */
    int inFileAddr;		/* location of segment in this file */
    int size;			/* size of segment */

    public:
    NoffReader(OpenFile* file, Segment S, const char* name)
        : FILE(file), name(name), virtualAddr(S.virtualAddr), inFileAddr(S.inFileAddr), size(S.size)
    {}

    /// 讀出一個Page，如果讀取範圍和分配到的Segment重疊，則讀取並回傳true。否則回傳false。
    bool ReadOnePage(char* outData) {
        // read nothing
        if (virtualAddr < 0 || virtualAddr >= (int)PageSize || size <= 0) {
            virtualAddr -= PageSize;
            return false;
        }

        const int size_to_read = min<int>(PageSize - virtualAddr, size);
        DEBUG(dbgMy, "Read " << size_to_read << " bytes from inFileAddr: " << inFileAddr << " (Segment: " << name << ")");
        FILE->ReadAt(outData, size_to_read, inFileAddr);

        // update
        // 下次讀取時，直接寫在outData的最前方（因為Segment是連續的，如果跨多個Page，則在橫跨的第2個page中一定是從頭開始寫）
        virtualAddr = 0;
        size -= size_to_read;
        inFileAddr += size_to_read;

        return true;
    }
};
}


// Static Member of AddrSpace ///////////////////////////////////////////////////////////////////////

bool AddrSpace::usedPhyPage[NumPhysPages] = {0};
std::list<TranslationEntry*> AddrSpace::pageList;

bool AddrSpace::IsPhyPageUsed(size_t index)
{
    ASSERT(0 <= index && index < NumPhysPages);
    return usedPhyPage[index];
}

unsigned int AddrSpace::SwapOutLastPage()
{
    TranslationEntry* curPage = pageList.back();
    /**
     * pop_back放在這裡是必要的，因為在SwapOut時，NachOS會讓目前的執行緒（下稱A）休息直到硬碟的寫入完成
     * 在休息的期間可能會切換到其他的執行緒（下稱B）
     * 若B也發生page fault，那B看到的pageList.back()必須和A看到的不一樣，不然同個physical page會被swap out兩次
     */
    pageList.pop_back();

    curPage->SwapOut();
    curPage->valid = false;
    // 在swap out完成前必須持續佔用curPage->physicalPage，避免其他thread想要佔用它
    usedPhyPage[curPage->physicalPage] = false;
    
    return curPage->physicalPage;
}

std::list<TranslationEntry*>* AddrSpace::getPageList()
{
    return &pageList;
}

void AddrSpace::UseFreePhyPage(size_t phyPage, TranslationEntry *entry)
{
    ASSERT(!AddrSpace::IsPhyPageUsed(phyPage));
    ASSERT(pageList.size() < NumPhysPages);

    // mark as used
    AddrSpace::usedPhyPage[phyPage] = true;

    // edit entry
    entry->physicalPage = phyPage;
    entry->valid = true;
    entry->use = false;
    entry->dirty = false;

    // TODO: Add entry into "page list"
    pageList.push_front(entry);
    // TODO: Swap in entry if needed
    entry->SwapIn();
}

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------


AddrSpace::AddrSpace()
{
    pageTable = nullptr;
    numPages = 0;
    
    // zero out the entire address space
//    bzero(kernel->machine->mainMemory, MemorySize);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   for(int i = 0; i < numPages; i++)
        AddrSpace::usedPhyPage[pageTable[i].physicalPage] = false;
   delete pageTable;
}


//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool 
AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;

    if (executable == NULL) {
	cerr << "Unable to open file " << fileName << "\n";
	return FALSE;
    }
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
//	cout << "number of pages of " << fileName<< " is "<<numPages<<endl;
    size = numPages * PageSize;

    numPages = divRoundUp(size,PageSize);
    delete pageTable; // prevent memory leak, if someone loads the file twice
    pageTable = new TranslationEntry[numPages];

    // 將pageTable和numPages傳給kernel->machine，避免它會用到
    kernel->machine->pageTable = this->pageTable;
    kernel->machine->pageTableSize = this->numPages;

    for(unsigned int i=0, j=0; i<numPages; i++){
        pageTable[i].virtualPage = i;
        while(j<NumPhysPages && AddrSpace::IsPhyPageUsed(j))
            j++;

        // there is an empty physical page
        if (j < NumPhysPages) {
            AddrSpace::UseFreePhyPage(j, pageTable + i);
        }
        // no empty physical page
        else {
            pageTable[i].physicalPage = 0;
            // Invalid, PageFault will occur when reading or writing
            pageTable[i].valid = false;
        }
        
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;
    }

    this->InitPages(executable, noffH);

    delete executable;			// close file
    return TRUE;			// success
}

void AddrSpace::InitPages(OpenFile* executable, const noffHeader& noffH) {
    DEBUG(dbgMy, "Initializing address space: " << numPages << ", " << numPages * PageSize);
    DEBUG(dbgMy, "Initializing code segment: " << noffH.code.virtualAddr << ", " << noffH.code.size << ", " << noffH.code.inFileAddr);
    DEBUG(dbgMy, "Initializing data segment: " << noffH.initData.virtualAddr << ", " << noffH.initData.size << ", " << noffH.initData.inFileAddr);

    // 如果沒有對應的physical page，則將內容暫存於此
    char buffer[PageSize] = { '\0' };
    char* outData = nullptr;
    NoffReader code(executable, noffH.code, "code"), initData(executable, noffH.initData, "initData");

    for (unsigned i = 0; i < numPages; i++) {
        // 有對應的physical page，直接寫入mainMemory
        if (pageTable[i].valid) {
            DEBUG(dbgMy, i << " valid");
            outData = kernel->machine->mainMemory + (pageTable[i].physicalPage * PageSize);
        }
        else {
            DEBUG(dbgMy, i << " invalid");
            outData = buffer;
            memset(buffer, 0, PageSize);
        }

        code.ReadOnePage(outData);
        initData.ReadOnePage(outData);

        if (!pageTable[i].valid) {
            pageTable[i].SwapOutData(outData);
            ASSERT(!pageTable[i].valid);
        }
    }
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program.  Load the executable into memory, then
//	(for now) use our own thread to run it.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

void 
AddrSpace::Execute(char *fileName) 
{
    if (!Load(fileName)) {
	cout << "inside !Load(FileName)" << endl;
	return;				// executable not found
    }

    //kernel->currentThread->space = this;
    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register

    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
        pageTable=kernel->machine->pageTable;
        numPages=kernel->machine->pageTableSize;
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}
