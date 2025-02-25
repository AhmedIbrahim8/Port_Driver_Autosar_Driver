 /******************************************************************************
 *
 * Module: Port
 *
 * File Name: Port.c
 *
 * Description: Source file for TM4C123GH6PM Microcontroller - Port Driver
 *
 * Author: Ahmed Ibrahim
 ******************************************************************************/

#include "Port.h"
#include "Port_Regs.h"

/* If the development error is off, no need to include Det.h*/
#if (PORT_DEV_ERROR_DETECT == STD_HIGH)

#include "Det.h"
/* Check the Autosar Version Between Det.h and Port.h */
#if ((PORT_AR_RELEASE_MAJOR_VERSION != DET_AR_MAJOR_VERSION)\
  || (PORT_AR_RELEASE_MINOR_VERSION != DET_AR_MINOR_VERSION)\
  || (PORT_AR_RELEASE_PATCH_VERSION != DET_AR_PATCH_VERSION))
#error "The Autosar Version of Det.h doesn't match the version of Port.h.h"

#endif
#endif


STATIC const Port_ConfigChannel *Port_Channels =NULL_PTR;
/* Variable to store the condition of the port driver ( initialized or not )*/
uint8 Port_Status = PORT_NOT_INITIALIZED;

/************************************************************************************
* Service Name: Port_Init
* Service ID[hex]: 0x00
* Sync/Async: Synchronous
* Reentrancy: Non reentrant
* Parameters (in): ConfigPtr - Pointer to configuration data
* Parameters (inout): None
* Parameters (out): None
* Return value: None
* Description: Function to Initialize the Port module.
************************************************************************************/
void Port_Init( const Port_ConfigType* ConfigPtr )
{
  /* local variable used to loop of the configuration array       */
  uint8 index;
  /* local variable to store the base address of the GPIO Register*/
  volatile uint32 *Port_Base_Address_Ptr = NULL_PTR;
  /* To wait three(3) seconds untill clock reach the Port */
  volatile uint8 dealy;
  
#if (PORT_DEV_ERROR_DETECT == STD_HIGH)
  if(ConfigPtr == NULL_PTR){
    Det_ReportError(PORT_MODULE_ID,
                    PORT_INSTANCE_ID,
                    PORT_INIT_SID,
                    PORT_E_PARAM_CONFIG);
  }
  else
#endif
  {
    /* Intializing the Port so, the status should be updated */
    Port_Status= PORT_INITIALIZED;
    /* Port_channels is variable from type Port_ConfigChannel             *
     * ConfigPtr is variable from type Port_ConfigType                    *
     * Here, Port_Channels will point to the first array in the structure */
    Port_Channels = ConfigPtr->channels;
    for(index=FIRST_LOOP;index<PORT_CONFIGURED_PINS;index++)
    {
      switch(Port_Channels[index].port_num)
      {
        /* PORTA Base Address */
      case PORTA_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTA_BASE_ADDRESS;
        break;
        /* PORTB Base Address */
      case PORTB_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTB_BASE_ADDRESS;
        break;
        /* PORTC Base Address */
      case PORTC_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTC_BASE_ADDRESS;
        break;
        /* PORTD Base Address */
      case PORTD_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTD_BASE_ADDRESS;
        break;
        /* PORTE Base Address */
      case PORTE_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTE_BASE_ADDRESS;
        break;
        /* PORTF Base Address */
      case PORTF_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTF_BASE_ADDRESS;
        break;
        /* (Misra Rules) */
      default:
        /* NO Action is needed (Misra Rules)*/
        break;
      }
      /* Initiate the clock of the Port*/
      SET_BIT(SYSCTL_REGCGC2_REG,Port_Channels[index].port_num);
      /* To wait 3-seconds (Equal Operation takes 3-seconds )*/
      dealy=SYSCTL_REGCGC2_REG;
      /* Important check : In TivaC PD7 & PF0 are special pins which *
       *                   are needed to be unlocked first before    *
       *                   write in it                               */
      if(((Port_Channels[index].port_num == PORTD_ID) && (Port_Channels[index].pin_num == SPECIAL_PIN_PD7))\
       ||((Port_Channels[index].port_num == PORTF_ID) && (Port_Channels[index].pin_num == SPECIAL_PIN_PF0)))
      {
        /* To unlock the commit register we put the magic number which is defined in port.h */
        *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_LOCK_REG_OFFSET)= MAGIC_NUMBER;
        /* Commit the pin based on the pin_num */
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_COMMIT_REG_OFFSET),Port_Channels[index].pin_num);
      }
      /* If JTAG Pins, we will not write any thing to them by making continue statement *
       * JTAG PINS are : - PC0                                                          *
       *                 - PC1                                                          *
       *                 - PC2                                                          *
       *                 - PC3                                                          */
      else if(((Port_Channels[index].port_num == PORTC_ID) && (Port_Channels[index].pin_num == JTAG_PIN_PC0))\
            ||((Port_Channels[index].port_num == PORTC_ID) && (Port_Channels[index].pin_num == JTAG_PIN_PC1))\
            ||((Port_Channels[index].port_num == PORTC_ID) && (Port_Channels[index].pin_num == JTAG_PIN_PC2))\
            ||((Port_Channels[index].port_num == PORTC_ID) && (Port_Channels[index].pin_num == JTAG_PIN_PC3)))
      {
        /*                                     (Violation Of Misra Rules)                                  *
         * Justification : We need to protect JTAG Pins From User that's why we made the continue Statment */
        continue;
      }
      /* Misra Rules */
      else
      {
        /* No Action nedded */
      }
      
      /* Check the direction of the pins                                *
       * if input pin :it should determine pull-up internal resistor    *
       *               or pull-down internal resistor or off based on   *
       *               the configuration structure                      *
       * if output pin:it should initiate the pin with the initial value*
       *               based on the configuration structure             *
       ******************************************************************/
      if(Port_Channels[index].direction == PORT_PIN_IN)
      {
        /* Pin is input so zero(0) should be put in the bit number at the Direction Register */
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIR_REG_OFFSET),Port_Channels[index].pin_num);
        /* if Internal Resistor = Pull Down */
        if(Port_Channels[index].resistor == PULL_DOWN)
        {
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_PULL_DOWN_REG_OFFSET),Port_Channels[index].pin_num);
        }
        /* if Internal Resistor = Pull Up */
        else if(Port_Channels[index].resistor == PULL_UP)
        {
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_PULL_UP_REG_OFFSET),Port_Channels[index].pin_num);
        }
        /* if Internal Resistor = off */
        else
        {
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_PULL_UP_REG_OFFSET),Port_Channels[index].pin_num);
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_PULL_DOWN_REG_OFFSET),Port_Channels[index].pin_num);
        }
        
      }
      else if(Port_Channels[index].direction == PORT_PIN_OUT)
      {
        /* Pin is output so one(1) should be put in the bit number at the Direction Register */
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIR_REG_OFFSET),Port_Channels[index].pin_num);
        /* PIN is Output so, We will clear the bit of this pin at the pull up and pull down register (To ensure that it is zero) */
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_PULL_DOWN_REG_OFFSET),Port_Channels[index].pin_num);
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_PULL_UP_REG_OFFSET),Port_Channels[index].pin_num);
        /* if initial value of the pin equal zero */
        if(Port_Channels[index].initial_value == PORT_PIN_LEVEL_LOW)
        {
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DATA_REG_OFFSET),Port_Channels[index].pin_num);
        }
        /* if initial value of the pin equal one */
        else if(Port_Channels[index].initial_value == PORT_PIN_LEVEL_HIGH)
        {
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DATA_REG_OFFSET),Port_Channels[index].pin_num);
        }
        /* Misra Rules */
        else
        {
          /* No Action Needed*/
        }
      }
      /* Misra Rules */
      else
      {
        /* No Action nedded*/
      }
      
      /* Check the modes to adjust : - Control Register           *
       *                             - Alernative Register        *
       *                             - Analog Register            *
       *                             - Digital Register           *
       ************************************************************/
      switch(Port_Channels[index].mode)
      {
        /* If Pin Mode = DIO Mode                                     *
         *     - 0 at Alternate Register                            * 
         *     - 1 at Digital Register                                *
         *     - 0 at Analog Register                                 *
         *     - 0 at the 4-bit of the pin number at control register */
      case PORT_PIN_MODE_DIO:
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[index].pin_num);
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[index].pin_num);
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[index].pin_num);
        *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)= \
       (*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT));
        break;
        /* If Pin Mode = ADC Mode                                     *
         *     - 0 at Alternate Register                            * 
         *     - 0 at Digital Register                                *
         *     - 1 at Analog Register                                 *
         *     - 0 at the 4-bit of the pin number at control register */
      case PORT_PIN_MODE_ADC:
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[index].pin_num);
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[index].pin_num);
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[index].pin_num);
        *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)= \
       (*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT));
        break;
        /* There is Analog Comparator and Digital Comparator and we need to differentiate *
         * between them. That can be done by the pin index.                               *
         *                      (TivaC Data Sheet Page 651)                               *
         * if PF0 or PF1               : Digital Comparator                               *
         * if PC4 or PC5 or PC6 or PC7 : Analog Comparator                                */
      case PORT_PIN_MODE_COMPARATOR:
        /* If Digital Comparator : - 1 at Alternate Register                                  *
         *                         - 0 at Analog Register                                     *
         *                         - 1 at Digital Register                                    *
         *                         - Number 9(DIGITAL_COMPARATOR_MODE) at Control Register    */
        if((index == PORTF_PIN0_ID_INDEX) || (index == PORTF_PIN1_ID_INDEX))
        {
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[index].pin_num);
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[index].pin_num);
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[index].pin_num);
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
          |(DIGITAL_COMPARATOR_MODE<<((Port_Channels[index].pin_num)*BIT_SHIFT));
        }
        /* If Ananlog Comparator : - 0 at Alternate Register                                  *
         *                         - 1 at Analog Register                                     *
         *                         - 0 at Digital Register                                    *
         *                         - Number 0 at Control Register                             */
        else
        {
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[index].pin_num);
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[index].pin_num);
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[index].pin_num);
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)= \
         (*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT));
        }
        break;
        /* There are 4-options which are :                               *
         *    - (USB0epen & USB0pflt) are Digital signals                *
         *    - ( USB0DM  &  USB0DP ) are Analog signals                 *
         * If PD2 or PD3 or PF4 or PC6 or PC7: Digital Signal            *
         * If PD4 or PD5 : Analog Signal                                 */
      case PORT_PIN_MODE_USB:
        /*    Analog :  - 0 at Alternate Register         *
         *              - 1 at Analog Register            *
         *              - 0 at Digiral Register           *
         *              - Numer 0 at Control Register     */
        if((index == PORTD_PIN4_ID_INDEX) || (index == PORTD_PIN5_ID_INDEX))
        {
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[index].pin_num);
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[index].pin_num);
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[index].pin_num);
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)= \
         (*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT));
        }
        /*   Digital :  - 1 at Alternate Register                           *
         *              - 0 at Analog Register                              *
         *              - 1 at Digiral Register                             *
         *              - Numer 8(USB_DIGITAL_MODE) at Control Register     */
        else
        {
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[index].pin_num);
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[index].pin_num);
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[index].pin_num);
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
          |(USB_DIGITAL_MODE<<((Port_Channels[index].pin_num)*BIT_SHIFT));
        }
        break;
          
        /* If Pin Mode is something else                                                                    *
         *     - 1 at Alternative Register                                                                  *
         *     - 1 at Digital Register (All Pin Modes are digital except ADC,Analog Comparator,USB Analog)  *
         *     - 0 at Analog Register                                                                       *
         *     - switch case to specify which mode of them                                                  */
      default:
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[index].pin_num);
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[index].pin_num);
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[index].pin_num);
        /* Switch Case to specify the mode */
        switch(Port_Channels[index].mode)
        {
          /* If the mode is CAN                               *
           * Note : - Can Mode Number is 3 at PF0 and PF3     *
           *        - Can Mode Number is 8 at the rest        */
        case PORT_PIN_MODE_CAN:
          /* Number 3(CAN_MODE_3) should be inserted at the 4-bits of the pin number if PF0 or PF3 */
          if((index == PORTF_PIN0_ID_INDEX) || (index == PORTF_PIN3_ID_INDEX))
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
          |(CAN_MODE_3<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          }
          /* Number 8(CAN_MODE_8) should be inserted at the 4-bits of the pin number if pin index is something else */
          else
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
          |(CAN_MODE_8<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          }
          break;
          /* If the mode is I2C, Number 3(I2C_MODE) should be inserted at the 4-bit of the pin number */
        case PORT_PIN_MODE_I2C:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
          |(I2C_MODE<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          break;
          /* It shall be used under control of the general purpose timer driver             *
           * Example : ICU(INPUT CAPTURE UNIT) which name is Capture Compare PWM pins (CCP) *
           * T0CCP0 ----> T3CCP1 & WT0CCP0 ----> WT5CCP1                                    *
           * Number 7(DIO_GPT_MODE) should be inserted at the 4-bit of the pin number       */
        case PORT_PIN_MODE_DIO_GPT:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
          |(DIO_GPT_MODE<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          break;
          /* If Non-Maskable Interrupt Mode, Number 8(NMI_MODE) at control register*/
        case PORT_PIN_MODE_NMI:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
          |(NMI_MODE<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          break;
          /* If Trace Data Mode, Number 14(TRACE_DATA_MODE) at Control reister */
        case PORT_PIN_MODE_TRACE_DATA:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
          |(TRACE_DATA_MODE<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          break;
          /* If QEI Mode, Number 6 at control register */
        case PORT_PIN_MODE_QEI:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
          |(QEI_MODE<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          break;
          /*                      If SPI Mode                           *
           * [PA2,PA3,PA4,PA5] used with SPI0                           *
           * [PF0,PF1,PF2,PF4] used with SPI1                           *
           * [PB4,PB5,PB6,PB7] used with SPI2                           *
           * [PD0,PD1,PD2,PD3] used with SPI3 and SPI1 so, we will      *
           * use PORTD with SPI3 and not SPI1 because we can use SPI3   *
           * only with PORTD but SPI1 can be used with PORTD            */
        case PORT_PIN_MODE_SPI:
          /* If [PD0 or PD1 or PD2 or PD3], number 1(SPI_MODE_1) at Control Register which is SPI3 */
          if((index == PORTD_PIN0_ID_INDEX) || (index == PORTD_PIN1_ID_INDEX) || (index == PORTD_PIN2_ID_INDEX) || (index == PORTD_PIN3_ID_INDEX))
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
          ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
            |(SPI_MODE_1<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          }
          /* If Something we will use numer 2(SPI_MODE_2) at Control Register which can be SPI0 or SPI1 or SPI2 based *
           * on the port number and pin number.                                                                       */
          else
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
          ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
            |(SPI_MODE_2<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          }
          break;
          /*                                 If PWM Mode                            *
           * PE4 Can be used with M1PWM2 & M0PWM4                                   *
           * PA6 Can be used with M1PWM2                                            *
           * So, we will use PE4 with M0PWM4                                        *
           * PE5 Can be used with M1PWM3 & M0PWM5                                   *
           * PA7 Can be used with M1PWM3                                            *
           * So, we will use PE5 with M0PWM5                                        *
           * PD0 Can be used with M0PWM6 & M1PWM0                                   *
           * PC4 Can be used with M0PWM6                                            *
           * So, we will use PD0 with M1PWM0                                        *
           * PD1 Can be used with M0PWM7 & M1PWM1                                   *
           * PC5 Can be used with M0PWM7                                            *
           * So, we will use PD1 with M1PWM1*/
        case PORT_PIN_MODE_PWM:
          /* Incase of PWM0 With [PB4 PB5 PB6 PB7 PC4 PC5 PE4 PE5], number 4(PWM0_MODE) at Control Register */
          if((index==PORTB_PIN4_ID_INDEX)||(index==PORTB_PIN5_ID_INDEX)||(index==PORTB_PIN6_ID_INDEX)||\
             (index==PORTB_PIN7_ID_INDEX)||(index==PORTC_PIN4_ID_INDEX)||(index==PORTC_PIN5_ID_INDEX)||\
             (index==PORTE_PIN4_ID_INDEX)||(index==PORTE_PIN5_ID_INDEX))
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
          ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
            |(PWM0_MODE<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          }
          /* Incase of PWM1 With [PA6 PA7 PD0 PD1 PF0 PF1 PF2 PF3], number 5(PWM1_MODE) at Control Register */
          else
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
          ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
            |(PWM1_MODE<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          }
          break;
          /*                                 If UART Mode                            *
           * PC4 with U1Rx & U4Rx                                                    *
           * PB0 with U1Rx                                                           *
           * So, we will use PC4 with U4Rx                                           *
           * PC5 with U1Tx & U4Tx                                                    *
           * PB1 with U1Tx                                                           *
           * So, we will use PC5 with U4Tx                                           *
           * UART Number will be 1(UART_MODE) at Control Register                    */
        case PORT_PIN_MODE_UART:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
          ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[index].pin_num)*BIT_SHIFT)))\
            |(UART_MODE<<((Port_Channels[index].pin_num)*BIT_SHIFT));
          break;
          /* Misra Rules */
        default :
          /* No Action Needed */
          break;
            
          
          
          
        }/* End of switch case inside the default */
        
        
        break;
      }/* End Of switch case (mode)*/
     
      
    }/* End Of For Loop*/
  
  
  }  /* End Of else */
  
  
}    /* End of Port_Init() */



/************************************************************************************
* Service Name: Port_SetPinDirection
* Service ID[hex]: 0x01
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Parameters (in): - Pin       : Port Pin ID Number
*                  - Direction : Port Pin Direction
* Parameters (inout): None
* Parameters (out): None
* Return value: None
* Description: Function to set the port pin direction at runtime.
************************************************************************************/
/* PORT086: The function Port_SetPinDirection shall only be available            * 
 *          to the user if the pre-compile parameter PortSetPinDirectionApi      *
 *          is set to TRUE. If set to FALSE, the function Port_SetPinDirection   *
 *          is not available.                                                    *
 * Function will be Removed if it is OFF at the configuration tool.              */
#if (PORT_SET_PIN_DIRECTION_API == STD_ON)
void Port_SetPinDirection( Port_PinType Pin, 
                           Port_PinDirectionType Direction )
{
  /* Local variable to store the base address of the GPIO register */
  volatile uint32 *Port_Base_Address_Ptr= NULL_PTR;
  /* Local Variable to store the error status */
  boolean error= FALSE;
  /* Det error checking code will be removed if it is off at the configuration tool */
#if(PORT_DEV_ERROR_DETECT == STD_ON)
  /* Checking if the pin direction is not changeable, it will report a det error *
   * and change the state of the error to be TRUE.                               */
  if(Port_Channels[Pin].direction_changeable==STD_OFF)
  {
    Det_ReportError(PORT_MODULE_ID,
                    PORT_INSTANCE_ID,
                    PORT_SET_PIN_DIRECTION_SID,
                    PORT_E_DIRECTION_UNCHANGEABLE);
    /* Error is happened so, it will be TRUE */
    error= TRUE;
  }
  /* Misra Rules */
  else
  {
    /* No Action Needed */
  }
  /* Checking if the pin equal or greater than the Port Configured Channels, it will*
   * report a det error and change the state of the error to be TRUE.               */
  if(Pin>= PORT_CONFIGURED_PINS)
  {
    Det_ReportError(PORT_MODULE_ID,
                    PORT_INSTANCE_ID,
                    PORT_SET_PIN_DIRECTION_SID,
                    PORT_E_PARAM_PIN);
    /* Error is happened so, it will be TRUE */
    error= TRUE;
  }
  /* Misra Rules */
  else
  {
    /* No Action Needed*/
  }
  /* Checking the status of the port driver. if it is uninitialized it will report  *
   * a det error and change the state of the error to TRUE.                         */
  if(Port_Status==PORT_NOT_INITIALIZED)
  {
    Det_ReportError(PORT_MODULE_ID,
                    PORT_INSTANCE_ID,
                    PORT_SET_PIN_DIRECTION_SID,
                    PORT_E_UNINIT);
    /* Error is happened so, it will be TRUE */
    error= TRUE;
  }
  /* Misra Rules */
  else
  {
    /* No Action Needed*/
  }

#endif /* End Of Det Error Checking */
  /* If the error state is FALSE, we can change the mode of the pin    *
   * If the error state is TRUE, The error is happened and it will not *
     enter the if condition and will exit the function.                */
  if(error==FALSE)
  {
    /* To specify the GPIO Base Address By the port num at the specified Pin Index */
   switch(Port_Channels[Pin].port_num)
      {
        /* PORTA Base Address */
      case PORTA_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTA_BASE_ADDRESS;
        break;
        /* PORTB Base Address */
      case PORTB_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTB_BASE_ADDRESS;
        break;
        /* PORTC Base Address */
      case PORTC_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTC_BASE_ADDRESS;
        break;
        /* PORTD Base Address */
      case PORTD_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTD_BASE_ADDRESS;
        break;
        /* PORTE Base Address */
      case PORTE_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTE_BASE_ADDRESS;
        break;
        /* PORTF Base Address */
      case PORTF_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTF_BASE_ADDRESS;
        break;
        /* (Misra Rules) */
      default:
        /* NO Action is needed (Misra Rules)*/
        break;
      }
   switch(Direction)
   {
     /* Pin is input so zero(0) should be put in the bit number at the Direction Register */
   case PORT_PIN_IN:
     CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIR_REG_OFFSET),Port_Channels[Pin].pin_num);
     break;
     /* Pin is output so one(1) should be put in the bit number at the Direction Register */
   case PORT_PIN_OUT:
     CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIR_REG_OFFSET),Port_Channels[Pin].pin_num);
     break;
     /* Misra Rules */
   default:
     /* No Action Needed */
     break;
   }
   
  }
  /* Misra Rules */
  else
  {
    /* No Action Nedded */
  }
  
}

#endif
                


/************************************************************************************
* Service Name: Port_RefreshPortDirection
* Service ID[hex]: 0x02
* Sync/Async: Synchronous
* Reentrancy: Non reentrant
* Parameters (in): None
* Parameters (inout): None
* Parameters (out): None
* Return value: None
* Description: Function to refresh port direction.
************************************************************************************/
void Port_RefreshPortDirection( void )
{
  /* Local variable to store the base address of the GPIO register */
  volatile uint32 *Port_Base_Address_Ptr= NULL_PTR;
  /* local variable used to loop of the configuration array       */
  uint8 index;
  /* Local Variable to store the error status */
  boolean error= FALSE;
  /* Det error checking code will be removed if it is off at the configuration tool */
#if(PORT_DEV_ERROR_DETECT ==STD_HIGH)
  /* Checking the status of the port driver. if it is uninitialized it will report  *
   * a det error and change the state of the error to TRUE.                         */
  if(Port_Status==PORT_NOT_INITIALIZED)
  {
    Det_ReportError(PORT_MODULE_ID,
                    PORT_INSTANCE_ID,
                    PORT_REFRESH_PORT_DIRECTION_SID,
                    PORT_E_UNINIT);
    /* Error is happened so, it will be TRUE */
    error=TRUE;
  }
  /* Misra Rules */
  else
  {
    /* No Action Needed*/
  }
  
#endif
  if(error==FALSE)
  {
    for(index=FIRST_LOOP;index<PORT_CONFIGURED_PINS;index++)
    {
      /* - Port Driver SWS Page (34)                                                                  *
           [PORT061]: The function Port_RefreshPortDirection shall exclude those port pins            *
                      from refreshing that are configured as �pin direction changeable during runtime *
       * Check if the pin direction is changeable, it will not be refreshed                           */
      if(Port_Channels[index].direction_changeable ==STD_OFF)
      {
        switch(Port_Channels[index].port_num)
      {
        /* PORTA Base Address */
      case PORTA_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTA_BASE_ADDRESS;
        break;
        /* PORTB Base Address */
      case PORTB_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTB_BASE_ADDRESS;
        break;
        /* PORTC Base Address */
      case PORTC_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTC_BASE_ADDRESS;
        break;
        /* PORTD Base Address */
      case PORTD_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTD_BASE_ADDRESS;
        break;
        /* PORTE Base Address */
      case PORTE_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTE_BASE_ADDRESS;
        break;
        /* PORTF Base Address */
      case PORTF_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTF_BASE_ADDRESS;
        break;
        /* (Misra Rules) */
      default:
        /* NO Action is needed (Misra Rules)*/
        break;
      }/* End Of Switch case */
      switch(Port_Channels[index].direction)
      {
      case PORT_PIN_IN:
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIR_REG_OFFSET),Port_Channels[index].pin_num);
        break;
      case PORT_PIN_OUT:
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIR_REG_OFFSET),Port_Channels[index].pin_num);
        break;
        /* Misra Rules */
      default:
        /* No Action Needed */
        break;
      }/* End Of Switch Case */
      
      }/* End of if condition */
      
      /* Misra Rules */
      else
      {
        /* No Action Nedded */
      }
      
      
    }/* End Of For Loop */
    
  }/* End Of if condition */
  /* Misra Rules */
  else
  {
    /* No Action Needed */
  }
  
  
}/* End Of Refresh function */

/************************************************************************************
* Service Name: Port_GetVersionInfo
* Service ID[hex]: 0x03
* Sync/Async: Synchronous
* Reentrancy: Non reentrant
* Parameters (in): None
* Parameters (inout): None
* Parameters (out): versioninfo : Pointer to where to store the version information 
*                                 of this module.
* Return value: None
* Description: Function to return the version information of the module.
************************************************************************************/
/* [PORT103] : The function Port_GetVersionInfo shall be pre compile time configurable *
 *             On/Off by the configuration parameter PortVersionInfoApi.               *
 * Function will be Removed if it is OFF at the configuration tool.                    */
#if (PORT_VERSION_INFO_API== STD_ON)
void Port_GetVersionInfo( Std_VersionInfoType* versioninfo )
{

#if (PORT_DEV_ERROR_DETECT == STD_ON)
/* PORT225: if Det is enabled, the parameter versioninfo shall be checked for being    *
 *          NULL. The error PORT_E_PARAM_POINTER shall be reported in case the value   *
 *          is a NULL pointer                                                          */
  if(versioninfo == NULL_PTR)
  {
    Det_ReportError(PORT_MODULE_ID,
                    PORT_INSTANCE_ID,
                    PORT_GET_VERSION_INFO_SID,
                    PORT_E_PARAM_POINTER);
  }
  else
#endif
  {
    /* Casting of the vendor id to uint16 as the member of the structure       */
    versioninfo->vendorID=(uint16)PORT_VENDOR_ID;
    /* Casting of the module id to uint16 as the member of the structure       */
    versioninfo->moduleID=(uint16)PORT_MODULE_ID;
    /* Casting of the autosar version to uint16 as the member of the structure */
    versioninfo->sw_major_version=(uint8)PORT_AR_RELEASE_MAJOR_VERSION;
    versioninfo->sw_minor_version=(uint8)PORT_AR_RELEASE_MINOR_VERSION;
    versioninfo->sw_patch_version=(uint8)PORT_AR_RELEASE_PATCH_VERSION;
  }
  
}

#endif




/************************************************************************************
* Service Name: Port_SetPinMode
* Service ID[hex]: 0x04
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Parameters (in): - Pin  : Port Pin ID Number
*                  - Mode : New Port Pin mode to be set on port pin
* Parameters (inout): None
* Parameters (out): None
* Return value: None
* Description: Function to set the port pin mode at runtime.
************************************************************************************/
/* true: Enabled - Function Port_SetPinMode() is available.      *
 * false: Disabled - Function Port_SetPinMode() is not available.*/
#if (PORT_SET_PIN_MODE_API == STD_ON)
void Port_SetPinMode( Port_PinType Pin, 
                     Port_PinModeType Mode )
{
  /* Local variable to store the base address of the GPIO register */
  volatile uint32 *Port_Base_Address_Ptr= NULL_PTR;
  /* Local Variable to store the error status */
  boolean error= FALSE;
  /* Det error checking code will be removed if it is off at the configuration tool */
#if(PORT_DEV_ERROR_DETECT == STD_ON)
  /* PORT223: If Det is enabled, the function Port_SetPinMode shall return PORT_E_MODE_UNCHANGEABLE  *
   * and return without any action, if the parameter PortPinModeChangeable is set to FALSE.          */
  if(Port_Channels[Pin].mode_changeable == STD_OFF)
  {
    Det_ReportError(PORT_MODULE_ID,
                    PORT_INSTANCE_ID,
                    PORT_SET_PIN_MODE_SID,
                    PORT_E_MODE_UNCHANGEABLE);
    error= TRUE;
  }
  /* Misra Rules */
  else
  {
    /* No Action Needed */
  }
  /* If the mode is out of range, it will report a det error and change the state of the error to be true */
  if(Mode > PORT_LAST_MODE_NUMBER)
  {
    Det_ReportError(PORT_MODULE_ID,
                    PORT_INSTANCE_ID,
                    PORT_SET_PIN_MODE_SID,
                    PORT_E_PARAM_INVALID_MODE);
    error= TRUE;
  }
  /* Misra Rules */
  else
  {
    /* No Action Needed */
  }
  /* Checking if the pin equal or greater than the Port Configured Channels, it will*
   * report a det error and change the state of the error to be TRUE.               */
  if(Pin>=PORT_CONFIGURED_PINS)
  {
    Det_ReportError(PORT_MODULE_ID,
                    PORT_INSTANCE_ID,
                    PORT_SET_PIN_MODE_SID,
                    PORT_E_PARAM_PIN);
    error= TRUE;
  }
  /* Misra Rules */
  else
  {
    /* No Action Needed */
  }
  /* Checking the status of the port driver. if it is uninitialized it will report  *
   * a det error and change the state of the error to TRUE.                         */
  if(Port_Status == PORT_NOT_INITIALIZED)
  {
    Det_ReportError(PORT_MODULE_ID,
                    PORT_INSTANCE_ID,
                    PORT_SET_PIN_MODE_SID,
                    PORT_E_UNINIT);
    error= TRUE;
  }
  /* Misra Rules */
  else
  {
    /* No Action Neede */
  }
  
#endif
  if(error == FALSE)
  {
    switch(Port_Channels[Pin].port_num)
      {
        /* PORTA Base Address */
      case PORTA_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTA_BASE_ADDRESS;
        break;
        /* PORTB Base Address */
      case PORTB_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTB_BASE_ADDRESS;
        break;
        /* PORTC Base Address */
      case PORTC_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTC_BASE_ADDRESS;
        break;
        /* PORTD Base Address */
      case PORTD_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTD_BASE_ADDRESS;
        break;
        /* PORTE Base Address */
      case PORTE_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTE_BASE_ADDRESS;
        break;
        /* PORTF Base Address */
      case PORTF_ID:
        Port_Base_Address_Ptr =(volatile uint32*)GPIO_PORTF_BASE_ADDRESS;
        break;
        /* (Misra Rules) */
      default:
        /* NO Action is needed (Misra Rules)*/
        break;
      }
      
      /* Check the modes to adjust : - Control Register           *
       *                             - Alernative Register        *
       *                             - Analog Register            *
       *                             - Digital Register           *
       ************************************************************/
      switch(Port_Channels[Pin].mode)
      {
        /* If Pin Mode = DIO Mode                                     *
         *     - 0 at Alternate Register                            * 
         *     - 1 at Digital Register                                *
         *     - 0 at Analog Register                                 *
         *     - 0 at the 4-bit of the pin number at control register */
      case PORT_PIN_MODE_DIO:
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[Pin].pin_num);
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[Pin].pin_num);
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[Pin].pin_num);
        *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)= \
       (*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
        break;
        /* If Pin Mode = ADC Mode                                     *
         *     - 0 at Alternate Register                            * 
         *     - 0 at Digital Register                                *
         *     - 1 at Analog Register                                 *
         *     - 0 at the 4-bit of the pin number at control register */
      case PORT_PIN_MODE_ADC:
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[Pin].pin_num);
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[Pin].pin_num);
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[Pin].pin_num);
        *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)= \
       (*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
        break;
        /* There is Analog Comparator and Digital Comparator and we need to differentiate *
         * between them. That can be done by the pin index.                               *
         *                      (TivaC Data Sheet Page 651)                               *
         * if PF0 or PF1               : Digital Comparator                               *
         * if PC4 or PC5 or PC6 or PC7 : Analog Comparator                                */
      case PORT_PIN_MODE_COMPARATOR:
        /* If Digital Comparator : - 1 at Alternate Register                                  *
         *                         - 0 at Analog Register                                     *
         *                         - 1 at Digital Register                                    *
         *                         - Number 9(DIGITAL_COMPARATOR_MODE) at Control Register    */
        if((Pin == PORTF_PIN0_ID_INDEX) || (Pin == PORTF_PIN1_ID_INDEX))
        {
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[Pin].pin_num);
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[Pin].pin_num);
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[Pin].pin_num);
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
          |(DIGITAL_COMPARATOR_MODE<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
        }
        /* If Ananlog Comparator : - 0 at Alternate Register                                  *
         *                         - 1 at Analog Register                                     *
         *                         - 0 at Digital Register                                    *
         *                         - Number 0 at Control Register                             */
        else
        {
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[Pin].pin_num);
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[Pin].pin_num);
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[Pin].pin_num);
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)= \
         (*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
        }
        break;
        /* There are 4-options which are :                               *
         *    - (USB0epen & USB0pflt) are Digital signals                *
         *    - ( USB0DM  &  USB0DP ) are Analog signals                 *
         * If PD2 or PD3 or PF4 or PC6 or PC7: Digital Signal            *
         * If PD4 or PD5 : Analog Signal                                 */
      case PORT_PIN_MODE_USB:
        /*    Analog :  - 0 at Alternate Register         *
         *              - 1 at Analog Register            *
         *              - 0 at Digiral Register           *
         *              - Numer 0 at Control Register     */
        if((Pin == PORTD_PIN4_ID_INDEX) || (Pin == PORTD_PIN5_ID_INDEX))
        {
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[Pin].pin_num);
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[Pin].pin_num);
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[Pin].pin_num);
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)= \
         (*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
        }
        /*   Digital :  - 1 at Alternate Register                           *
         *              - 0 at Analog Register                              *
         *              - 1 at Digiral Register                             *
         *              - Numer 8(USB_DIGITAL_MODE) at Control Register     */
        else
        {
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[Pin].pin_num);
          CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[Pin].pin_num);
          SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[Pin].pin_num);
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
          |(USB_DIGITAL_MODE<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
        }
        break;
          
        /* If Pin Mode is something else                                                                    *
         *     - 1 at Alternative Register                                                                  *
         *     - 1 at Digital Register (All Pin Modes are digital except ADC,Analog Comparator,USB Analog)  *
         *     - 0 at Analog Register                                                                       *
         *     - switch case to specify which mode of them                                                  */
      default:
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ALT_FUNC_REG_OFFSET),Port_Channels[Pin].pin_num);
        SET_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_DIGITAL_ENABLE_REG_OFFSET),Port_Channels[Pin].pin_num);
        CLEAR_BIT(*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_ANALOG_MODE_SEL_REG_OFFSET),Port_Channels[Pin].pin_num);
        /* Switch Case to specify the mode */
        switch(Port_Channels[Pin].mode)
        {
          /* If the mode is CAN                               *
           * Note : - Can Mode Number is 3 at PF0 and PF3     *
           *        - Can Mode Number is 8 at the rest        */
        case PORT_PIN_MODE_CAN:
          /* Number 3(CAN_MODE_3) should be inserted at the 4-bits of the pin number if PF0 or PF3 */
          if((Pin == PORTF_PIN0_ID_INDEX) || (Pin == PORTF_PIN3_ID_INDEX))
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
          |(CAN_MODE_3<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          }
          /* Number 8(CAN_MODE_8) should be inserted at the 4-bits of the pin number if pin index is something else */
          else
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
          |(CAN_MODE_8<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          }
          break;
          /* If the mode is I2C, Number 3(I2C_MODE) should be inserted at the 4-bit of the pin number */
        case PORT_PIN_MODE_I2C:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
          |(I2C_MODE<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          break;
          /* It shall be used under control of the general purpose timer driver             *
           * Example : ICU(INPUT CAPTURE UNIT) which name is Capture Compare PWM pins (CCP) *
           * T0CCP0 ----> T3CCP1 & WT0CCP0 ----> WT5CCP1                                    *
           * Number 7(DIO_GPT_MODE) should be inserted at the 4-bit of the pin number       */
        case PORT_PIN_MODE_DIO_GPT:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
          |(DIO_GPT_MODE<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          break;
          /* If Non-Maskable Interrupt Mode, Number 8(NMI_MODE) at control register*/
        case PORT_PIN_MODE_NMI:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
          |(NMI_MODE<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          break;
          /* If Trace Data Mode, Number 14(TRACE_DATA_MODE) at Control reister */
        case PORT_PIN_MODE_TRACE_DATA:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
          |(TRACE_DATA_MODE<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          break;
          /* If QEI Mode, Number 6 at control register */
        case PORT_PIN_MODE_QEI:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
        ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
          |(QEI_MODE<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          break;
          /*                      If SPI Mode                           *
           * [PA2,PA3,PA4,PA5] used with SPI0                           *
           * [PF0,PF1,PF2,PF4] used with SPI1                           *
           * [PB4,PB5,PB6,PB7] used with SPI2                           *
           * [PD0,PD1,PD2,PD3] used with SPI3 and SPI1 so, we will      *
           * use PORTD with SPI3 and not SPI1 because we can use SPI3   *
           * only with PORTD but SPI1 can be used with PORTD            */
        case PORT_PIN_MODE_SPI:
          /* If [PD0 or PD1 or PD2 or PD3], number 1(SPI_MODE_1) at Control Register which is SPI3 */
          if((Pin == PORTD_PIN0_ID_INDEX) || (Pin == PORTD_PIN1_ID_INDEX) || (Pin == PORTD_PIN2_ID_INDEX) || (Pin == PORTD_PIN3_ID_INDEX))
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
          ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
            |(SPI_MODE_1<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          }
          /* If Something we will use numer 2(SPI_MODE_2) at Control Register which can be SPI0 or SPI1 or SPI2 based *
           * on the port number and pin number.                                                                       */
          else
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
          ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
            |(SPI_MODE_2<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          }
          break;
          /*                                 If PWM Mode                            *
           * PE4 Can be used with M1PWM2 & M0PWM4                                   *
           * PA6 Can be used with M1PWM2                                            *
           * So, we will use PE4 with M0PWM4                                        *
           * PE5 Can be used with M1PWM3 & M0PWM5                                   *
           * PA7 Can be used with M1PWM3                                            *
           * So, we will use PE5 with M0PWM5                                        *
           * PD0 Can be used with M0PWM6 & M1PWM0                                   *
           * PC4 Can be used with M0PWM6                                            *
           * So, we will use PD0 with M1PWM0                                        *
           * PD1 Can be used with M0PWM7 & M1PWM1                                   *
           * PC5 Can be used with M0PWM7                                            *
           * So, we will use PD1 with M1PWM1*/
        case PORT_PIN_MODE_PWM:
          /* Incase of PWM0 With [PB4 PB5 PB6 PB7 PC4 PC5 PE4 PE5], number 4(PWM0_MODE) at Control Register */
          if((Pin==PORTB_PIN4_ID_INDEX)||(Pin==PORTB_PIN5_ID_INDEX)||(Pin==PORTB_PIN6_ID_INDEX)||\
             (Pin==PORTB_PIN7_ID_INDEX)||(Pin==PORTC_PIN4_ID_INDEX)||(Pin==PORTC_PIN5_ID_INDEX)||\
             (Pin==PORTE_PIN4_ID_INDEX)||(Pin==PORTE_PIN5_ID_INDEX))
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
          ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
            |(PWM0_MODE<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          }
          /* Incase of PWM1 With [PA6 PA7 PD0 PD1 PF0 PF1 PF2 PF3], number 5(PWM1_MODE) at Control Register */
          else
          {
            *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
          ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
            |(PWM1_MODE<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          }
          break;
          /*                                 If UART Mode                            *
           * PC4 with U1Rx & U4Rx                                                    *
           * PB0 with U1Rx                                                           *
           * So, we will use PC4 with U4Rx                                           *
           * PC5 with U1Tx & U4Tx                                                    *
           * PB1 with U1Tx                                                           *
           * So, we will use PC5 with U4Tx                                           *
           * UART Number will be 1(UART_MODE) at Control Register                    */
        case PORT_PIN_MODE_UART:
          *(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)=\
          ((*(volatile uint32 *)((volatile uint8 *)Port_Base_Address_Ptr+PORT_CTL_REG_OFFSET)) &~(CONTROL_REGISTER_MASK<<((Port_Channels[Pin].pin_num)*BIT_SHIFT)))\
            |(UART_MODE<<((Port_Channels[Pin].pin_num)*BIT_SHIFT));
          break;
          /* Misra Rules */
        default :
          /* No Action Needed */
          break;
            
          
          
          
        }/* End of switch case inside the default */
        
        
        break;
      }/* End Of switch case (mode)*/
      
  }/* End Of if condition */
  /* Misra Rules */
  else
  {
    /* No Action Needed */
  }
  
}


#endif



