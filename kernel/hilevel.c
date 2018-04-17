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

pcb_t pcb[ 30 ]; // By changing the number you can vary the number of programs being run (1.b)
int n = 1;//sizeof(pcb)/sizeof(pcb[0]); // Get the size of pcb (divide the whole array b the size of each element)
int executing = 0;

void round_robin_scheduler( ctx_t* ctx ) {

    // Round robin scheduler - starts at 0 and increases pcb index by 1 using executingNext
    int executingNext = (executing + 1)%n; //check that the next one is ready
    memcpy( &pcb[ executing ].ctx, ctx, sizeof( ctx_t ) ); // preserve P_1
    pcb[ executing ].status = STATUS_READY;                // update   P_1 status
    memcpy( ctx, &pcb[ executingNext ].ctx, sizeof( ctx_t ) ); // restore  P_2
    pcb[ executingNext ].status = STATUS_EXECUTING;            // update   P_2 status
    executing = executingNext;                                 // update   index => P_2

    PL011_putc( UART0, executing+'0', true );
  return;
}

void priority_scheduler( ctx_t* ctx ) {

    //If age = priority then do the memcpy stuff and reset the age. If it doesnt then do nothting and just carry on.
    if ( pcb[ executing ].age == pcb[ executing ].basePriority) {

      int executingNext = 0;
      pcb[ executing ].age = 0;

      for (int i=(executing+1)%n; i < n; i++) {
        if (pcb[i].status == STATUS_READY) { //Find first AVAILABLE pcb
          executingNext = i;
          break;
        }
      }

      memcpy( &pcb[ executing ].ctx, ctx, sizeof( ctx_t ) ); // preserve executing
      pcb[ executing ].status = STATUS_READY;                // update executing's status
      memcpy( ctx, &pcb[ executingNext ].ctx, sizeof( ctx_t ) ); // restore next program
      pcb[ executingNext ].status = STATUS_EXECUTING;            // update next program's status
      executing = executingNext;

      PL011_putc( UART0, executing+'0', true );
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
  pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_console ); ///////
  pcb[ 0 ].ctx.sp   = ( uint32_t )( &tos_console );
  pcb[ 0 ].basePriority = 0;                   //Setting console to high priority so that it continues to execute.
  pcb[ 0 ].age = 0;                           /////////////////////////

  // memset( &pcb[ 1 ], 0, sizeof( pcb_t ) );
  // pcb[ 1 ].pid      = 2;
  // pcb[ 1 ].status   = STATUS_READY;
  // pcb[ 1 ].ctx.cpsr = 0x50;
  // pcb[ 1 ].ctx.pc   = ( uint32_t )( &main_P4 );
  // pcb[ 1 ].ctx.sp   = ( uint32_t )( &tos_P4  );
  // pcb[ 1 ].basePriority = 2;
  // pcb[ 1 ].age = 0;

  // memset( &pcb[ 1 ], 0, sizeof( pcb_t ) );
  // pcb[ 1 ].pid      = 2;
  // pcb[ 1 ].status   = STATUS_READY;
  // pcb[ 1 ].ctx.cpsr = 0x50;
  // pcb[ 1 ].ctx.pc   = ( uint32_t )( &main_P4 );
  // pcb[ 1 ].ctx.sp   = ( uint32_t )( &tos_P4  );
  // pcb[ 1 ].basePriority = 2;
  // pcb[ 1 ].age = 0;
  //
  // memset( &pcb[ 2 ], 0, sizeof( pcb_t ) );
  // pcb[ 2 ].pid      = 3;
  // pcb[ 2 ].status   = STATUS_READY;
  // pcb[ 2 ].ctx.cpsr = 0x50;
  // pcb[ 2 ].ctx.pc   = ( uint32_t )( &main_Ptemp );
  // pcb[ 2 ].ctx.sp   = ( uint32_t )( &tos_Ptemp  );
  // pcb[ 2 ].basePriority = 1;
  // pcb[ 2 ].age = 0;

  // memset( &pcb[ 3 ], 0, sizeof( pcb_t ) ); //this is usually pcb2
  // pcb[ 3 ].pid      = 4;
  // pcb[ 3 ].status   = STATUS_READY;
  // pcb[ 3 ].ctx.cpsr = 0x50;
  // pcb[ 3 ].ctx.pc   = ( uint32_t )( &main_P5 );
  // pcb[ 3 ].ctx.sp   = ( uint32_t )( &tos_P5  );
  // pcb[ 3 ].basePriority = 6;
  // pcb[ 3 ].age = 0;

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
    //round_robin_scheduler(ctx);
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
    // case 0x00 : { // 0x00 => yield()
    //   round_robin_scheduler( ctx );
    //   break;
    // }

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

 // fork [11, Page 881]:
 //  - create new child process with unique PID,
 //  - replicate state (e.g., address space) of parent in child,
 //  - parent and child both return from fork, and continue to execute after the call point,
 //  - return value is 0 for child, and PID of child for parent.

// Put the ctx back into pcb[executing].ctx
// Copy pcb[executing] to pcb[new]
// change pid of pcb[new]
// find top of stack of parent
// find offset of ctx.sp into that stack
// make new stack space for pcb[new]
// copy tos for parent into pcb[new].sp in full
// offset the sp for child by correct amount (same as parent)
// give 0 to child.ctx.gpr[0]
// give new pid to parent.ctx.gpr[0]

    case 0x03 : { //fork

      pcb_t* parent = &pcb[executing];
      pcb_t* child = &pcb[ n ]; //find first available pcb space

      memcpy( &parent->ctx, ctx, sizeof( ctx_t ) );

      memset( child, 0, sizeof(pcb_t));

      //memcpy( &pcb[ n+1 ].ctx, ctx, sizeof( ctx_t ));
      memcpy( child, parent, sizeof(pcb_t));
      child->pid = n + 1;

      uint32_t parentTos = (uint32_t) &tos_newProcesses-(executing*0x00001000);
      int offset = (uint32_t) parentTos - parent->ctx.sp;

      uint32_t childTos = (uint32_t) &tos_newProcesses-((n)*0x00001000);
      memcpy((void *) childTos - 0x00001000, (void *) parentTos - 0x00001000, 0x00001000 ); //minus 0x00001000 from childTos and parentTos ??
      child->ctx.sp = (uint32_t) childTos - offset;

      child->status = STATUS_READY;


      ctx->gpr[ 0 ] = child->pid;
      child->ctx.gpr[ 0 ] = 0;
      n++;

      break;
      //hilevel handler reset is being called!! Something is setting pc/lr to 0 and therefore starting it all over again.


      //create new child pcb and copy the values from the parent
      // memset( &pcb[ n+1 ], 0, sizeof( pcb_t ) );
      // pcb[ n+1 ].pid      = pcb[ executing ].pid + 1;                      //unique PID but is it a problem for this to be hardcoded>
      // pcb[ n+1 ].status   = STATUS_READY;
      // pcb[ n+1 ].ctx.cpsr = pcb[ executing ].ctx.cpsr;
      // pcb[ n+1 ].ctx.pc   = pcb[ executing ].ctx.pc;
      // pcb[ n+1 ].ctx.sp   = pcb[ executing ].ctx.sp;
      // pcb[ n+1 ].basePriority = pcb[ executing ].basePriority;
      // pcb[ n+1 ].age = pcb[ executing ].age;                           /////////////////////////
      //
      // //Copy parents stack to child's stack
      // memcpy(tos_newProcesses+(n+1)*1000, tos_newProcesses+executing*1000, 0x00001000 ); // 1) top of stack of child, 2)top of stack of parent, 3) size of what im copying
      // int offset = tos_newProcesses+executing*1000 - pcb[ executing ].ctx.sp; //where the stack pointer is in relation to the top of stack
      // pcb[n+1].ctx.sp = tos_newProcesses+(n+1)*1000 - offset; //Minus offset so that stack pointer is in same position relative to parent
      //
      // //I have made space (tos_newProcesses) for new processes, somehow have to handle that space here. ^^^^
      //
      // //  - return value is 0 for child, and PID of child for parent.
      // parent->ctx.gpr[ 0 ] = child->pid;
      // child->ctx.gpr[ 0 ] = 0;
      //
      // break;

      //The maximum size of pcb is 1!! PROBLEM.
      //Shell was being printed over and over again.

    }

    case 0x04 : { //exit
      memset( &pcb[ executing ], 0, sizeof( pcb_t ) );
      pcb[ executing ].status = STATUS_TERMINATED;   //P5 has a limit of 50 therefore calls exit (0x04), handle this.
      //pcb[ executing ].basePriority = -1;
      priority_scheduler(ctx);
      break;
    }


// EXEC
// replace current process image (e.g., text segment) with with new process image: effectively this means
//  execute a new program,
// reset state (e.g., stack pointer); continue to execute at the entry point of new program,
// no return, since call point no longer exists

     case 0x05 : { //exec

       PL011_putc( UART0, 'E', true );

       memset((uint32_t)&tos_newProcesses-(executing*0x00001000)-0x00001000, 0, 0x00001000);
       ctx->pc = ctx->gpr[0];
       ctx->sp = (uint32_t) &tos_newProcesses-(executing*0x00001000);

       // void* main_newprocess = (void *)ctx->gpr[ 0 ];
       //
       //
       // memcpy(&pcb[ executing ].ctx, ctx, sizeof( ctx_t ) ); // preserve process that was executing
       // pcb[ executing ].status = STATUS_READY;                // update  the status of executing process
       //
       // pcb[n+1].ctx.pc = (uint32_t) (main_newprocess);
       //
       //
       //
       //
       // memcpy( ctx, &pcb[ n+1].ctx, sizeof( ctx_t ) ); // restore  process to execute
       // pcb[ n+1 ].status = STATUS_EXECUTING;            // update   the status of the executing process
       // executing = n+1; //sets the process that is excuting to the new process
       //
       break;
     }
     case 0x06 : { //kill
       // for process identified by pid, send signal of x
      int pid = ctx->gpr[0];       //////////this PID is 3! Because I wrote terminate 3
      for (int i=0;i<n;i++) {
        if (pcb[i].pid == pid) {
          PL011_putc( UART0, 'K', true );
          memset( &pcb[ i ], 0, sizeof( pcb_t ) );
          pcb[ i ].status = STATUS_TERMINATED;
          break;
          //pcb[ i ].basePriority = -1;
        }
      }
     }
     break;



    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
