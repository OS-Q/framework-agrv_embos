#include "BSP_UART.h"
#include "RTOS.h"
#include "board.h"

#define BSP_UART UARTx(OS_UART)
#define BSP_UART_IRQHandler_(__UNIT) UART##__UNIT##_isr
#define BSP_UART_IRQHandler(__UNIT) BSP_UART_IRQHandler_(__UNIT)

void BSP_UART_IRQHandler(OS_UART)(void)
{
  if (UART_IsMaskedIntActive(BSP_UART, UART_INT_RX)) {
    OS_COM_OnRx(UART_ReceiveData(BSP_UART));
  }
  if (UART_IsMaskedIntActive(BSP_UART, UART_INT_TX)) {
    OS_COM_OnTx();
  }
  UART_ClearInt(BSP_UART, UART_INT_ALL);
}

void BSP_UART_DeInit(unsigned int Unit)
{
  PERIPHERAL_DISABLE_(UART, OS_UART);
}

void BSP_UART_Init(unsigned int Unit, unsigned long Baudrate, unsigned char NumDataBits, unsigned char Parity, unsigned char NumStopBits)
{
  PERIPHERAL_ENABLE_(UART, OS_UART);
  UART_Init(BSP_UART, Baudrate, UART_LCR_DATABITS_8, UART_LCR_STOPBITS_1,
            Parity == BSP_UART_PARITY_NONE ? UART_LCR_PARITY_NONE : Parity == BSP_UART_PARITY_EVEN ? UART_LCR_PARITY_EVEN : UART_LCR_PARITY_ODD,
            UART_LCR_FIFO_1);
  UART_EnableInt(BSP_UART, UART_INT_RX | UART_INT_TX);
  INT_EnableIRQ(UARTx_IRQn(OS_UART), PLIC_MAX_PRIORITY);
}

void BSP_UART_Write1(unsigned int Unit, unsigned char Data)
{
  UART_TransmitData(BSP_UART, Data);
}
