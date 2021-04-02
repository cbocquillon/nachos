/* \file drvACIA.cc
   \brief Routines of the ACIA device driver
//
//      The ACIA is an asynchronous device (requests return
//      immediately, and an interrupt happens later on).
//      This is a layer on top of the ACIA.
//      Two working modes are to be implemented in assignment 2:
//      a Busy Waiting mode and an Interrupt mode. The Busy Waiting
//      mode implements a synchronous IO whereas IOs are asynchronous
//      IOs are implemented in the Interrupt mode (see the Nachos
//      roadmap for further details).
//
//  Copyright (c) 1999-2000 INSA de Rennes.
//  All rights reserved.
//  See copyright_insa.h for copyright notice and limitation
//  of liability and disclaimer of warranty provisions.
//
*/

/* Includes */

#include "kernel/system.h"         // for the ACIA object
#include "kernel/synch.h"
#include "machine/ACIA.h"
#include "drivers/drvACIA.h"

//-------------------------------------------------------------------------
// DriverACIA::DriverACIA()
/*! Constructor.
  Initialize the ACIA driver.
  In the ACIA Interrupt mode,
  initialize the reception index and semaphores and allow
  reception interrupts.
  In the ACIA Busy Waiting mode, simply initialize the ACIA
  working mode and create the semaphore.
  */
//-------------------------------------------------------------------------

DriverACIA::DriverACIA() {
    if (g_cfg->ACIA == ACIA_INTERRUPT) {
        send_sema = new Semaphore("send_sema", 1);
        receive_sema = new Semaphore("receive_sema", 0);
        ind_send = 1;
        ind_rec = 0;
        g_machine->acia->SetWorkingMode(REC_INTERRUPT | SEND_INTERRUPT);
        DEBUG('i', "ACIA Driver initialized in INTERRUPT mode.\n");
    } else {

    }
}

//-------------------------------------------------------------------------
// DriverACIA::TtySend(char* buff)
/*! Routine to send a message through the ACIA (Busy Waiting or Interrupt mode)
  */
//-------------------------------------------------------------------------

int DriverACIA::TtySend(char* buff) {
    DEBUG('i', "Call to TtySend\n");
    send_sema->P();
    int i = 0;
    // copying the line into the emission buffer
    do {
        send_buffer[i] = buff[i];
        i++;
    } while (buff[i-1] != '\0');
    // sending the first character
    ind_send = 1;
    g_machine->acia->PutChar(send_buffer[0]);
}

//-------------------------------------------------------------------------
// DriverACIA::TtyReceive(char* buff,int length)
/*! Routine to reveive a message through the ACIA
//  (Busy Waiting and Interrupt mode).
  */
//-------------------------------------------------------------------------

int DriverACIA::TtyReceive(char* buff,int lg) {
    DEBUG('i', "Call to TtyReceive\n");
    receive_sema->P();
    int borne = min(lg, ind_rec);
    for (int i=0; i<borne ; i++) {
        buff[i] = receive_buffer[i];
    }
    //buff[borne-1] = '\0';
    ind_rec = 0;
}

//-------------------------------------------------------------------------
// DriverACIA::InterruptSend()
/*! Emission interrupt handler.
  Used in the ACIA Interrupt mode only.
  Detects when it's the end of the message (if so, releases the send_sema semaphore), else sends the next character according to index ind_send.
  */
//-------------------------------------------------------------------------

void DriverACIA::InterruptSend() {
  if (send_buffer[ind_send] != '\0') {
    ind_send++;
    g_machine->acia->PutChar(send_buffer[ind_send]);
  } else {
    send_sema->V();
  }
}

//-------------------------------------------------------------------------
// DriverACIA::Interrupt_receive()
/*! Reception interrupt handler.
  Used in the ACIA Interrupt mode only. Reveices a character through the ACIA.
  Releases the receive_sema semaphore and disables reception
  interrupts when the last character of the message is received
  (character '\0').
  */
//-------------------------------------------------------------------------

void DriverACIA::InterruptReceive() {
    receive_buffer[ind_rec] = g_machine->acia->GetChar();
    if (receive_buffer[ind_rec] == '\0') {
        receive_sema->V();
    } else {
        ind_rec++;
    }
}
