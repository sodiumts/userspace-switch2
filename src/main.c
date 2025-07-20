#include "../include/main.h"
#include "../include/ringbuffer.h"

#include "pipewire/stream.h"
#include "spa/param/audio/raw.h"

#include <fcntl.h>
#include <libusb.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

const unsigned char INITCMD_03 [] = {0x03, 0x91, 0x00, 0x0D, 0x00, 0x08, 0x00, 0x00, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const unsigned char UNKNOWNCMD_07 [] = {0x07, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
const unsigned char UNKNOWNCMD_16 [] = {0x16, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
const unsigned char REQUEST_MAC_1501 [] = {0x15, 0x91, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
const unsigned char LTKREQUEST_1502 [] = {0x15, 0x91, 0x00, 0x02, 0x00, 0x11, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const unsigned char UNKNOWNCMD_1503 [] = {0x15, 0x91, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00};
const unsigned char UNKNOWNCMD_09 [] = {0x09, 0x91, 0x00, 0x07, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const unsigned char IMUCOMMAND_0C02 [] = {0x0C, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00};
const unsigned char OUTUNKNOWNCMD_11 [] = {0x11, 0x91, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00};
const unsigned char UNKNOWNCMD_0A [] = {0x0A, 0x91, 0x00, 0x08, 0x00, 0x14, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x35, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const unsigned char IMUCOMMAND_0C04 [] = {0x0C, 0x91, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00};
const unsigned char ENABLE_HAPTICS_03 [] = {0x03, 0x91, 0x00, 0x0A, 0x00, 0x04, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00};
const unsigned char OUTUNKNOWNCMD_10 [] = {0x10, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
const unsigned char OUTUNKNOWNCMD_01 [] = {0x01, 0x91, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00};
const unsigned char OUTUNKNOWNCMD_03 [] = {0x03, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00};
const unsigned char OUTUNKNOWNCMD_0A [] = {0x0A, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0x00};
const unsigned char SET_PLAYER_LED_09 [] = {0x09, 0x91, 0x00, 0x07, 0x00, 0x08, 0x00, 0x00, 0x01 /* led bitfield */, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
const unsigned char SET_FEATURE_FLAGS [] = {0x0c, 0x91, 0x01, 0x02, 0x00, 0x04, 0x00, 0x00, 0b00000000, 0x00, 0x00, 0x00};
const unsigned char CHANGE_FEATURE_FLAGS [] = {0x0c, 0x91, 0x01, 0x04, 0x00, 0x04, 0x00, 0x00, 0b00000000, 0x00, 0x00, 0x00};

typedef struct {
    const unsigned char *data;
    size_t length;
} Command;

Command commands [] = {
    {INITCMD_03, sizeof(INITCMD_03)},
    {UNKNOWNCMD_07, sizeof(UNKNOWNCMD_07)},
    {UNKNOWNCMD_16, sizeof(UNKNOWNCMD_16)},
    {REQUEST_MAC_1501, sizeof(REQUEST_MAC_1501)},
    {LTKREQUEST_1502, sizeof(LTKREQUEST_1502)},
    {UNKNOWNCMD_1503, sizeof(UNKNOWNCMD_1503)},
    {UNKNOWNCMD_09, sizeof(UNKNOWNCMD_09)},
    //{IMUCOMMAND_0C02, sizeof(IMUCOMMAND_0C02)},
    {OUTUNKNOWNCMD_11, sizeof(OUTUNKNOWNCMD_11)},
    {UNKNOWNCMD_0A, sizeof(UNKNOWNCMD_0A)},
    //{IMUCOMMAND_0C04, sizeof(IMUCOMMAND_0C04)},
    {ENABLE_HAPTICS_03, sizeof(ENABLE_HAPTICS_03)},
    {OUTUNKNOWNCMD_10, sizeof(OUTUNKNOWNCMD_10)},
    {OUTUNKNOWNCMD_01, sizeof(OUTUNKNOWNCMD_01)},
    {OUTUNKNOWNCMD_03, sizeof(OUTUNKNOWNCMD_03)},
    {OUTUNKNOWNCMD_0A, sizeof(OUTUNKNOWNCMD_0A)},
    {SET_PLAYER_LED_09, sizeof(SET_PLAYER_LED_09)},
    {SET_FEATURE_FLAGS, sizeof(SET_FEATURE_FLAGS)},
    {CHANGE_FEATURE_FLAGS, sizeof(CHANGE_FEATURE_FLAGS)}
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

#define SAMPLE_RATE 48000
#define FREQUENCY 440
#define CHANNELS 2
#define BYTES_PER_SAMPLE (CHANNELS * sizeof(int16_t))
#define MS_PER_TRANSFER 1 

#define FRAME_BYTES (NUM_PACKETS * PACKET_SIZE) 

#define PACKET_SIZE 192 
#define NUM_PACKETS 3

static unsigned char *frame_buf;

static struct libusb_transfer *xfer;

#define SAMPLES_PER_PACKET (PACKET_SIZE / BYTES_PER_SAMPLE)  
#define SAMPLES_PER_FRAME (SAMPLES_PER_PACKET * NUM_PACKETS)  

#define RINGBUF_MS      100
#define RINGBUF_SIZE  ((SAMPLE_RATE * CHANNELS * sizeof(int16_t) * RINGBUF_MS) / 1000)

struct audio_data {
    struct pw_context *context;
    struct pw_core *core;
    struct pw_stream *capture_stream;
    struct pw_main_loop *loop;
    ring_buffer_t *ringbuf;
};

struct audio_data global_audio;

void init_pipewire_capture(struct audio_data *data);

static void *pipewire_thread(void *arg) {
    struct audio_data *data = arg;
    pw_main_loop_run(data->loop);
    return NULL;
}

static void LIBUSB_CALL audio_transfer_cb(struct libusb_transfer *t) {
    struct audio_data *data = t->user_data;
    
    if (t->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "Audio transfer error: %d\n", t->status);
        return;
    }
    
    size_t read = ring_buffer_read(data->ringbuf, t->buffer, FRAME_BYTES);
    
    if (read < FRAME_BYTES) {
        // underrun
        memset(t->buffer + read, 0, FRAME_BYTES - read);
    }
    
    if (libusb_submit_transfer(t) < 0) {
        fprintf(stderr, "Failed to resubmit audio transfer\n");
    }
}

int controllerType = -1;

int send_init_sequence(libusb_device_handle *devHandle) {
    unsigned char response [64];
    for (size_t i = 0; i < NUM_COMMANDS; i++) {
        Command cmd = commands[i];
        int res = libusb_bulk_transfer(devHandle, INPUT_ENDPOINT,(unsigned char *) cmd.data, cmd.length, NULL, 500);
        if (res != LIBUSB_SUCCESS) {
            printf("Libusb device transfer error %s\n", libusb_strerror(res)); 
            return res;
        }
        
        res = libusb_bulk_transfer(devHandle, OUTPUT_ENDPOINT, response, sizeof(response), NULL, 500);
        if (res < 0 && res != LIBUSB_ERROR_TIMEOUT) {
            printf("Libusb device ack receive error %s\n", libusb_strerror(res)); 
            return res;
        }
    }
    return 0;
}

int handle_audio(libusb_device_handle *devHandle) {
    int res = libusb_claim_interface(devHandle, IF_CTRL);
    if (res != 0) {
        printf("Libusb audio control interface claim error %s\n", libusb_strerror(res));
    }
    res = libusb_claim_interface(devHandle, IF_STREAM);
    if (res != 0) {
        printf("Libusb audio stream interface claim error %s\n", libusb_strerror(res));
    }


    res = libusb_set_interface_alt_setting(devHandle, IF_STREAM, ALT_SETTING);
    if (res != 0) {
        printf("Libusb alt setting error %s\n", libusb_strerror(res));
    }

    init_pipewire_capture(&global_audio);

    pthread_t pw_thread;
    pthread_create(&pw_thread, NULL, pipewire_thread, &global_audio);

    uint32_t sample_rate = htole32(SAMPLE_RATE); 
    libusb_control_transfer(
        devHandle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
        SET_CUR,
        (SAMPLING_FREQ_CONTROL << 8),
        IF_STREAM,  
        (unsigned char*)&sample_rate,
        sizeof(sample_rate),
        1000
    );

    uint8_t unmute = 0x00;
    libusb_control_transfer(
        devHandle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
        SET_CUR,
        (MUTE_CONTROL << 8) | 0x01, 
        IF_STREAM,
        &unmute,
        sizeof(unmute),
        1000
    );

    xfer = libusb_alloc_transfer(NUM_PACKETS);
    if (!xfer) {
        fprintf(stderr, "Failed to allocate transfer\n");
        return 1;
    }

    frame_buf = malloc(FRAME_BYTES);
    if (!frame_buf) {
        fprintf(stderr, "Failed to allocate audio buffer\n");
        libusb_free_transfer(xfer);
        return 1;
    }

    memset(frame_buf, 0, FRAME_BYTES);

    libusb_fill_iso_transfer(
        xfer,
        devHandle,
        ISO_ES_OUT, 
        frame_buf,
        FRAME_BYTES,
        NUM_PACKETS,
        audio_transfer_cb,
        &global_audio,
        0 
    );
    for (int i = 0; i < NUM_PACKETS; i++) {
        xfer->iso_packet_desc[i].length = PACKET_SIZE;
    }

    int r = libusb_submit_transfer(xfer);
    if (r < 0) {
        fprintf(stderr, "libusb_submit_transfer failed: %s\n", libusb_error_name(r));
        libusb_free_transfer(xfer);
        free(frame_buf);
        return r;
    }

    printf("Audio transfer submitted successfully\n");

    while (xfer) {
        if (libusb_handle_events_completed(NULL, NULL) < 0)
            break;
    }
    libusb_release_interface(devHandle, IF_STREAM);
    libusb_release_interface(devHandle, IF_CTRL);
    
    return 0;
}


int init_usb_state(libusb_device *dev) { 
    libusb_device_handle *devHandle = NULL;

    int res = libusb_open(dev, &devHandle);
    if (res != 0) {
        printf("Libusb device open error %s\n", libusb_strerror(res));
        return res;
    }
    printf("Opened device handle\n");
    
    res = libusb_set_auto_detach_kernel_driver(devHandle, 1);
    if (res != LIBUSB_SUCCESS) {
        printf("Libusb auto detach error %s\n", libusb_strerror(res));
        return res; 
    }

    res = libusb_claim_interface(devHandle, USBINT_NUM);
    if (res != 0) {
        printf("Libusb device claim error %s\n", libusb_strerror(res));
        return res;
    }
    printf("Claimed device interface\n");

    res = send_init_sequence(devHandle);
    if (res != 0) {
        return res;
    }

    printf("Sent Init command sequence\n");


    res = libusb_release_interface(devHandle, USBINT_NUM);
    if (res != 0) {
        printf("Libusb device release error %s\n", libusb_strerror(res));
        return res;
    }
    printf("Released device interface\n");
    
    handle_audio(devHandle); 

    libusb_close(devHandle);
    printf("Closed device handle\n");
    return 0;
}

bool is_switch2(struct libusb_device_descriptor *desc) {
    if (desc->idVendor != NINTENDO_VENID)
        return false;

    switch (desc->idProduct) {
        case SW2PRO_PID:
            printf("Found Nintendo Switch 2 Pro Controller\n");
            controllerType = SW2PRO_PID;
            return true;
        case SW2GCN_PID:
            printf("Found Nintendo Switch 2 NSO Gamecube Controller\n");
            controllerType = SW2GCN_PID;
            return true;
    }
    return false; 
}

int emit_event(int fd, int type, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    return write(fd, &ev, sizeof(ev));
}

int emit_sync(int fd) {
    return emit_event(fd, EV_SYN, SYN_REPORT, 0);
}

static void on_process(void *userdata) {
    struct audio_data *data = userdata;
    struct pw_buffer *buf = pw_stream_dequeue_buffer(data->capture_stream);
    
    if (!buf) return;
    
    struct spa_buffer *spa_buf = buf->buffer;
    int16_t *samples = spa_buf->datas[0].data;
    size_t n_samples = spa_buf->datas[0].chunk->size / sizeof(int16_t);
    
    // Write to ring buffer
    ring_buffer_write(data->ringbuf, (uint8_t*)samples, n_samples * sizeof(int16_t));
    
    pw_stream_queue_buffer(data->capture_stream, buf);
}

static const struct pw_stream_events capture_events = {
    .process = on_process,
};

void init_pipewire_capture(struct audio_data *data) {
    pw_init(NULL, NULL);
    
    data->loop = pw_main_loop_new(NULL);
    data->context = pw_context_new(pw_main_loop_get_loop(data->loop), NULL, 0);
    data->core = pw_context_connect(data->context, NULL, 0);
    
    data->ringbuf = ring_buffer_create(RINGBUF_SIZE);
    
    struct pw_properties *stream_props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CLASS, "Audio/Sink",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE, "Music",
        PW_KEY_NODE_NAME, "switch2-audio-output",
        PW_KEY_NODE_DESCRIPTION, "Switch 2 Pro Controller Audio",
        "audio.channels", "2",
        "audio.position", "[FL,FR]",
        NULL
    );
    
    data->capture_stream = pw_stream_new_simple(
        pw_main_loop_get_loop(data->loop),
        "Switch2-Audio-Playback",
        stream_props,
        &capture_events,
        data
    );
    
    // Set audio format
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    struct spa_audio_info_raw info = {
        .format = SPA_AUDIO_FORMAT_S16,
        .rate = 48000,
        .channels = 2,
        .position = { SPA_AUDIO_CHANNEL_FL, SPA_AUDIO_CHANNEL_FR }
    };
    const struct spa_pod *params[1] = {
        spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info)
    };
    
    pw_stream_connect(data->capture_stream,
                      PW_DIRECTION_INPUT,
                      PW_ID_ANY,
                      PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_RT_PROCESS | PW_STREAM_FLAG_MAP_BUFFERS,
                      params, 1);
}

int main() {
    libusb_init_context(NULL, NULL, 0);

    libusb_device **devs;
    ssize_t count = libusb_get_device_list(NULL, &devs);


    libusb_device *found = NULL;

    for (ssize_t i = 0; i < count; i++) {
        libusb_device *dev = devs[i];
        struct libusb_device_descriptor desc;

        libusb_get_device_descriptor(dev, &desc);

        if (is_switch2(&desc)){
            found = dev;
            break;
        }
    }

    if(found) {
        int res = init_usb_state(found);
        if (res != 0) {
            libusb_free_device_list(devs, 1);
            libusb_exit(NULL);
            return res;
        }
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);

    return 0;
}
