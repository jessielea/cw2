/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

/* Since we *know* there will be 2 processes, stemming from the 2 user
 * programs, we can
 *
 * - allocate a fixed-size process table (of PCBs), and then maintain
 *   an index into it for the currently executing process,
 * - employ a fixed-case of round-robin scheduling: no more processes
 *   can be created, and neither is able to terminate.
 */

pcb_t pcb[ 30 ]; // Vary the number of programs being run
int n = 1;
int executing = 0;

// Array of shared memory regions
shrm_t shrms[maxShrm];
int numberOfShrms = 0;

void round_robin_scheduler( ctx_t* ctx ) {

    // Round robin scheduler - starts at 0 and increases pcb index by 1 using executingNext
    int executingNext = (executing + 1)%n;
    memcpy( &pcb[ executing ].ctx, ctx, sizeof( ctx_t ) );      // preserve P_1
    pcb[ executing ].status = STATUS_READY;                     // update   P_1 status
    memcpy( ctx, &pcb[ executingNext ].ctx, sizeof( ctx_t ) );  // restore  P_2
    pcb[ executingNext ].status = STATUS_EXECUTING;             // update   P_2 status
    executing = executingNext;                                  // update   index => P_2

  return;
}

//Find next process to execute
int nextProcess() {

  for (int i=1; i < n; i++) {
    if (pcb[(executing+i)%n].status == STATUS_READY) {
      return (executing+i)%n;
    }
  }
  return executing;
}

//Preserve the currently executing process and update its status
void saveProcess(ctx_t* ctx) {
  memcpy( &pcb[ executing ].ctx, ctx, sizeof( ctx_t ) );
  pcb[ executing ].status = STATUS_READY;
}

//Restore the next process and update its status
void updateProcess(int executingNext, ctx_t* ctx) {
  memcpy( ctx, &pcb[ executingNext ].ctx, sizeof( ctx_t ) );
  pcb[ executingNext ].status = STATUS_EXECUTING;
  executing = executingNext;
}

void priority_scheduler( ctx_t* ctx ) {

    //If age = priority then schedule the next process, otherwise continue executing current process.
    if ( pcb[ executing ].age == pcb[ executing ].basePriority) {
      pcb[ executing ].age = 0;

      int executingNext = nextProcess();
      saveProcess(ctx);
      updateProcess(executingNext, ctx);

      return;
    }
    else {
      pcb[ executing ].age = pcb[ executing ].age + 1;
    }
}


extern void     main_P3();
extern uint32_t tos_P3;
extern void     main_P4();
extern uint32_t tos_P4;
extern void     main_Ptemp();
extern uint32_t tos_Ptemp;
extern void     main_P5();
extern uint32_t tos_P5;
extern void     main_console();
extern uint32_t tos_console;
extern uint32_t tos_newProcesses;
extern void     main_phil();
extern uint32_t tos_shrm;



void hilevel_handler_rst(ctx_t* ctx) {
  /* Configure the mechanism for interrupt handling by
   *
   * - configuring timer st. it raises a (periodic) interrupt for each
   *   timer tick,
   * - configuring GIC st. the selected interrupts are forwarded to the
   *   processor via the IRQ interrupt signal, then
   * - enabling IRQ interrupts.
   */

  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor


    /* Initialise PCBs representing processes stemming from execution of
   * the two user programs.  Note in each case that
   *
   * - the CPSR value of 0x50 means the processor is switched into USR
   *   mode, with IRQ interrupts enabled, and
   * - the PC and SP values matche the entry point and top of stack.
   */

  memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );
  pcb[ 0 ].pid      = 1;
  pcb[ 0 ].status   = STATUS_READY;
  pcb[ 0 ].ctx.cpsr = 0x50;
  pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_console );
  pcb[ 0 ].ctx.sp   = ( uint32_t )( &tos_console );
  pcb[ 0 ].basePriority = 0;
  pcb[ 0 ].age = 0;

  /* Once the PCBs are initialised, we (arbitrarily) select one to be
   * restored (i.e., executed) when the function then returns.
   */
  PL011_putc( UART0, 'R', true );

  memcpy( ctx, &pcb[ 0 ].ctx, sizeof( ctx_t ) );
  pcb[ 0 ].status = STATUS_EXECUTING;
  executing = 0;

  int_enable_irq();

  return;
}

void hilevel_handler_irq(ctx_t* ctx) {
  // Step 2: read  the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  if( id == GIC_SOURCE_TIMER0 ) {
    PL011_putc( UART0, 'T', true );
    TIMER0->Timer1IntClr = 0x01;
    priority_scheduler(ctx);
  }

  // Step 5: write the interrupt identifier to signal we're done.

  GICC0->EOIR = id;

  return;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {
  /* Based on the identified encoded as an immediate operand in the
   * instruction,
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call,
   * - write any return value back to preserved usr mode registers.
   */

  switch( id ) {

    case 0x00 : { // 0x00 => yield()
      priority_scheduler( ctx );
      break;
    }

    case 0x01 : { // 0x01 => write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++, true );
      }

      ctx->gpr[ 0 ] = n;
      break;
    }

    case 0x02 : { //read
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        *x = PL011_getc( UART0 , true );
        x++;
      }

      PL011_putc( UART0, 'x', true );

      ctx->gpr[ 0 ] = n;
      break;
    }

    case 0x03 : { //fork

      pcb_t* parent = &pcb[executing];
      pcb_t* child = &pcb[ n ];            //n is equivalent to the no. of processes + 1

      memcpy( &parent->ctx, ctx, sizeof( ctx_t ) );

      memset( child, 0, sizeof(pcb_t));

      memcpy( child, parent, sizeof(pcb_t));
      child->pid = n + 1;

      uint32_t parentTos = (uint32_t) &tos_newProcesses-(executing*0x00001000);
      int offset = (uint32_t) parentTos - parent->ctx.sp;

      uint32_t childTos = (uint32_t) &tos_newProcesses-((n)*0x00001000);
      memcpy((void *) childTos - 0x00001000, (void *) parentTos - 0x00001000, 0x00001000 );
      child->ctx.sp = (uint32_t) childTos - offset;

      child->status = STATUS_READY;

      ctx->gpr[ 0 ] = child->pid;
      child->ctx.gpr[ 0 ] = 0;
      n++;

      break;
    }

    case 0x04 : { //exit

      memset( &pcb[ executing ], 0, sizeof( pcb_t ) );
      pcb[ executing ].status = STATUS_TERMINATED;

      //Equivalent to calling the scheduler except I *don't* want to save the process
      int executingNext = nextProcess();
      updateProcess(executingNext, ctx);

      break;
    }

     case 0x05 : { //exec

       PL011_putc( UART0, 'E', true );

       // Replace current process with new process i.e. the argument to exec is an address.
       ctx->pc = ctx->gpr[0];
       ctx->sp = (uint32_t) &tos_newProcesses-(executing*0x00001000);

       break;
     }

     case 0x06 : { //kill

      int pid = ctx->gpr[0];
      for (int i=0;i<n;i++) {
        if (pcb[i].pid == pid) {
          PL011_putc( UART0, 'K', true );
          memset( &pcb[ i ], 0, sizeof( pcb_t ) );
          pcb[ i ].status = STATUS_TERMINATED;
          break;
        }
      }
      break;
     }

     case 0x08 : { //SHMGET(int x)

       //id is the philospher number
       uint32_t id = (uint32_t) ctx->gpr[0];

       int target = -1;

       for( int i = 0; i < maxShrm; i++) {
         if ( shrms[i].shmid == id ) {
           target = i;
         }
       }

       //If the region of shared memory doesn't exist yet
       if (target == -1) {
         target = numberOfShrms; //numberOfShrms equivalent to the next slot in array
         shrms[target].shmid = id;
         shrms[target].tos = (void*) &tos_shrm - (numberOfShrms*1000);
         shrms[target].lock = false;

         numberOfShrms++;
       }
       else {
         if (shrms[target].lock) {
           priority_scheduler(ctx); //No luck! Fork is taken, carry on.
         }
       }

       shrms[target].lock = true;
       ctx->gpr[0] = (uint32_t) shrms[target].tos; //Return the position in shared memory (not actually being used)

       break;
     }


     case 0x09 : { //SHMDT
       uint32_t id = (uint32_t) ctx->gpr[0];

       int target = -1;

       for( int i = 0; i < maxShrm; i++) {
         if ( shrms[i].shmid == id ) {
           target = i;
         }
       }

       if (target == -1) break;

       shrms[target].lock = false;

       break;
     }

    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
