#include <libusb/libusb.h>
#include <stdio.h>

int main() {
    libusb_device **devs;

    libusb_init_context(NULL, NULL, 0);
    

    libusb_get_device_list(NULL, &devs);

    for (int i = 0; devs[i]; i++) {
        libusb_device *dev = devs[i];
        struct libusb_device_descriptor desc;

        libusb_get_device_descriptor(dev, &desc);

        printf("Dev %04X - %04X", desc.idVendor, desc.idProduct);
    }

    libusb_free_device_list(devs, 1);

    libusb_exit(NULL);
}
