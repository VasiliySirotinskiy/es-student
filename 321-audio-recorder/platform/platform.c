#include "platform/platform.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

// вход АЦП: вывод GP26, канал 0
#define ADC_PIN 26
#define ADC_CHANNEL 0

// встроенный датчик температуры — канал 4
#define ADC_TEMP_CHANNEL 4

// вывод, через резистор заряжающий конденсатор RC-цепи
#define CHARGE_PIN 22

// опорное напряжение АЦП на плате Pico и число уровней 12-разрядного кода
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

// частота clk_adc, от неё делитель отсчитывает шаг
#define ADC_CLOCK_HZ 48000000.0f

static const uint LED_PIN = 25;
static const uint BUTTON_PIN = 15;

static repeating_timer_t blink_timer;
static bool blink_running;
static volatile bool blink_on;
static volatile uint32_t blink_toggles;

// кто сейчас работает с АЦП; меняют и функции, и обработчики прерываний
typedef enum {
    ADC_FREE,
    ADC_CAPTURE,
    ADC_BLOCK,
} adc_user_t;

static volatile adc_user_t adc_user;

static repeating_timer_t capture_timer;
static uint16_t capture_samples[PLATFORM_CAPTURE_MAX];
static uint32_t capture_times[PLATFORM_CAPTURE_MAX];
static uint32_t capture_count;
static uint32_t capture_period_us;
static volatile uint32_t capture_index;
static volatile uint32_t capture_handler_us;

static int block_channel;
static uint16_t block_samples[PLATFORM_BLOCK_MAX];
static uint32_t block_start_us;
static volatile uint32_t block_end_us;
static volatile bool block_ready;

// АЦП в непрерывном режиме с шагом 44,1 кГц, буфер АЦП с запросом DMA
// на каждый отсчёт
static void adc_free_running(void)
{
    adc_select_input(ADC_CHANNEL);
    adc_fifo_setup(true, true, 1, false, false);
    adc_set_clkdiv(ADC_CLOCK_HZ / PLATFORM_SAMPLE_RATE_HZ - 1);
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

// обработчик будильника захвата: время и отсчёт, затем время своей работы
static bool on_capture(repeating_timer_t *timer)
{
    (void)timer;
    uint32_t start_us = time_us_32();
    capture_times[capture_index] = start_us;
    capture_samples[capture_index] = adc_read();
    capture_index++;
    capture_handler_us += time_us_32() - start_us;

    if (capture_index < capture_count)
    {
        return true;
    }

    // захват окончен: АЦП свободен, будильник снимается
    adc_user = ADC_FREE;
    return false;
}

// обработчик будильника индикатора
static bool on_blink(repeating_timer_t *timer)
{
    (void)timer;
    blink_on = !blink_on;
    gpio_put(LED_PIN, blink_on);
    blink_toggles++;

    // true — вызвать снова через тот же период
    return true;
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

    gpio_init(CHARGE_PIN);
    gpio_put(CHARGE_PIN, false);
    gpio_set_dir(CHARGE_PIN, GPIO_OUT);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_set_temp_sensor_enabled(true);

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

bool platform_blink_start(uint32_t period_ms)
{
    if (period_ms < 10 || period_ms > 60000)
    {
        return false;
    }

    platform_blink_stop();
    blink_toggles = 0;

    // минус перед периодом — отсчитывать от начала прошлого вызова
    blink_running = add_repeating_timer_ms(-(int32_t)period_ms, on_blink, NULL, &blink_timer);
    return blink_running;
}

uint32_t platform_blink_stop(void)
{
    if (blink_running)
    {
        cancel_repeating_timer(&blink_timer);
        blink_running = false;
    }

    blink_on = false;
    gpio_put(LED_PIN, 0);
    return blink_toggles;
}

void platform_charge_set(bool on)
{
    gpio_put(CHARGE_PIN, on);
}

bool platform_button_read(void)
{
    return !gpio_get(BUTTON_PIN);
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}

// температура кристалла по встроенному датчику, градусы Цельсия
float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    float voltage = adc_read() * ADC_VREF_V / ADC_LEVELS;
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

bool platform_adc_busy(void)
{
    return adc_user != ADC_FREE;
}

bool platform_capture_start(uint32_t count, uint32_t period_us)
{
    if (adc_user != ADC_FREE || count == 0 || count > PLATFORM_CAPTURE_MAX || period_us == 0)
    {
        return false;
    }

    adc_select_input(ADC_CHANNEL);
    capture_count = count;
    capture_period_us = period_us;
    capture_index = 0;
    capture_handler_us = 0;
    adc_user = ADC_CAPTURE;

    if (!add_repeating_timer_us(-(int64_t)period_us, on_capture, NULL, &capture_timer))
    {
        capture_count = 0;
        adc_user = ADC_FREE;
        return false;
    }
    return true;
}

bool platform_capture_done(void)
{
    return capture_count != 0 && capture_index == capture_count && adc_user == ADC_FREE;
}

void platform_capture_get(platform_capture_t *capture)
{
    *capture = (platform_capture_t){0};
    if (!platform_capture_done())
    {
        return;
    }

    capture->samples = capture_samples;
    capture->count = capture_count;
    capture->period_us = capture_period_us;
    if (capture_count == 1)
    {
        return;
    }

    uint32_t interval_us = capture_times[1] - capture_times[0];
    capture->min_interval_us = interval_us;
    capture->max_interval_us = interval_us;
    for (uint32_t i = 2; i < capture_count; i++)
    {
        interval_us = capture_times[i] - capture_times[i - 1];
        if (interval_us < capture->min_interval_us)
        {
            capture->min_interval_us = interval_us;
        }
        if (interval_us > capture->max_interval_us)
        {
            capture->max_interval_us = interval_us;
        }
    }

    uint32_t elapsed_us = capture_times[capture_count - 1] - capture_times[0];
    if (elapsed_us != 0)
    {
        capture->handler_percent = capture_handler_us * 100.0f / elapsed_us;
    }
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
