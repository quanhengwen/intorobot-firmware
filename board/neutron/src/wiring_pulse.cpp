/**
 ******************************************************************************
 * @file     : wiring_pulse.cpp
 * @author   : robin
 * @version  : V1.0.0
 * @date     : 6-December-2014
 * @brief    :
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */
//#include "wiring_pulse.h"
#include "variant.h"

/*********************************************************************************
  *Function      : uint32_t pulseIn(uint8_t pin, uint8_t state, uint32_t timeout)
  *Description  : Measures the length (in microseconds) of a pulse on the pin
  *Input           : pin:Data input port number
  				  state:HIGH or LOW
  				  timeout: Set up wait for timeout
  *Output         : none
  *Return         : return pluse width
  *author         : lz
  *date            : 6-December-2014
  *Others         : Works on pulses from 2-3 microseconds to 3 minutes in length, but must be called at least a few dozen microseconds
 				  before the start of the pulse.
**********************************************************************************/
uint32_t pulseIn(uint8_t pin, uint8_t state, uint32_t timeout)
{
#if 0
    /* timeout是超时 等不到测量的电平
       state 是需测量的电平 1或者 0
       timeout 超时时间 us    计数值 = timeout * SYSTEM_US_TICKS
       按指令计算时间
  	*/

  	STM32_Pin_Info p = PIN_MAP[pin];
	uint32_t width = 0;

	uint32_t numloops = 0;
	uint32_t maxloops = timeout * SYSTEM_US_TICKS;

	// wait for any previous pulse to end
	while (GPIO_ReadInputDataBit(p.gpio_peripheral, p.gpio_pin) == state) // 读到的电平是测量的电平 等待
	{
		if (numloops++ == maxloops)
		{
			return 0;
		}
	}

	// wait for the pulse to start
	while (GPIO_ReadInputDataBit(p.gpio_peripheral, p.gpio_pin) != state) // 读到的电平不是测试的电平 等待
	{
		if (numloops++ == maxloops)
		{
			return 0;
		}
	}

	// wait for the pulse to stop
	while (GPIO_ReadInputDataBit(p.gpio_peripheral, p.gpio_pin) == state)  // 经过等待之后 读到的电平是测试的电平 开始计时
	{
		if (numloops++ == maxloops)
		{
			return 0;
		}
		width++; // 28条指令时间
	}
	return ((width * 28)+148) / SYSTEM_US_TICKS; // 返回值 单位 us 148为误差
#endif
return 1;
}



#if 0
#include "wiring_private.h"
#include "pins_arduino.h"

/* Measures the length (in microseconds) of a pulse on the pin; state is HIGH
 * or LOW, the type of pulse to measure.  Works on pulses from 2-3 microseconds
 * to 3 minutes in length, but must be called at least a few dozen microseconds
 * before the start of the pulse.
 *
 * This function performs better with short pulses in noInterrupt() context
 */
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout)
{
	// cache the port and bit of the pin in order to speed up the
	// pulse width measuring loop and achieve finer resolution.  calling
	// digitalRead() instead yields much coarser resolution.
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	uint8_t stateMask = (state ? bit : 0);

	// convert the timeout from microseconds to a number of times through
	// the initial loop; it takes approximately 16 clock cycles per iteration
	unsigned long maxloops = microsecondsToClockCycles(timeout)/16;

	unsigned long width = countPulseASM(portInputRegister(port), bit, stateMask, maxloops);

	// prevent clockCyclesToMicroseconds to return bogus values if countPulseASM timed out
	if (width)
		return clockCyclesToMicroseconds(width * 16 + 16);
	else
		return 0;
}

/* Measures the length (in microseconds) of a pulse on the pin; state is HIGH
 * or LOW, the type of pulse to measure.  Works on pulses from 2-3 microseconds
 * to 3 minutes in length, but must be called at least a few dozen microseconds
 * before the start of the pulse.
 *
 * ATTENTION:
 * this function relies on micros() so cannot be used in noInterrupt() context
 */
unsigned long pulseInLong(uint8_t pin, uint8_t state, unsigned long timeout)
{
	// cache the port and bit of the pin in order to speed up the
	// pulse width measuring loop and achieve finer resolution.  calling
	// digitalRead() instead yields much coarser resolution.
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	uint8_t stateMask = (state ? bit : 0);

	// convert the timeout from microseconds to a number of times through
	// the initial loop; it takes 16 clock cycles per iteration.
	unsigned long numloops = 0;
	unsigned long maxloops = microsecondsToClockCycles(timeout);

	// wait for any previous pulse to end
	while ((*portInputRegister(port) & bit) == stateMask)
		if (numloops++ == maxloops)
			return 0;

	// wait for the pulse to start
	while ((*portInputRegister(port) & bit) != stateMask)
		if (numloops++ == maxloops)
			return 0;

	unsigned long start = micros();
	// wait for the pulse to stop
	while ((*portInputRegister(port) & bit) == stateMask) {
		if (numloops++ == maxloops)
			return 0;
	}
	return micros() - start;
}
#endif