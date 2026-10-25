#include "pico/stdlib.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "log.h"
#include "device.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"

int main(void)
{
    stdio_init_all();
    led_task_init();
    button_task_init();
    stdio_text_protocol_init();
    LOG_INF("%s %s\n", DEVICE_NAME, FIRMWARE_VERSION);
    profiling_init();

    while (1)
    {
        profiling_iteration();
        led_task_handle();
        button_task_handle();

        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
    }
}
