/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2022 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system                           *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.18.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
Purpose : Initializes and handles the hardware for embOS
*/

#include "RTOS.h"
#include "BSP_UART.h"
#include "interrupt.h"
#include "board.h"

extern uint32_t __stack_pointer$;
extern uint32_t __stack_start;

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/*********************************************************************
*
*       Interrupt handling configuration
*/
#ifndef   EXTEND_GLOBAL_ISR_CONTEXT
  #define EXTEND_GLOBAL_ISR_CONTEXT  (1)  // Allows to pass a context to global ISRs
#endif

/*********************************************************************
*
*       System tick settings
*/
#define OS_TIMER_FREQ (BOARD_PLL_FREQUENCY)
#define OS_TICK_FREQ  (1000u)
#define OS_INT_FREQ   (OS_TICK_FREQ)

#define OS_TIMER_RELOAD  (OS_TIMER_FREQ / OS_INT_FREQ)

/*********************************************************************
*
*       embOSView settings
*/
#ifndef   OS_VIEW_IFSELECT
  #define OS_VIEW_IFSELECT  OS_VIEW_DISABLED
#endif

/*********************************************************************
*
*       Device specific SFRs
*/
//
//  Machine timer registers
//
#define MTIME                 (*(volatile OS_U64*)(0x200BFF8u))  // Used to generate OS tick, Timer counter register
#define MTIMECMP              (*(volatile OS_U64*)(0x2004000u))  // Used to generate OS tick, Timer compare register
//
// Core-local interrupts
//
#define NUM_LOCAL_INTERRUPTS  (16 + LOCAL_INT_COUNT)
//
//  Programmable interrupt controller
//
#define PLIC_BASE_ADDR         (PLIC_BASE)
#define PLIC_NUM_INTERRUPTS    (PLIC_TOTAL_INTERRUPT_COUNT)
/*********************************************************************
*
*       Function prototypes
*
**********************************************************************
*/
void ISR_M_Software(void);
void ISR_M_Timer   (void);
void ISR_M_External(void);

/*********************************************************************
*
*      Static          data
*
**********************************************************************
*/

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _OS_GetHWTimerCycles()
*
*  Function description
*    Returns the current hardware timer count value.
*
*  Return value
*    Current timer count value.
*/
static unsigned int _OS_GetHWTimerCycles(void) {
  unsigned int Result;
  OS_I64       Diff;
  OS_U64       Counter;
  OS_U64       Compare;

  Compare = MTIMECMP;
  Counter = MTIME;
  Diff    = (OS_I64)(Counter - Compare);
  if (Diff < 0) {
    Result = (unsigned int)(OS_TIMER_RELOAD + Diff);
  } else {
    Result = (unsigned int)(Diff);
  }
  return Result;
}

/*********************************************************************
*
*       _OS_GetHWTimer_IntPending()
*
*  Function description
*    Returns if the hardware timer interrupt pending flag is set.
*
*  Return value
*    == 0: Interrupt pending flag not set.
*    != 0: Interrupt pending flag set.
*/
static unsigned int _OS_GetHWTimer_IntPending(void) {
  return ((unsigned int)OS_CLINT_GetIntPending(IRQ_M_TIMER));
}

/*********************************************************************
*
*       _ExceptionHandler()
*
*  Function description
*    This routine may be used to investigate synchronous traps.
*
*  Additional information
*    _ExceptionHandler() is called when a synchronous trap has occurred.
*/
static void _ExceptionHandler(OS_REG_TYPE mcause, OS_REG_TYPE mepc) {
  OS_USE_PARA(mepc);                // May be used for further investigation.

#if (OS_DEBUG != 0)
  switch (mcause & MCAUSE_CAUSE) {  // Detect specific exception cause:
  case  0: while (1);               // - Instruction address misaligned.
  case  1: while (1);               // - Instruction access fault.
  case  2: while (1);               // - Illegal instruction.
  case  3: while (1);               // - Breakpoint.
  case  4: while (1);               // - Load address misaligned.
  case  5: while (1);               // - Load access fault.
  case  6: while (1);               // - Store/AMO address misaligned.
  case  7: while (1);               // - Store/AMO access fault.
  case  8: while (1);               // - Environment call from U-mode.
  case  9: while (1);               // - Environment call from S-mode.
  case 11: while (1);               // - Environment call from M-mode.
  case 12: while (1);               // - Instruction page fault.
  case 13: while (1);               // - Load page fault.
  case 15: while (1);               // - Store/AMO page fault.
  default: while (1);               // - Reserved. This should never happen.
  }
#else
  OS_USE_PARA(mcause);
  while (1);  // Halt execution
#endif
}

/*********************************************************************
*
*       _ISR_NotInstalled()
*
*  Function description
*    This routine may be used to detect non-installed interrupts.
*
*  Additional information
*    _ISR_NotInstalled() is called when an interrupt is pending for
*    which no specific interrupt handler was previously installed.
*/
static void _ISR_NotInstalled(void) {
  volatile int Dummy;

  Dummy = 1;
  while (Dummy > 0) {
    //
    // You may set a breakpoint here to detect Interrupts for which no ISR was registered
    //
  }
}

#if (EXTEND_GLOBAL_ISR_CONTEXT != 0)
/*********************************************************************
*
*       _ISR_NotInstalled_Ex()
*
*  Function description
*    This routine may be used to detect non-installed interrupts.
*
*  Additional information
*    _ISR_NotInstalled_Ex() is called when an interrupt is pending for
*    which no specific interrupt handler was previously installed.
*/
static void _ISR_NotInstalled_Ex(void* pContext) {
  volatile int Dummy;

  OS_USE_PARA(pContext);

  Dummy = 1;
  while (Dummy > 0) {
    //
    // You may set a breakpoint here to detect Interrupts for which no ISR was registered
    //
  }
}
#endif

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       ISR_M_Software()
*
*  Function description
*    This routine does not serve any specific purpose, but is included
*    for demonstration only.
*
*  Additional information
*    ISR_M_Software() is called when the Machine Software interrupt is pending.
*    Machine Software interrupt becomes pending when bit 3 (MSIP) of the
*    Machine Interrupt Pending Register (MIP) is set.
*/
void ISR_M_Software(void) {
  OS_INT_Enter();
  OS_CLINT_ClearIntPending(IRQ_M_SOFTWARE);  // Explicitly clear MSIP bit.
  //
  // Perform any functionality here.
  //
  OS_INT_Leave();
}

/*********************************************************************
*
*       ISR_M_Timer()
*
*  Function description
*    This routine performs the embOS tick handling.
*
*  Additional information
*    ISR_M_Timer() is called when the Machine Timer interrupt is pending.
*    Machine Timer Interrupt becomes pending when (MTIMECMP >= MTIME).
*/
void ISR_M_Timer(void) {
  OS_U64 Compare;

  OS_INT_Enter();
  Compare = MTIMECMP;  // Read timer compare value
  do {                 // We might have to perform numerous ticks if (machine timer) interrupt has been disabled for extended periods
    OS_TICK_Handle();
    Compare += OS_TIMER_RELOAD;
  } while (Compare <= MTIME);
  MTIMECMP = Compare;  // Eventually, write new compare value. Implicitly clears MTIP bit.
  OS_INT_Leave();
}

/*********************************************************************
*
*       ISR_M_External()
*
*  Function description
*    This routine detects the specific reason for the global IRQ and
*    calls the respective interrupt service routine that was previously
*    installed by OS_PIC_InstallISR().
*
*  Additional information
*    ISR_M_External() is called when the Machine External interrupt is
*    pending. Machine External interrupt becomes pending when it is
*    asserted by the external Programmable Interrupt Controller (PIC).
*/
void ISR_M_External(void) {
  OS_U32 IRQIndex;
  OS_INT_Enter();
  IRQIndex = OS_PLIC_ClaimInt();   // Claim highest-priority global IRQ.
  if (IRQIndex != 0u) {            // "0" indicates no IRQ was pending.
    plic_isr[IRQIndex]();    // Call appropriate handler.
    OS_PLIC_CompleteInt(IRQIndex); // Signal interrupt completion to PLIC.
  }
  OS_INT_Leave();
}

/*********************************************************************
*
*       OS_TrapHandler()
*
*  Function description
*    This routine is used to handle traps routed to the mtvec vector.
*    It checks whether the trap was caused by an exception or an
*    interrupt and decides how to continue trap handling.
*
*  Additional information
*    Handling of traps highly depends on the implemented interrupt
*    controller and the used mode. Thus, the implementation of the
*    OS_TrapHandler() might me different for other RISC-V devices.
*
*    OS_TrapHandler() forwards exceptions to _ExceptionHandler().
*/
OS_REG_TYPE OS_TrapHandler(OS_REG_TYPE mcause, OS_REG_TYPE mepc) {
  if (mcause & MCAUSE_INT) {
    //
    // Caused by interrupt: call appropriate high-level handler.
    //
    clint_isr[mcause & MCAUSE_CAUSE]();
  } else {
    //
    // Caused by synchronous trap: call fault handler.
    //
    _ExceptionHandler(mcause, mepc);
  }
  return mepc;
}

/*********************************************************************
*
*       OS_InitHW()
*
*  Function description
*    Initialize the hardware required for embOS to run.
*/
static const OS_SYSTIMER_CONFIG _SysTimerConfig = {OS_TIMER_FREQ, OS_INT_FREQ, OS_TIMER_UPCOUNTING, _OS_GetHWTimerCycles, _OS_GetHWTimer_IntPending};
void OS_InitHW(void) {
  OS_INT_IncDI();
  //
  // Initialize core-local interrupt handling
  //
  OS_CLINT_Init(NUM_LOCAL_INTERRUPTS, clint_isr);                           // Implicitly disables all sources
  for (CLINT_IRQn i = IRQ_U_SOFTWARE; i < NUM_LOCAL_INTERRUPTS; i++) {
    if (clint_isr[i] == NULL) {
      (void)OS_CLINT_InstallISR(i, _ISR_NotInstalled);                        // Install dummy handler (allows to omit NULL-pointer checks in OS_TrapHandler())
    }
  }
  OS_CLINT_SetDirectMode();                                                 // Replace the default trap_entry which was set during startup and set mode to direct
  //
  // Install and enable Machine Timer Interrupt
  //
  (void)OS_CLINT_InstallISR(IRQ_M_TIMER, ISR_M_Timer);
  OS_CLINT_EnableInt(IRQ_M_TIMER);
  //
  // Install and enable Machine External Interrupt
  //
  (void)OS_CLINT_InstallISR(IRQ_M_EXTERNAL, ISR_M_External);
  OS_CLINT_EnableInt(IRQ_M_EXTERNAL);
  //
  // Install and enable Machine Software Interrupt
  //
  OS_CLINT_EnableInt(IRQ_M_SOFTWARE);
  //
  // Initialize global interrupt handling (PLIC)
  //
  OS_PLIC_Init(PLIC_BASE_ADDR, PLIC_NUM_INTERRUPTS, PLIC_MAX_PRIORITY, plic_isr);
  for (PIC_IRQn i = IRQ_S0; i < PLIC_NUM_INTERRUPTS; i++) {
    if (plic_isr[i] == NULL) {
      (void)OS_PLIC_InstallISR(i, _ISR_NotInstalled);                          // Install dummy handler (allows to omit NULL-pointer checks in ISR_M_External()) {}
    }
  }
  //
  // Set-up the OS tick interrupt timer
  //
  MTIME    = 1u;                                                           // Configure counter register (must set the register to a non-zero value to start the counting process [1])
  MTIMECMP = OS_TIMER_RELOAD + 1u;                                         // Configure compare register
  //
  // Inform embOS about the timer settings
  //
  OS_TIME_ConfigSysTimer(&_SysTimerConfig);
#if (OS_VIEW_IFSELECT == OS_VIEW_IF_JLINK)
  JLINKMEM_SetpfOnRx(OS_COM_OnRx);
  JLINKMEM_SetpfOnTx(OS_COM_OnTx);
  JLINKMEM_SetpfGetNextChar(OS_COM_GetNextChar);
#elif (OS_VIEW_IFSELECT == OS_VIEW_IF_UART)
  BSP_UART_Init(OS_UART, OS_BAUDRATE, BSP_UART_DATA_BITS_8, BSP_UART_PARITY_NONE, BSP_UART_STOP_BITS_1);
#endif
  OS_INT_DecRI();
}

/*********************************************************************
*
*       OS_Idle()
*
*  Function description
*    This code is executed whenever no task, software timer, or
*    interrupt is ready for execution.
*    It may be used to e.g. enter a low power mode of the device.
*
*  Additional information
*    The idle loop does not have a stack of its own, therefore no
*    functionality should be implemented that relies on the stack
*    to be preserved.
*    The idle loop can be exited only when an embOS interrupt causes
*    a context switch (e.g. after expiration of a task timeout),
*    hence embOS interrupts must not permanently be disabled.
*/
void OS_Idle(void) {            // Idle loop: No task is ready to execute
  while (1) {                   // Nothing to do ... wait for interrupt
    #if (OS_DEBUG == 0)
      //
      // When uncommenting this line, please be aware device
      // specific issues could occur.
      // Therefore, we do not call __WFI() by default.
      //
      //__asm volatile("wfi");  // Switch CPU into sleep mode
    #endif
  }
}

/*********************************************************************
*
*       Optional communication with embOSView
*
**********************************************************************
*/

/*********************************************************************
*
*       OS_COM_Send1()
*
*  Function description
*    Sends one character.
*/
void OS_COM_Send1(OS_U8 c) {
#if   (OS_VIEW_IFSELECT == OS_VIEW_IF_JLINK)
  JLINKMEM_SendChar(c);
#elif (OS_VIEW_IFSELECT == OS_VIEW_IF_UART)
  BSP_UART_Write1(OS_UART, c);
#elif (OS_VIEW_IFSELECT == OS_VIEW_IF_ETHERNET)
  UDP_Process_Send1(c);
#elif (OS_VIEW_IFSELECT == OS_VIEW_DISABLED)
  OS_USE_PARA(c);          // Avoid compiler warning
  OS_COM_ClearTxActive();  // Let embOS know that Tx is not busy
#endif
}

/*************************** End of file ****************************/
