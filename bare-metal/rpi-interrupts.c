#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "startup.h"

#include "rpi-armtimer.h"
#include "rpi-base.h"
#include "rpi-gpio.h"
#include "rpi-interrupts.h"

#include "tube-isr.h"

// From here: https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=53862
void reboot_now(void)
{
  const int PM_PASSWORD = 0x5a000000;
  const int PM_RSTC_WRCFG_FULL_RESET = 0x00000020;
  unsigned int *PM_WDOG = (unsigned int *) (PERIPHERAL_BASE + 0x00100024);
  unsigned int *PM_RSTC = (unsigned int *) (PERIPHERAL_BASE + 0x0010001c);
  
  // timeout = 1/16th of a second? (whatever)
  *PM_WDOG = PM_PASSWORD | 1;
  *PM_RSTC = PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET;
  while(1);
}

/** @brief The BCM2835/6 Interupt controller peripheral at it's base address */
static rpi_irq_controller_t* rpiIRQController =
        (rpi_irq_controller_t*)RPI_INTERRUPT_CONTROLLER_BASE;

/**
    @brief Return the IRQ Controller register set
*/
rpi_irq_controller_t* RPI_GetIrqController( void )
{
    return rpiIRQController;
}

/**
    @brief The IRQ Interrupt handler

    This handler is run every time an interrupt source is triggered. It's
    up to the handler to determine the source of the interrupt and most
    importantly clear the interrupt flag so that the interrupt won't
    immediately put us back into the start of the handler again.
*/
void __attribute__((interrupt("IRQ"))) interrupt_vector(void)
{
    static int lit = 0;

    if (RPI_GetGpio()->GPEDS0 & RST_PIN_MASK) {
      RPI_GetGpio()->GPEDS0 = RST_PIN_MASK;
      //printf("RST PIN!!!\r\n");
      reboot_now();
    }


    /* Clear the ARM Timer interrupt */
    /* TODO: We should check it was the timer interrupt.... */
    RPI_GetArmTimer()->IRQClear = 1;

    /* Flip the LED */
    if( lit )
    {
        LED_OFF();
        lit = 0;
    }
    else
    {
        LED_ON();
        lit = 1;
    }

    if (RPI_GetGpio()->GPEDS0 & IRQ_PIN_MASK) {
      RPI_GetGpio()->GPEDS0 = IRQ_PIN_MASK;
      //printf("IRQ PIN!!!\r\n");
      TubeInterrupt();
    }

    if (RPI_GetGpio()->GPEDS0 & NMI_PIN_MASK) {
      //RPI_GetGpio()->GPEDS0 = NMI_PIN_MASK;
      printf("NMI PIN!!!\r\n");
    }

}


/**
    @brief The FIQ Interrupt Handler

    The FIQ handler can only be allocated to one interrupt source. The FIQ has
    a full CPU shadow register set. Upon entry to this function the CPU
    switches to the shadow register set so that there is no need to save
    registers before using them in the interrupt.

    In C you can't see the difference between the IRQ and the FIQ interrupt
    handlers except for the FIQ knowing it's source of interrupt as there can
    only be one source, but the prologue and epilogue code is quite different.
    It's much faster on the FIQ interrupt handler.

    The prologue is the code that the compiler inserts at the start of the
    function, if you like, think of the opening curly brace of the function as
    being the prologue code. For the FIQ interrupt handler this is nearly
    empty because the CPU has switched to a fresh set of registers, there's
    nothing we need to save.

    The epilogue is the code that the compiler inserts at the end of the
    function, if you like, think of the closing curly brace of the function as
    being the epilogue code. For the FIQ interrupt handler this is nearly
    empty because the CPU has switched to a fresh set of registers and so has
    not altered the main set of registers.
*/
void __attribute__((interrupt("FIQ"))) fast_interrupt_vector(void)
{

}
