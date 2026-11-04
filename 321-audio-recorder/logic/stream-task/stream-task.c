#include "stream-task/stream-task.h"

#include <stddef.h>
#include "platform/platform.h"

static stream_state_t stream_state = STREAM_STATE_IDLE;
static stream_task_callback_t frame_callback = NULL;
static uint8_t stream_bits = 16;
static uint16_t stream_number = 0;
static uint32_t stream_frames = 0;
static uint32_t stream_samples = 0;

void stream_task_init(stream_task_callback_t on_frame)
{
    frame_callback = on_frame;
    stream_state = STREAM_STATE_IDLE;
}

void stream_task_handle(void)
{
    if (stream_state != STREAM_STATE_RUN)
    {
        return;
    }

    uint32_t samples;
    const uint8_t *data = platform_stream_next(&samples);
    if (data == NULL)
    {
        return;
    }

    stream_header_t header = {
        .sync_word = STREAM_SYNC_WORD,
        .number = stream_number,
        .count = (uint16_t)samples,
    };
    uint32_t bytes = samples * (stream_bits / 8u);
    frame_callback(&header, data, bytes);
    platform_stream_release();

    stream_number++;
    stream_frames++;
    stream_samples += samples;
}

bool stream_task_start(uint8_t bits)
{
    if (stream_state != STREAM_STATE_IDLE || frame_callback == NULL || bits != 16 ||
        !platform_stream_start(bits))
    {
        return false;
    }

    stream_bits = bits;
    stream_number = 0;
    stream_frames = 0;
    stream_samples = 0;
    stream_state = STREAM_STATE_RUN;
    return true;
}

void stream_task_stop(void)
{
    platform_stream_stop();
    stream_state = STREAM_STATE_IDLE;
}

stream_state_t stream_task_get_state(void)
{
    return stream_state;
}

uint8_t stream_task_get_bits(void)
{
    return stream_bits;
}

uint32_t stream_task_get_frames(void)
{
    return stream_frames;
}

uint32_t stream_task_get_samples(void)
{
    return stream_samples;
}

uint32_t stream_task_get_overruns(void)
{
    return platform_stream_overruns();
}
