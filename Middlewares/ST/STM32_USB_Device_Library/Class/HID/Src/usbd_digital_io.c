/**
  ******************************************************************************
  * @file    usbd_digital_io.c
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   This file provides the HID core functions.
  *
  * @verbatim
  *      
  *          ===================================================================      
  *                                HID Class  Description
  *          =================================================================== 
  *           This module manages the HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - The Mouse protocol
  *             - Usage Page : Generic Desktop
  *             - Usage : Digital IO
  *             - Collection : Application 
  *      
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *           
  *      
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "usbd_digital_io.h"
#include "gpio.h"
#include "stm32f4xx_hal_gpio.h"

/* Global variables */
HID_DIGITAL_IO_TypeDef digital_io;
HID_DIGITAL_IO_TypeDef digital_io_new_state;
HID_Digital_IO_Trigger digital_io_trigger;
Digital_IO_Change_Flag digital_io_change_flag;
Digital_IO_Report_Flag digital_io_report_flag;
ORDERED_ARRAY digital_io_switch_buffer;

/* Functions */

/**
  * @brief  USBH_HID_Digital_IO_Init
  *         The function init the HID digital IO.
  * @param  phost: Host handle
  * @retval USBH Status
  */
void USBD_HID_Digital_IO_Init(HID_DIGITAL_IO_TypeDef* digital_io_instance)
{
  uint8_t port_idx = 0, pin_idx = 0;

  // Initialize default values
  digital_io_instance->port_enabled_size = DIGITAL_MAX_PORT_NUM;

  // Step over all ports
  for(port_idx = 0; port_idx < DIGITAL_MAX_PORT_NUM; port_idx++)
  {
	  // Add default gpio settings
	  digital_io_instance->ports[port_idx].gpio_settings.Mode = GPIO_MODE_INPUT;
	  digital_io_instance->ports[port_idx].gpio_settings.Pull = GPIO_PULLDOWN;
	  digital_io_instance->ports[port_idx].gpio_settings.Pin = 0;

	  // Add default number and change state
	  digital_io_instance->ports[port_idx].pin_enabled_size = DIGITAL_MAX_PIN_NUM;
	  digital_io_instance->ports[port_idx]._changeIO = UNCHANGED;
	  digital_io_instance->ports[port_idx]._changePIN = UNCHANGED;

	  // Set pin specific default values
	  for (pin_idx = 0; pin_idx < DIGITAL_MAX_PIN_NUM; pin_idx++){
		  digital_io_instance->ports[port_idx].pins[pin_idx] = DIGITAL_PIN_LOW;
		  digital_io_instance->ports[port_idx].gpio_settings.Pin |= gpio_digital_pin[port_idx][pin_idx];
	  }
  }
}

/**
  * @brief  USBH_HID_Digital_IO_Init
  *         The function init the HID digital IO.
  * @param  phost: Host handle
  * @retval USBH Status
  */
void USBD_HID_Digital_IO_Reset_SwitchTrig(void)
{
  uint8_t port_idx = 0;

  // Reset switch buffer
  digital_io_switch_buffer.head_idx = 0;
  digital_io_switch_buffer.tail_idx = (DIGITAL_MAX_PORT_NUM - 1);

  // Unset trigger, change and report flag
  digital_io_trigger = DONTCARE;
  digital_io_change_flag = UNCHANGED;
  digital_io_change_flag = NO_REPORT;

  for(port_idx = 0; port_idx < DIGITAL_MAX_PORT_NUM; port_idx++)
  {
	  // Fill switch buffer
	  digital_io_switch_buffer.array[port_idx] = PORT_UNUSED;
  }
}

/**
  * @brief  USBH_HID_Digital_IO_Init
  *         The function init the HID digital IO.
  * @retval USBH Status
  */
void USBD_HID_Digital_IO_CreateReport(uint8_t* report)
{
  uint8_t port_idx = 0, pin_idx = 0, offset = 0, report_num = 0;

  // Clean old report
  report[0] = 0;
  report[1] = 0;
  report[2] = 0;
  report[3] = 0;

  // Step over all ports
  for(port_idx = 0; port_idx < DIGITAL_MAX_PORT_NUM; port_idx++)
  {
	  // Add IO directions to the report
	  // FORMAT: 1 byte (2 last bit reserved)
	  // XX543210, when 0...5 indicate the direction of the numbered ports
	  report[0] += (digital_io.ports[port_idx].gpio_settings.Mode << port_idx);

	  // Select actual report and add offset
	  if ((port_idx % 2) == 0)
	  {
		  report_num += 1;
		  offset = 0;
	  }
	  else
	  {
		  offset = 4;
	  }

	  // Add pin values to the report
	  // FORMAT: 3 byte -> 0000|1111, 2222|3333, 4444|5555 (4 pin / port)
	  // Numbers sign the actual port
	  // Step over all pins
	  for (pin_idx = 0; pin_idx < DIGITAL_MAX_PIN_NUM; pin_idx++){
		  report[report_num] += (digital_io.ports[port_idx].pins[pin_idx] << (pin_idx + offset));
	  }
  }

}

/**
  * @brief  USBH_HID_Digital_IO_Init
  *         The function init the HID digital IO.
  * @retval USBH Status
  */
void USBD_HID_Digital_IO_Read(void)
{
  uint8_t port_idx = 0, pin_idx = 0;

  // Step over all ports
  for(port_idx = 0; port_idx < DIGITAL_MAX_PORT_NUM; port_idx++)
  {
	  for (pin_idx = 0; pin_idx < DIGITAL_MAX_PIN_NUM; pin_idx++){
		  digital_io.ports[port_idx].pins[pin_idx] = GPIO_Read_DIGITAL_IO(port_idx, pin_idx);
	  }
  }
}


/**
  * @brief  USBH_HID_Digital_IO_Init
  *         The function init the HID digital IO.
  * @retval USBH Status
  */
void USBD_HID_Digital_IO_Set_Changes(uint8_t* output_buff)
{
	volatile uint8_t port_idx = 0, pin_idx = 0;
	volatile Digital_IO_Pull_Info temp_pull = NOPULL;
	volatile Digital_IO_Mode_Info temp_mode = INPUT;
	volatile uint8_t temp_pins = 0;
	volatile Digital_IO_Change_Flag temp_change = UNCHANGED;

	// Step over all ports (all ports have a byte in the output buffer, first four bytes for settings)
	for(port_idx = 0; port_idx < DIGITAL_MAX_PORT_NUM; port_idx++)
	{
		// CHANGE: 1st bit in the byte store the usage of port (if not used -> default)
		temp_change = (output_buff[port_idx] & 0x01);
		if (temp_change == CHANGED)
		{
			// MODE: 2nd bit in the byte contain IO direction (mask it)
			temp_mode = (output_buff[port_idx] & 0x02);
			digital_io_new_state.ports[port_idx].gpio_settings.Mode = (temp_mode == OUTPUT) ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;

			// PULL: 3nd and 4nd bits in the byte together define pull type (mask these)
			temp_pull = (output_buff[port_idx] & 0x0C);
			switch(temp_pull)
			{
				case NOPULL:
					digital_io_new_state.ports[port_idx].gpio_settings.Pull = GPIO_NOPULL;
					break;
				case PULLDOWN:
					digital_io_new_state.ports[port_idx].gpio_settings.Pull = GPIO_PULLDOWN;
					break;
				case PULLUP:
					digital_io_new_state.ports[port_idx].gpio_settings.Pull = GPIO_PULLUP;
					break;
				default:
					digital_io_new_state.ports[port_idx].gpio_settings.Pull = GPIO_NOPULL;
					break;
			} // switch (pull)

			// Update _changeIO flag based on PULL and MODE settings
			if ( digital_io_new_state.ports[port_idx].gpio_settings.Mode != digital_io.ports[port_idx].gpio_settings.Mode ||
				 digital_io_new_state.ports[port_idx].gpio_settings.Pull != digital_io.ports[port_idx].gpio_settings.Pull )
			{
				digital_io_new_state.ports[port_idx]._changeIO = CHANGED;

				// Add OUT -> IN higher priority than IN -> OUT (avoid to connecting two outputs together)
				if (digital_io_new_state.ports[port_idx].gpio_settings.Mode == GPIO_MODE_OUTPUT_PP)
				{
					digital_io_switch_buffer.array[digital_io_switch_buffer.tail_idx] = port_idx;
					digital_io_switch_buffer.tail_idx--;
				}
				else
				{
					digital_io_switch_buffer.array[digital_io_switch_buffer.head_idx] = port_idx;
					digital_io_switch_buffer.head_idx++;
				}
			} // if (_changeIO)
			else
			{
				digital_io_new_state.ports[port_idx]._changeIO = UNCHANGED;
			} // else (_changeIO)


			// Set values only OUTPUT ports
			if (temp_mode == OUTPUT)
			{
				// Read pin values and shift down these (all ports have a byte in the output buffer, last four bytes for values)
				temp_pins = (output_buff[port_idx] & (0x10 | 0x20 | 0x40 | 0x80)) >> 4;

				// Step over all pins
				for (pin_idx = 0; pin_idx < DIGITAL_MAX_PIN_NUM; pin_idx++)
				{
					// Mask actual pin values
					digital_io_new_state.ports[port_idx].pins[pin_idx] = (temp_pins >> pin_idx) & 0x01;

					// Update _changePIN flag based on pin values and _changeIO state (true, when old is INPUT or (OUTPUT and pin values different))
					if (
							(
								(digital_io_new_state.ports[port_idx].pins[pin_idx] != digital_io.ports[port_idx].pins[pin_idx]) &&
								(digital_io_new_state.ports[port_idx]._changeIO == UNCHANGED)
							)
							||  (digital_io_new_state.ports[port_idx]._changeIO == CHANGED)
						)
					{
						digital_io_new_state.ports[port_idx]._changePIN = CHANGED;
					} // if (_changePIN)
					else
					{
						digital_io_new_state.ports[port_idx]._changePIN = UNCHANGED;
					} // else (_changePIN)
				}
			}
			// The _changePIN flag may different if the previous setting was OUTPUT
			else
			{
				digital_io_new_state.ports[port_idx]._changePIN = UNCHANGED;
			}// if (pin)
		} // if (usage)
		// Use default settings
		else
		{
		  // Add default gpio settings
		  digital_io_new_state.ports[port_idx].gpio_settings.Mode = GPIO_MODE_INPUT;
		  digital_io_new_state.ports[port_idx].gpio_settings.Pull = GPIO_PULLDOWN;
		  digital_io_new_state.ports[port_idx].gpio_settings.Pin = 0;

		  // Add default number and change state
		  digital_io_new_state.ports[port_idx].pin_enabled_size = DIGITAL_MAX_PIN_NUM;
		  digital_io_new_state.ports[port_idx]._changeIO = UNCHANGED;
		  digital_io_new_state.ports[port_idx]._changePIN = UNCHANGED;

		  // Set pin specific default values
		  for (pin_idx = 0; pin_idx < DIGITAL_MAX_PIN_NUM; pin_idx++){
			  digital_io_new_state.ports[port_idx].pins[pin_idx] = DIGITAL_PIN_LOW;
			  digital_io_new_state.ports[port_idx].gpio_settings.Pin |= gpio_digital_pin[port_idx][pin_idx];
		  }
		} // else (usage)
	} // for (ports)
}

/**
  * @brief  USBH_HID_Digital_IO_Init
  *         The function init the HID digital IO.
  * @retval USBH Status
  */
void USBD_HID_Digital_IO_Trigger (uint8_t* output_buff)
{
	if (output_buff[0] == 0xfe)
	{
		digital_io_trigger = TRIGGERED;
	}
	else
	{
		digital_io_trigger = DONTCARE;
	}
}

/**
  * @brief  USBH_HID_Digital_IO_Init
  *         The function init the HID digital IO.
  * @retval USBH Status
  */
void USBD_HID_Digital_IO_SwitchPorts(void)
{
	uint8_t port_idx = 0, pin_idx = 0;
	// Copy changes to the digital IO instance
	digital_io = digital_io_new_state;

	// Set changes physically

	// Fist step: OUT -> IN changes
	while(digital_io_switch_buffer.head_idx != 0)
	{
		digital_io_switch_buffer.head_idx --;
		USBD_HID_Digital_IO_GPIO_Setup (digital_io_switch_buffer.head_idx);

	}

	// Second step: IN -> OUT changes
	while(digital_io_switch_buffer.tail_idx != (DIGITAL_MAX_PORT_NUM - 1))
	{
		digital_io_switch_buffer.tail_idx ++;
		USBD_HID_Digital_IO_GPIO_Setup (digital_io_switch_buffer.tail_idx);
	}

	// Third step: set/unset gpio values
	for (port_idx = 0; port_idx < DIGITAL_MAX_PORT_NUM; port_idx ++)
	{
		if(digital_io.ports[port_idx]._changePIN == CHANGED)
		{
			for (pin_idx = 0; pin_idx < DIGITAL_MAX_PIN_NUM; pin_idx ++)
			{
				GPIO_Write_DIGITAL_IO(port_idx, pin_idx, digital_io.ports[port_idx].pins[pin_idx]);
			}
		}
		// Reset default values
		digital_io.ports[port_idx]._changePIN = UNCHANGED;
		digital_io.ports[port_idx]._changeIO = UNCHANGED;
	}



	// Finish with reset the worn out buffers
	//USBD_HID_Digital_IO_Init(digital_io_new_state);
	//USBD_HID_Digital_IO_Reset_SwitchTrig();
}

/**
  * @brief  USBH_HID_Digital_IO_Init
  *         The function init the HID digital IO.
  * @retval USBH Status
  */
void USBD_HID_Digital_IO_GPIO_Setup (uint8_t idx)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_TypeDef* temp_portdef = gpio_digital_port[digital_io_switch_buffer.array[idx]][0];
	GPIO_InitStruct.Pin = digital_io.ports[digital_io_switch_buffer.array[idx]].gpio_settings.Pin;
	GPIO_InitStruct.Mode = digital_io.ports[digital_io_switch_buffer.array[idx]].gpio_settings.Mode;
	GPIO_InitStruct.Pull = digital_io.ports[digital_io_switch_buffer.array[idx]].gpio_settings.Pull;
	HAL_GPIO_Init(temp_portdef, &GPIO_InitStruct);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
