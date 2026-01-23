#include "blinky_thread.h"

#define LED1  BSP_IO_PORT_00_PIN_06
#define LED2  BSP_IO_PORT_00_PIN_07
#define LED3  BSP_IO_PORT_00_PIN_08

void blinky_thread_entry(void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
    
    /* LED level: start with LOW (sáng, vì active low) */
    bsp_io_level_t pin_level = BSP_IO_LEVEL_LOW;
    
    while(1)
    {
        R_BSP_PinAccessEnable();
        
        /* Toggle all LEDs */
        R_BSP_PinWrite(LED1, pin_level);
        R_BSP_PinWrite(LED2, pin_level);
        R_BSP_PinWrite(LED3, pin_level);
        
        R_BSP_PinAccessDisable();
        
        /* Toggle level for next cycle */
        pin_level = (pin_level == BSP_IO_LEVEL_LOW) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW;
        
        /* Delay 500ms */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}