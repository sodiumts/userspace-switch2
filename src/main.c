#include "../include/main.h"
#include "hidapi.h"

#include <fcntl.h>
#include <libusb.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <linux/uinput.h>

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
const unsigned char SET_FEATURE_FLAGS [] = {0x0c, 0x91, 0x01, 0x02, 0x00, 0x04, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00};
const unsigned char CHANGE_FEATURE_FLAGS [] = {0x0c, 0x91, 0x01, 0x04, 0x00, 0x04, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00};
const unsigned char VIBRO [] = {0x0A, 0x91, 0x01, 0x02, 0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00};

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
    {CHANGE_FEATURE_FLAGS, sizeof(CHANGE_FEATURE_FLAGS)},
    {VIBRO, sizeof(VIBRO)}
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))


uint16_t primary_cal[6];
uint16_t secondary_cal[6];
uint16_t trigger_cal[2];

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

void unpack_12bit_sequence(
    const uint8_t *in_bytes,
    size_t         byte_len,
    size_t         count,
    uint16_t      *out_vals
) {
    size_t   total_bits = count * 12;
    size_t   needed_bytes = (total_bits + 7) / 8;
    if (byte_len < needed_bytes) {
        fprintf(stderr, "Not enough input bytes: have %zu, need %zu\n",
                byte_len, needed_bytes);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        size_t bit_pos   = i * 12;
        size_t byte_idx  = bit_pos / 8;
        size_t bit_offset= bit_pos % 8;

        // grab the next 3 bytes (we need up to 12 + 7 = 19 bits)
        uint32_t chunk = in_bytes[byte_idx]
                       | (uint32_t)in_bytes[byte_idx + 1] << 8
                       | (uint32_t)in_bytes[byte_idx + 2] << 16;

        // shift down to align the 12-bit word, then mask
        out_vals[i] = (chunk >> bit_offset) & 0x0FFF;
    }
}

int read_spi_memory(libusb_device_handle *devHandle, int address, unsigned char size, uint16_t *output_cal) {
    unsigned char read_command [] = {0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, size, 0x7E, 0x00, 0x00, address & 0xFF, (address >> 8) & 0xFF, (address >> 16) & 0xFF, (address >> 24) & 0xFF};
    int res = libusb_bulk_transfer(devHandle, INPUT_ENDPOINT, read_command, sizeof(read_command) / sizeof(unsigned char), NULL, 500);
    if (res != LIBUSB_SUCCESS) {
        printf("Libusb device transfer error %s\n", libusb_strerror(res));
        return res;
    }
    
    unsigned char data[512];
    int actual_length;

    res = libusb_bulk_transfer(devHandle, OUTPUT_ENDPOINT, data, sizeof(data), &actual_length, 500);
    if (res == 0 && actual_length > 0) {
        const uint8_t *payload   = data + 16;
        size_t         payload_len = actual_length - 16;
        if (address == 0x13140) {
            output_cal[0] = payload[0];
            output_cal[1] = payload[1];
        } else {
            if (payload_len >= 0x28 + 9) {
                const uint8_t *cal_bytes = payload + 0x28;
                unpack_12bit_sequence(cal_bytes, /*=*/ 9, /*=*/ 6, output_cal);
            } else {
                printf("wrong payload len. \n");
                return 100;
            }
        }
        
    } else {
        printf("Error in bulk receive: %s\n", libusb_strerror(res));
        return res;
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

    // read primary stick calibration
    read_spi_memory(devHandle, 0x13080, 0x40, primary_cal);
    // read secondary stick calibration
    read_spi_memory(devHandle, 0x130C0, 0x40, secondary_cal);

    if (controllerType == SW2GCN_PID) {
        read_spi_memory(devHandle, 0x13140, 0x2, trigger_cal);
    }


    res = libusb_release_interface(devHandle, USBINT_NUM);
    if (res != 0) {
        printf("Libusb device release error %s\n", libusb_strerror(res));
        return res;
    }
    printf("Released device interface\n");

     
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

void apply_stick_calibration(int32_t *xval, int32_t *yval, uint16_t *calvals) {
    int32_t x = *xval - calvals[0];
    if (x < 0)
        x = (x * 32767) / calvals[4];
    else
        x = (x * 32767) / calvals[2];

    int32_t y = *yval - calvals[1];
    if (y < 0)
        y = (y * 32767) / calvals[5];
    else
        y = (y * 32767) / calvals[3];

    *xval = x;
    *yval = y;
}
int handle_controller_inputs() {
    int res = hid_init();
    if (res != 0) {
        wprintf(L"HID init error: %ls\n", hid_error(NULL));
        return 1;
    }
    printf("Init HID\n");

    hid_device* handle = hid_open(NINTENDO_VENID, controllerType, NULL);
    if (!handle) { 
        wprintf(L"HID init error: %ls\n", hid_error(NULL));
        return 1;
    }
    printf("");
    
    unsigned char data[64];
    
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    // DPad
    ioctl(fd, UI_SET_ABSBIT, ABS_HAT0X);
    ioctl(fd, UI_SET_ABSBIT, ABS_HAT0Y);

    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_RX);
    ioctl(fd, UI_SET_ABSBIT, ABS_RY);
    
    if (controllerType == SW2GCN_PID) {
        printf("Setting up triggers for gc controller\n");
        ioctl(fd, UI_SET_ABSBIT, ABS_Z);
        ioctl(fd, UI_SET_ABSBIT, ABS_RZ);
    }

    for (size_t i = 0; i < sizeof(emmitableButtons) / sizeof(emmitableButtons[0]); i++) {
        ioctl(fd, UI_SET_KEYBIT, emmitableButtons[i]);
    }

    struct uinput_user_dev uidev;

    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Translation gamepad");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x1234;
    uidev.id.product = 0x5678;
    uidev.id.version = 1;

    uidev.absmin[ABS_HAT0X] = -1;
    uidev.absmax[ABS_HAT0X] = 1;
    uidev.absmin[ABS_HAT0Y] = -1;
    uidev.absmax[ABS_HAT0Y] = 1;

    uidev.absmin[ABS_X] = -32768;
    uidev.absmax[ABS_X] = 32767;
    uidev.absmin[ABS_Y] = -32768;
    uidev.absmax[ABS_Y] = 32767;
    
    uidev.absmin[ABS_RX] = -32768;
    uidev.absmax[ABS_RX] = 32767;
    uidev.absmin[ABS_RY] = -32768;
    uidev.absmax[ABS_RY] = 32767;

    if (controllerType == SW2GCN_PID) {
        uidev.absmin[ABS_Z] = 0;
        uidev.absmax[ABS_Z] = 255;
        uidev.absmin[ABS_RZ] = 0;
        uidev.absmax[ABS_RZ] = 255;
    }

    write(fd, &uidev, sizeof(uidev));
    
    if(ioctl(fd, UI_DEV_CREATE) < 0) {
        printf("Failed to create device");
        close(fd);
        return 1;
    }
    while(true){
        res = hid_read(handle, data, sizeof(data));

        if (res == -1) {
            wprintf(L"HID init error: %ls\n", hid_error(NULL));
            close(fd);
            return 1; 
        }

        bool changed = false;
        for (size_t i = 0; i < NUM_MAPPINGS; i++) {
            struct ButtonMapping mapping = buttons[i];
            int pressed = !!(data[mapping.payloadByte] & mapping.mask);
            if (pressed != mapping.lastState) {
                emit_event(fd, EV_KEY, mapping.code, pressed);
                buttons[i].lastState = pressed;
                changed = true;
            }

            if (changed)
                emit_sync(fd);
        } 

        changed = false; 
        for (size_t i = 0; i < NUM_DPAD; i++) {
            struct DpadMapping mapping = dpad[i];
            int pressed = !!(data[mapping.payloadByte] & mapping.mask);
            if(pressed != mapping.lastState) {
                int hatx = 0;
                int haty = 0;

                dpad[i].lastState = pressed;        
                
                if (DPAD_LEFT == i || DPAD_RIGHT == i) {
                    if (dpad[DPAD_LEFT].lastState)
                        hatx = -1;
                    else if (dpad[DPAD_RIGHT].lastState)
                        hatx = 1;

                    emit_event(fd, EV_ABS, ABS_HAT0X, hatx);
                }

                if (DPAD_UP == i || DPAD_DOWN == i) {
                    if (dpad[DPAD_UP].lastState)
                        haty = -1;
                    else if (dpad[DPAD_DOWN].lastState)
                        haty = 1;

                    emit_event(fd, EV_ABS, ABS_HAT0Y, haty);
                } 
                changed = true;
            }


            if (changed)
                emit_sync(fd);
        }

        changed = false;
        for (size_t i = 0; i < NUM_STICKS; i++) {
            struct AnalogueStick map = sticks[i];
            uint8_t *stickData = data + map.payloadStart;
            // get 2 12 bit values out of 3 bytes
            uint16_t xval = stickData[0] | ((stickData[1] & 0x0F) << 8);
            uint16_t yval = (stickData[1] >> 4) | (stickData[2] << 4);
            
            
            int32_t xcal = xval;
            int32_t ycal = yval;
            
            // apply calibration
            if (i == 0) {
                apply_stick_calibration(&xcal, &ycal, primary_cal);
            } else { 
                apply_stick_calibration(&xcal, &ycal, secondary_cal);
            }

            if (xval != map.lastStateX) {
                //int32_t x_scaled = (int32_t)(xval - 2048) * 32767 / 2047;
                int32_t x_scaled = xcal;
                emit_event(fd, EV_ABS, map.codeX, x_scaled);
                sticks[i].lastStateX = xval;
                changed = true;
            }

            if (yval != map.lastStateY) {
                //int32_t y_scaled = (int32_t)(yval - 2048) * 32767 / 2047;
                int32_t y_scaled = ycal;
                y_scaled = -1 * y_scaled;
                emit_event(fd, EV_ABS, map.codeY, y_scaled);
                sticks[i].lastStateY = yval;
                changed = true;
            }

            if (changed)
                emit_sync(fd);
        }
        
        changed = false;
        // For gamecube controller
        if (controllerType == SW2GCN_PID) {
            for (size_t i = 0; i < NUM_TRIGGERS; i++) {
                struct AnalogueTriggers map = triggers[i];
                unsigned char pressedAmount = data[map.payloadByte];
                if (pressedAmount != map.lastState) {
                    triggers[i].lastState = pressedAmount;
                    
                    printf("%d\n", trigger_cal[0]);
                    printf("%d\n", trigger_cal[1]);
                    printf("press %d\n", pressedAmount);

                    float scale = (float)(pressedAmount - trigger_cal[i]) / (255 - trigger_cal[i]);
                    scale = scale < 0 ? 0 : scale;
                    int32_t cal = (int32_t)(scale * 255);
                    
                    printf("cal %d\n", cal);
                    // Remap 36-190 to 0-255
                    //int clamped = pressedAmount;
                    //if (pressedAmount < TRIGGER_RANGE_MIN) clamped = TRIGGER_RANGE_MIN;
                    //if (pressedAmount > TRIGGER_RANGE_MAX) clamped = TRIGGER_RANGE_MAX;
                    //int mapped = (clamped - TRIGGER_RANGE_MIN) * 255 / (TRIGGER_RANGE_MAX - TRIGGER_RANGE_MIN);
                    emit_event(fd, EV_ABS, map.code, cal);
                    changed = true;
                }

                if(changed)
                    emit_sync(fd);
            }
        }
    }

    return 0;
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

    if(controllerType != -1) {
        int res = handle_controller_inputs(); 
        if (res != 0) {
            return res;
        }
    } else {
        printf("No controller found\n");
    }
    return 0;
}
