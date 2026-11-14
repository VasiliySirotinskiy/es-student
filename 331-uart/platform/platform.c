#include "platform/platform.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

static const uint LED_PIN = 25;
static const uint BUTTON_PIN = 15;

#define ADC_PIN 26
#define ADC_CHANNEL 0

// вывод, который через резистор 1 кОм ведёт линию RC-цепи
#define LINE_PIN 22

// выводы модуля UART0
#define UART0_TX_PIN 0
#define UART0_RX_PIN 1

// выводы модуля UART1
#define UART1_TX_PIN 8
#define UART1_RX_PIN 9

#define UART_BAUD 115200

// кто сейчас работает с АЦП; меняют и функции, и обработчики прерываний
typedef enum {
    ADC_FREE,
    ADC_BLOCK,
} adc_user_t;

static volatile adc_user_t adc_user;
static int block_channel;
static uint16_t block_samples[PLATFORM_BLOCK_MAX];
static uint32_t block_start_us;
static volatile uint32_t block_end_us;
static volatile bool block_ready;

static void adc_free_running(void)
{
    adc_select_input(ADC_CHANNEL);
    adc_fifo_setup(true, true, 1, false, false);
    adc_set_clkdiv(0);
}

// АЦП обратно в режим отсчётов по команде
static void adc_one_shot(void)
{
    adc_run(false);
    adc_fifo_drain();
    adc_fifo_setup(false, false, 0, false, false);
    adc_set_clkdiv(0);
}

// обработчик прерывания DMA_IRQ_1: блок захвачен
static void on_block_done(void)
{
    dma_channel_acknowledge_irq1(block_channel);
    block_end_us = time_us_32();
    adc_one_shot();
    block_ready = true;
    adc_user = ADC_FREE;
}

static uart_inst_t *uart_of(platform_uart_t uart)
{
    return uart == PLATFORM_UART0 ? uart0 : uart1;
}

// принятое обработчиком прерывания: блок длиннее очереди приёма
#define RX_BUFFER_SIZE 128

static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint32_t rx_count;

// обработчик UART0_IRQ: забирает из очереди приёма всё, что там есть
static void on_uart0_rx(void)
{
    while (uart_is_readable(uart0))
    {
        uint8_t byte = uart_get_hw(uart0)->dr & 0xff;
        if (rx_count < RX_BUFFER_SIZE)
        {
            rx_buffer[rx_count++] = byte;
        }
    }
}

void platform_init(void)
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, false);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);

    gpio_init(LINE_PIN);
    platform_line_set(PLATFORM_LINE_PUSH_PULL, false);

    // делитель, формат 8N1, очереди, включение модуля
    uart_init(uart0, UART_BAUD);
    platform_uart_loop_set(false);

    uart_init(uart1, UART_BAUD);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);

    irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx);
    irq_set_enabled(UART0_IRQ, true);

    // канал блока сообщает о конце через DMA_IRQ_1
    block_channel = dma_claim_unused_channel(true);
    dma_channel_set_irq1_enabled(block_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_1, on_block_done);
    irq_set_enabled(DMA_IRQ_1, true);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_button_read(void)
{
    return !gpio_get(BUTTON_PIN);
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return adc_read() * (3.3f / 4096.0f);
}

bool platform_block_start(uint32_t count)
{
    if (adc_user != ADC_FREE || count == 0 || count > PLATFORM_BLOCK_MAX)
    {
        return false;
    }

    adc_free_running();

    // 16-битные передачи из одного адреса в растущий, по запросу АЦП
    dma_channel_config cfg = dma_channel_get_default_config(block_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);

    block_ready = false;
    adc_user = ADC_BLOCK;

    // куда, откуда, сколько; true — запустить сразу
    dma_channel_configure(block_channel, &cfg, block_samples, &adc_hw->fifo, count, true);
    block_start_us = time_us_32();
    adc_run(true);
    return true;
}

bool platform_block_done(void)
{
    return block_ready;
}

const uint16_t *platform_block_samples(void)
{
    return block_samples;
}

uint32_t platform_block_time_us(void)
{
    return block_end_us - block_start_us;
}

void platform_line_set(platform_line_mode_t mode, bool high)
{
    if (mode == PLATFORM_LINE_PUSH_PULL)
    {
        // драйвер включён и сам задаёт уровень
        gpio_disable_pulls(LINE_PIN);
        gpio_put(LINE_PIN, high);
        gpio_set_dir(LINE_PIN, GPIO_OUT);
        return;
    }

    // открытый сток: в регистре выхода ноль, переключается только режим
    gpio_pull_up(LINE_PIN);
    gpio_put(LINE_PIN, 0);
    gpio_set_dir(LINE_PIN, high ? GPIO_IN : GPIO_OUT);
}

void platform_uart_loop_set(bool internal)
{
    uint function = internal ? GPIO_FUNC_NULL : GPIO_FUNC_UART;
    gpio_set_function(UART0_TX_PIN, function);
    gpio_set_function(UART0_RX_PIN, function);

    if (internal)
    {
        hw_set_bits(&uart_get_hw(uart0)->cr, UART_UARTCR_LBE_BITS);
    }
    else
    {
        hw_clear_bits(&uart_get_hw(uart0)->cr, UART_UARTCR_LBE_BITS);
    }
}

void platform_uart_clear(platform_uart_t uart)
{
    uart_inst_t *u = uart_of(uart);

    while (uart_is_readable(u))
    {
        (void)uart_get_hw(u)->dr;
    }

    // запись в UARTECR сбрасывает признаки ошибок
    uart_get_hw(u)->rsr = 0;

    if (uart == PLATFORM_UART0)
    {
        rx_count = 0;
    }
}

bool platform_uart_put(platform_uart_t uart, uint8_t byte)
{
    uart_inst_t *u = uart_of(uart);

    if (!uart_is_writable(u))
    {
        return false;
    }

    // место в очереди есть, поэтому uart_putc_raw() не ждёт
    uart_putc_raw(u, byte);
    return true;
}

bool platform_uart_busy(platform_uart_t uart)
{
    return uart_get_hw(uart_of(uart))->fr & UART_UARTFR_BUSY_BITS;
}

size_t platform_uart_read(platform_uart_t uart, uint8_t *data, size_t size)
{
    uart_inst_t *u = uart_of(uart);
    size_t n = 0;

    if (uart == PLATFORM_UART0)
    {
        uint32_t count = rx_count;
        for (uint32_t i = 0; i < count && n < size; i++)
        {
            data[n++] = rx_buffer[i];
        }
    }

    while (uart_is_readable(u) && n < size)
    {
        data[n++] = uart_get_hw(u)->dr & 0xff;
    }

    return n;
}

uint32_t platform_uart_errors(platform_uart_t uart)
{
    return uart_get_hw(uart_of(uart))->rsr & 0xf;
}

uint32_t platform_uart_baud_set(platform_uart_t uart, uint32_t baud)
{
    return uart_set_baudrate(uart_of(uart), baud);
}

void platform_uart_parity_set(platform_uart_t uart, platform_parity_t parity)
{
    static const uart_parity_t parities[] = {
        UART_PARITY_NONE,
        UART_PARITY_EVEN,
        UART_PARITY_ODD,
    };

    uart_set_format(uart_of(uart), 8, 1, parities[parity]);
}

void platform_uart_rx_irq_set(bool on)
{
    // прерывание по четырём байтам в очереди и по паузе на линии
    uart_set_irqs_enabled(uart0, on, false);
}
