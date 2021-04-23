/*! \file pagefaultmanager.cc
Routines for the page fault managerPage Fault Manager
*/
//
//  Copyright (c) 1999-2000 INSA de Rennes.
//  All rights reserved.
//  See copyright_insa.h for copyright notice and limitation
//  of liability and disclaimer of warranty provisions.
//

#include "kernel/thread.h"
#include "vm/swapManager.h"
#include "vm/physMem.h"
#include "vm/pagefaultmanager.h"

PageFaultManager::PageFaultManager() {
}

// PageFaultManager::~PageFaultManager()
/*! Nothing for now
*/
PageFaultManager::~PageFaultManager() {
}

// ExceptionType PageFault(uint32_t virtualPage)
/*!
//	This method is called by the Memory Management Unit when there is a
//      page fault. This method loads the page from :
//      - read-only sections (text,rodata) $\Rightarrow$ executive
//        file
//      - read/write sections (data,...) $\Rightarrow$ executive
//        file (1st time only), or swap file
//      - anonymous mappings (stack/bss) $\Rightarrow$ new
//        page from the MemoryManager (1st time only), or swap file
//
//	\param virtualPage the virtual page subject to the page fault
//	  (supposed to be between 0 and the
//        size of the address space, and supposed to correspond to a
//        page mapped to something [code/data/bss/...])
//	\return the exception (generally the NO_EXCEPTION constant)
*/
ExceptionType PageFaultManager::PageFault(uint32_t virtualPage) {
#ifndef ETUDIANTS_TP
    printf("**** Warning: page fault manager is not implemented yet\n");
    exit(-1);
    return ((ExceptionType)0);
#endif
#ifdef ETUDIANTS_TP
    /* We're handling a page fault, aka the V bit in the translation table wasn't set.
       This method needs to access the current thread's translation table.
       Then:
       - Look for a free page in the physical page table
       - Load the missing page from the disk:
          + If it's in the swap, load it from there and clear Swap bit
          + If not, then the appropriate disk address is stored in the translation table.
            If this address is -1, then fill the page with 0
       - Set all the appropriate info in the translation table

       Concerning the greatest source of frustration, multi-threading:
       - Another thread can start while we're handling a page fault with a disk IO.
         If this thread asks for the same page, then it must yield to another thread for now
       - If a thread asks for a page, while it was being written into the swap,
         then this thread must yield (there's probably a more efficient way of doing this)
    */
    Process* process = g_current_thread->GetProcessOwner();
    OpenFile* exec_file = process->exec_file;
    TranslationTable* translation_table = process->addrspace->translationTable;

    // waiting until the page isn't used for a disk IO, then set the bit
    while (translation_table->getBitIo(virtualPage)) {
        g_current_thread->Yield();
    }
    ASSERT(translation_table->getBitIo(virtualPage) == 0);
    translation_table->setBitIo(virtualPage);

    // get a page in physical memory, halt if there isn't enough space
    // don't forget to unlock the page at the end of the page fault handler!
    int pp = g_physical_mem_manager->AddPhysicalToVirtualMapping(process->addrspace, virtualPage);
    if (pp == -1) {
        printf("Not enough free space to load program %s\n", exec_file->GetName());
        g_machine->interrupt->Halt(-1);
    }

    // check if the page is in the swap
    if (translation_table->getBitSwap(virtualPage)) {
        DEBUG('v', "Page #%d is in swap at sector #%d.\n", virtualPage,
            translation_table->getAddrDisk(virtualPage));
        // wait for the completion of the swap write
        while (translation_table->getAddrDisk(virtualPage) == -1) {
            g_current_thread->Yield();
        }
        g_swap_manager->GetPageSwap(
            translation_table->getAddrDisk(virtualPage),
            (char*) g_machine->mainMemory + pp*g_cfg->PageSize);
        // release the swap sector
        g_swap_manager->ReleasePageSwap(translation_table->getAddrDisk(virtualPage));
        translation_table->clearBitSwap(virtualPage);
    } else {
        if (translation_table->getAddrDisk(virtualPage) != -1) {
            DEBUG('v', "Page #%d is in exec file.\n", virtualPage);
            exec_file->ReadAt(
                (char*) g_machine->mainMemory + pp*g_cfg->PageSize,
                g_cfg->PageSize,
                translation_table->getAddrDisk(virtualPage));
        } else {
            DEBUG('v', "Page #%d is an anonymous section.\n", virtualPage);
            memset(g_machine->mainMemory + pp*g_cfg->PageSize,0, g_cfg->PageSize);
        }
    }

    // set the fields in the virtual page entry
    translation_table->clearBitIo(virtualPage);
    translation_table->setBitValid(virtualPage);
    translation_table->setPhysicalPage(virtualPage, pp);

    // unlock the page, ready to be used
    g_physical_mem_manager->UnlockPage((int)pp);

    return NO_EXCEPTION;
#endif
}
