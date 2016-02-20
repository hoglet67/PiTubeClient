// EconetClock.c Created on: Feb 19, 2016

//
// pwmoutput.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

//  Also elements Copyright (C) 2016 Simon R. Ellwood

#include <stdint.h>
#include "bcm2835.h"
#include "EconetClock.h"
#include "../bare-metal/rpi-gpio.h"

uint32_t read32(uint32_t nAddress)
{
  return *((uint32_t*) nAddress);
}

void write32(uint32_t nAddress, uint32_t nValue)
{
  *((uint32_t*) nAddress) = nValue;
}

void EconetClock_Start(uint32_t Speed, uint32_t Duty)
{
  write32(ARM_PWM_RNG1, Speed);
  write32(ARM_PWM_DAT1, Duty);
  write32(ARM_PWM_CTL, ARM_PWM_CTL_PWEN1);
  RPI_SetGpioHi (ECONET_ENABLE_PIN);
}

void EconetClock_Stop(void)
{
  RPI_SetGpioLo (ECONET_ENABLE_PIN);                        // Disable the enable output
  write32(ARM_PWM_CTL, 0);                                  // Disable the clock output
}

void EconetClock_Init(void)
{
  RPI_SetGpioLo (ECONET_ENABLE_PIN);                        // Disable the clock otput
  RPI_SetGpioPinFunction(ECONET_ENABLE_PIN, FS_OUTPUT);     // Econet Clock Enable PIN
  RPI_SetGpioPinFunction(ECONET_CLOCK_PIN, FS_ALT5);        // PWM0 - Econet Clock PIN

  //RPI_SetGpioPinFunction();
  //RPI_SetGpioPinFunction();
}
