//-----------------------------------------------------------------
/*! \file mem.cc
//  \brief Routines for the physical page management
*/
//
//  Copyright (c) 1999-2000 INSA de Rennes.
//  All rights reserved.
//  See copyright_insa.h for copyright notice and limitation
//  of liability and disclaimer of warranty provisions.
//-----------------------------------------------------------------

#include <unistd.h>
#include "vm/physMem.h"

//-----------------------------------------------------------------
// PhysicalMemManager::PhysicalMemManager
//
/*! Constructor. It simply clears all the page flags and inserts them in the
// free_page_list to indicate that the physical pages are free
*/
//-----------------------------------------------------------------
PhysicalMemManager::PhysicalMemManager() {

  long i;

  tpr = new struct tpr_c[g_cfg->NumPhysPages];

  for (i=0;i<g_cfg->NumPhysPages;i++) {
    tpr[i].free=true;
    tpr[i].locked=false;
    tpr[i].owner=NULL;
    free_page_list.Append((void*)i);
  }
  i_clock=-1;
}

PhysicalMemManager::~PhysicalMemManager() {
  // Empty free page list
  int64_t page;
  while (!free_page_list.IsEmpty()) page =  (int64_t)free_page_list.Remove();

  // Delete physical page table
  delete[] tpr;
}

//-----------------------------------------------------------------
// PhysicalMemManager::RemovePhysicalToVitualMapping
//
/*! This method releases an unused physical page by clearing the
//  corresponding bit in the page_flags bitmap structure, and adding
//  it in the free_page_list.
//
//  \param num_page is the number of the real page to free
*/
//-----------------------------------------------------------------
void PhysicalMemManager::RemovePhysicalToVirtualMapping(long num_page) {

  // Check that the page is not already free
  ASSERT(!tpr[num_page].free);

  // Update the physical page table entry
  tpr[num_page].free=true;
  tpr[num_page].locked=false;
  if (tpr[num_page].owner->translationTable!=NULL)
    tpr[num_page].owner->translationTable->clearBitValid(tpr[num_page].virtualPage);

  // Insert the page in the free list
  free_page_list.Prepend((void*)num_page);
}

//-----------------------------------------------------------------
// PhysicalMemManager::UnlockPage
//
/*! This method unlocks the page numPage, after
//  checking the page is in the locked state. Used
//  by the page fault manager to unlock at the
//  end of a page fault (the page cannot be evicted until
//  the page fault handler terminates).
//
//  \param num_page is the number of the real page to unlock
*/
//-----------------------------------------------------------------
void PhysicalMemManager::UnlockPage(long num_page) {
  ASSERT(num_page<g_cfg->NumPhysPages);
  ASSERT(tpr[num_page].locked==true);
  ASSERT(tpr[num_page].free==false);
  tpr[num_page].locked = false;
}

//-----------------------------------------------------------------
// PhysicalMemManager::ChangeOwner
//
/*! Change the owner of a page
//
//  \param owner is a pointer on new owner (Thread *)
//  \param numPage is the concerned page
*/
//-----------------------------------------------------------------
void PhysicalMemManager::ChangeOwner(long numPage, Thread* owner) {
  // Update statistics
  g_current_thread->GetProcessOwner()->stat->incrMemoryAccess();
  // Change the page owner
  tpr[numPage].owner = owner->GetProcessOwner()->addrspace;
}

//-----------------------------------------------------------------
// PhysicalMemManager::AddPhysicalToVirtualMapping
//
/*! This method returns a new physical page number. If there is no
//  page available, it evicts one page (page replacement algorithm).
//
//  NB: this method locks the newly allocated physical page such that
//      it is not stolen during the page fault resolution. Don't forget
//      to unlock it
//
//  \param owner address space (for backlink)
//  \param virtualPage is the number of virtualPage to link with physical page
//  \return A new physical page number.
*/
//-----------------------------------------------------------------
int PhysicalMemManager::AddPhysicalToVirtualMapping(AddrSpace* owner,int virtualPage) {
#ifndef ETUDIANTS_TP
    printf("**** Warning: function AddPhysicalToVirtualMapping is not implemented\n");
    exit(-1);
    return (0);
#endif
#ifdef ETUDIANTS_TP
    /* We're binding a new virtual to a physical one.
       If a physical page is free : set the right fields in the TPR entry
       If not :
       - Find an unused physical page using the clock algorithm
       - Lock the page and check the Modified bit : copy the page in a new
         swap sector if set, then set the Swap bit
       - Clear the V bit in the evicted virtual page entry
       - Set the right fields in the TPR entry
    */

    // find a free page
    int pp = FindFreePage();
    // no free page found, evict one
    if (pp == -1) {
        pp = EvictPage();
        if (pp == -1) {
            // normally this shouldn't happen : clock algorithm loops forever
            printf("Could not find free page or evict one. (Swap full ?)\n");
            return -1;
        }
        TranslationTable* prev_owner = tpr[pp].owner->translationTable;
        int prev_page = tpr[pp].virtualPage;

        // locking the page in case of nested page miss
        tpr[pp].locked = true;

        // previous page was modified, copy it on a swap sector
        if (prev_owner->getBitM(prev_page)) {
            prev_owner->setBitSwap(prev_page);
            prev_owner->setAddrDisk(prev_page, -1);
            // copy the page in the swap in a newly allocated sector
            int swap_sector = g_swap_manager->PutPageSwap(
                -1,   // let the swap manager choose and return a sector
                (char*)&(g_machine->mainMemory[pp*g_cfg->PageSize]));
            prev_owner->setAddrDisk(prev_page, swap_sector);
        }
        // invalidating previous owner entry
        prev_owner->setPhysicalPage(prev_page, -1);
        prev_owner->clearBitValid(prev_page);
        DEBUG('v', "Replacing page #%d in TPR[%d] with page #%d.\n", prev_page, pp, virtualPage);
    }

    // Update the physical page entry
    tpr[pp].free = false;
    tpr[pp].locked = true;
    tpr[pp].virtualPage = virtualPage;
    tpr[pp].owner = owner;

    return pp;
#endif
}

//-----------------------------------------------------------------
// PhysicalMemManager::FindFreePage
//
/*! This method returns a new physical page number, if it finds one
//  free. If not, return -1. Does not run the clock algorithm.
//
//  \return A new free physical page number.
*/
//-----------------------------------------------------------------
int PhysicalMemManager::FindFreePage() {
    int64_t page;
    // Check that the free list is not empty
    if (free_page_list.IsEmpty())
        return -1;

    // Update statistics
    g_current_thread->GetProcessOwner()->stat->incrMemoryAccess();
    // Get a page from the free list
    page = (int64_t)free_page_list.Remove();
    // Check that the page is really free
    ASSERT(tpr[page].free);
    // Update the physical page table
    tpr[page].free = false;

    return page;
}

//-----------------------------------------------------------------
// PhysicalMemManager::EvictPage
//
/*! This method implements page replacement, using the well-known
//  clock algorithm.
//
//  \return A new free physical page number.
*/
//-----------------------------------------------------------------
int PhysicalMemManager::EvictPage() {
#ifndef ETUDIANTS_TP
    printf("**** Warning: page replacement algorithm is not implemented yet\n");
    exit(-1);
    return (0);
#endif
#ifdef ETUDIANTS_TP
    // initialized the next page after the page chosen in a previous eviction
    int i = (i_clock+1)%g_cfg->NumPhysPages;
    int chosen_page = (i_clock+1)%g_cfg->NumPhysPages;

    while (true) {
        if (!tpr[i].locked && !tpr[i].owner->translationTable->getBitU(tpr[i].virtualPage)) {
            chosen_page = i;
            break;
        } else {
            tpr[i].owner->translationTable->clearBitU(tpr[i].virtualPage);
        }
        i = (i+1)%g_cfg->NumPhysPages;
    }

    i_clock = chosen_page;
    return chosen_page;

#endif
}

//-----------------------------------------------------------------
// PhysicalMemManager::Print
//
/*! print the current status of the table of physical pages
//
//  \param rpage number of real page
*/
//-----------------------------------------------------------------

void PhysicalMemManager::Print(void) {
  int i;

  printf("Contents of TPR (%d pages)\n",g_cfg->NumPhysPages);
  for (i=0;i<g_cfg->NumPhysPages;i++) {
    printf("Page %d free=%d locked=%d virtpage=%d owner=%lx U=%d M=%d\n",
	   i,
	   tpr[i].free,
	   tpr[i].locked,
	   tpr[i].virtualPage,
	   (long int)tpr[i].owner,
	   (tpr[i].owner!=NULL) ? tpr[i].owner->translationTable->getBitU(tpr[i].virtualPage) : 0,
	   (tpr[i].owner!=NULL) ? tpr[i].owner->translationTable->getBitM(tpr[i].virtualPage) : 0);
  }
}
