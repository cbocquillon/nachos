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
        - Load the missing page from the disk. The appropriate disk sector addr
          is stored in the translation table. If this address is -1, fill with 0
          (we'll worry about the swap later...)
        - Look for a free page in the physical page table
        - Set all the appropriate info in the tables
    */
    Process* process = g_current_thread->GetProcessOwner();
    OpenFile* exec_file = process->exec_file;
    TranslationTable* translation_table = process->addrspace->translationTable;
    // Get a page in physical memory, halt if there isn't enough space
    // Don't forget to unlock the page at the end of the page fault handler !
    int pp = g_physical_mem_manager->AddPhysicalToVirtualMapping(process->addrspace, virtualPage);
    if (pp == -1) {
        printf("Not enough free space to load program %s\n", exec_file->GetName());
        g_machine->interrupt->Halt(-1);
    }
    translation_table->setPhysicalPage(virtualPage, pp);

    if (translation_table->getAddrDisk(virtualPage) != -1) {
        exec_file->ReadAt(
            (char*)&(g_machine->mainMemory[pp*g_cfg->PageSize]),
            g_cfg->PageSize,
            translation_table->getAddrDisk(virtualPage));
    } else {
        memset(
            &(g_machine->mainMemory[pp*g_cfg->PageSize]),
            0, g_cfg->PageSize);
    }


    translation_table->setAddrDisk(virtualPage,-1);
    translation_table->setBitValid(virtualPage);
    g_physical_mem_manager->UnlockPage((int)pp);

    return NO_EXCEPTION;
#endif
}
