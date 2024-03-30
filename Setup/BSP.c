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
File    : BSP.c
Purpose : BSP for CertusPRO-NX_Versa
*/

#include "BSP.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define GPIO0_INST_BASE_ADDR  (0x80400u)
#define RD_DATA_REG           (*(volatile unsigned int*)(GPIO0_INST_BASE_ADDR + 0x00u))
#define SET_DATA_REG          (*(volatile unsigned int*)(GPIO0_INST_BASE_ADDR + 0x08u))
#define CLEAR_DATA_REG        (*(volatile unsigned int*)(GPIO0_INST_BASE_ADDR + 0x0Cu))
#define DIRECTION_REG         (*(volatile unsigned int*)(GPIO0_INST_BASE_ADDR + 0x10u))
#define INT_TYPE_REG          (*(volatile unsigned int*)(GPIO0_INST_BASE_ADDR + 0x14u))
#define INT_METHOD_REG        (*(volatile unsigned int*)(GPIO0_INST_BASE_ADDR + 0x18u))
#define INT_STATUS_REG        (*(volatile unsigned int*)(GPIO0_INST_BASE_ADDR + 0x1Cu))
#define INT_ENABLE_REG        (*(volatile unsigned int*)(GPIO0_INST_BASE_ADDR + 0x20u))
#define INT_SET_REG           (*(volatile unsigned int*)(GPIO0_INST_BASE_ADDR + 0x24u))
#define LED_COUNT             (8u)

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       BSP_Init()
*/
void BSP_Init(void) {
  unsigned int i;

  for (i = 0u; i < LED_COUNT; i++) {
    DIRECTION_REG  |= (1u << i);
    INT_TYPE_REG   &= (0u << i);
    INT_METHOD_REG &= (0u << i);
    INT_STATUS_REG &= (0u << i);
    INT_ENABLE_REG &= (0u << i);
    INT_SET_REG    &= (0u << i);
  }
}

/*********************************************************************
*
*       BSP_SetLED()
*/
void BSP_SetLED(int Index) {
  if ((unsigned int)Index < LED_COUNT) {
    SET_DATA_REG = (1u << Index);
  }
}

/*********************************************************************
*
*       BSP_ClrLED()
*/
void BSP_ClrLED(int Index) {
  if ((unsigned int)Index < LED_COUNT) {
    CLEAR_DATA_REG = (1u << Index);
  }
}

/*********************************************************************
*
*       BSP_ToggleLED()
*/
void BSP_ToggleLED(int Index) {
  if ((unsigned int)Index < LED_COUNT) {
    if ((RD_DATA_REG >> Index) & 1u) {
      CLEAR_DATA_REG = (1u << Index);
    } else {
      SET_DATA_REG = (1u << Index);
    }
  }
}

/*************************** End of file ****************************/
