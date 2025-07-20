#include "../include/main.h"
#include "hidapi.h"

#include <fcntl.h>
#include <libusb.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

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

struct ButtonMapping {
    unsigned char mask;
    int code;
    int payloadByte;
    int lastState;
};


struct ButtonMapping buttons[] = {
    {BUTTON_A, BTN_EAST, 3, 0},
    {BUTTON_B, BTN_SOUTH, 3, 0},
    {BUTTON_X, BTN_NORTH, 3, 0},
    {BUTTON_Y, BTN_WEST, 3, 0},
    {BUTTON_L, BTN_TL, 4, 0},
    {BUTTON_R, BTN_TR, 3, 0},
    {BUTTON_ZL, BTN_TL2, 4, 0},
    {BUTTON_ZR, BTN_TR2, 3, 0},
    {BUTTON_STICK_L, BTN_THUMBL, 4, 0},
    {BUTTON_STICK_R, BTN_THUMBR, 3, 0},
    {BUTTON_PLUS, BTN_START, 3, 0},
    {BUTTON_MINUS, BTN_SELECT, 4, 0},
    {BUTTON_CAPTURE, BTN_Z, 5, 0},
    {BUTTON_HOME, BTN_MODE, 5, 0}
};

struct DpadMapping {
    unsigned char mask;
    int payloadByte;
    int lastState;
};

enum {DPAD_DOWN = 0, DPAD_UP, DPAD_LEFT, DPAD_RIGHT};

struct DpadMapping dpad[] = {
    {BUTTON_DPAD_DOWN, 4, 0},
    {BUTTON_DPAD_UP, 4, 0},
    {BUTTON_DPAD_LEFT, 4, 0},
    {BUTTON_DPAD_RIGHT, 4, 0},
};

struct AnalogueTriggers {
    int payloadByte;
    int code;
    unsigned char lastState;
};

enum {LEFT_TRIGGER = 0, RIGHT_TRIGGER};
struct AnalogueTriggers triggers [] = {
    {LEFT_ANALOGUE_BYTE, ABS_Z, 0},
    {RIGHT_ANALOGUE_BYTE, ABS_RZ, 0}
};

struct AnalogueStick {
    int payloadStart;
    int length;
    int codeX;
    int codeY;
    uint16_t lastStateX;
    uint16_t lastStateY;
};

enum {LEFT_STICK = 0, RIGHT_STICK};
struct AnalogueStick sticks [] = {
    {LEFT_STICK_BYTE, 3, ABS_X, ABS_Y, 0, 0},
    {RIGHT_STICK_BYTE, 3, ABS_RX, ABS_RY, 0, 0}
};

#define NUM_STICKS 2
#define NUM_TRIGGERS 2

#define NUM_MAPPINGS (sizeof(buttons) / sizeof(buttons[0]))
#define NUM_DPAD 4

int emmitableButtons[] = {
    BTN_NORTH,
    BTN_SOUTH,
    BTN_EAST,
    BTN_WEST,
    BTN_TL,
    BTN_TR,
    BTN_TL2,
    BTN_TR2,
    BTN_THUMBL,
    BTN_THUMBR,
    BTN_START,
    BTN_SELECT,
    BTN_Z,
    BTN_MODE
};


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
static double phase = 0.0;
static double phase_inc = 2 * M_PI * FREQUENCY / SAMPLE_RATE; 

static struct libusb_transfer *xfer;

#define SAMPLES_PER_PACKET (PACKET_SIZE / BYTES_PER_SAMPLE)  
#define SAMPLES_PER_FRAME (SAMPLES_PER_PACKET * NUM_PACKETS)  

static void LIBUSB_CALL audio_transfer_cb(struct libusb_transfer *t) {
    if (t->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "Audio transfer error: %d\n", t->status);
        libusb_free_transfer(t);
        free(frame_buf);
        xfer = NULL;
        return;
    }

    int16_t *pcm = (int16_t*)frame_buf;
    for (int i = 0; i < SAMPLES_PER_FRAME; i++) {
        double value = sin(phase) * 0.3; 
        int16_t s = (int16_t)(value * 32767);
        pcm[i * CHANNELS] = s;
        pcm[i * CHANNELS + 1] = s;
        phase += phase_inc;
        if (phase >= 2*M_PI) phase -= 2*M_PI;
    }

    // Resubmit transfer
    if (libusb_submit_transfer(t) < 0) {
        fprintf(stderr, "Failed to resubmit audio transfer\n");
        libusb_free_transfer(t);
        free(frame_buf);
        xfer = NULL;
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

    res = libusb_claim_interface(devHandle, IF_CTRL);
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

    uint8_t unmute = 0x00;  // 0 = unmute
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
        NULL,
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
