/**
 * Project: GG Button tester
 *
 * Based on Joonas Pihlajamaa work (http://codeandlife.com)
 * Copyright: (C) 2016 by Alexis Lothoré
 * License: GNU GPL v3 (see License.txt)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include <usb.h>

#ifdef DEBUG
    #define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
    #define DBG(...)
#endif

#define USB_DATA_OUT 2
#define USB_DATA_WRITE 3
#define USB_DATA_IN 4

#define GG_BUTTON_VID       (0x16C0)
#define GG_BUTTON_PID       (0x05DC)
#define GG_BUTTON_VSTRING   "tropicao-network.com"
#define GG_BUTTON_PSTRING   "GGButton"

#define SWITCH_STATE        1

#define PLAYER_COMMAND      "/usr/bin/aplay"
#define PLAYER_ARG          "sound.wav"

usb_dev_handle *handle = NULL;
uv_loop_t loop_data;

// used to get descriptor strings for device identification
static int usbGetDescriptorString(usb_dev_handle *dev, int index, int langid,
                                  char *buf, int buflen) {
    char buffer[256];
    int rval, i;

	// make standard request GET_DESCRIPTOR, type string and given index
    // (e.g. dev->iProduct)
	rval = usb_control_msg(dev,
        USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
        USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid,
        buffer, sizeof(buffer), 1000);

    if(rval < 0) // error
		return rval;

    // rval should be bytes read, but buffer[0] contains the actual response size
	if((unsigned char)buffer[0] < rval)
		rval = (unsigned char)buffer[0]; // string is shorter than bytes read

	if(buffer[1] != USB_DT_STRING) // second byte is the data type
		return 0; // invalid return type

	// we're dealing with UTF-16LE here so actual chars is half of rval,
	// and index 0 doesn't count
	rval /= 2;

	// lossy conversion to ISO Latin1
	for(i = 1; i < rval && i < buflen; i++) {
		if(buffer[2 * i + 1] == 0)
			buf[i-1] = buffer[2 * i];
		else
			buf[i-1] = '?'; // outside of ISO Latin1 range
	}
	buf[i-1] = 0;

	return i-1;
}

static usb_dev_handle * usbOpenDevice(int vendor, char *vendorName,
                                      int product, char *productName) {
	struct usb_bus *bus;
	struct usb_device *dev;
	char devVendor[256], devProduct[256];

	usb_dev_handle * handle = NULL;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	for(bus=usb_get_busses(); bus; bus=bus->next) {
		for(dev=bus->devices; dev; dev=dev->next) {
			if(dev->descriptor.idVendor != vendor ||
               dev->descriptor.idProduct != product)
                continue;

            // we need to open the device in order to query strings
            if(!(handle = usb_open(dev))) {
                DBG("Warning: cannot open USB device: %s\n", usb_strerror());
                continue;
            }

            // get vendor name
            if(usbGetDescriptorString(handle, dev->descriptor.iManufacturer, 0x0409, devVendor, sizeof(devVendor)) < 0) {
                fprintf(stderr,
                    "Warning: cannot query manufacturer for device: %s\n",
                    usb_strerror());
                usb_close(handle);
                continue;
            }

            // get product name
            if(usbGetDescriptorString(handle, dev->descriptor.iProduct,
               0x0409, devProduct, sizeof(devVendor)) < 0) {
                fprintf(stderr,
                    "Warning: cannot query product for device: %s\n",
                    usb_strerror());
                usb_close(handle);
                continue;
            }

            if(strcmp(devVendor, vendorName) == 0 &&
               strcmp(devProduct, productName) == 0)
                return handle;
            else
                usb_close(handle);
		}
	}

	return NULL;
}

static void _player_returned_cb(uv_process_t* ev, int64_t exit_code, int term_signal)
{
    if(exit_code)
    {
        DBG("Player finished with status %d", (int)exit_code);
    }
}

static void _main_timer_cb(uv_timer_t *handle_t)
{
    static char buffer[256]={0};
    static uint8_t firstPush = 1;
    uv_process_t player_handle;
    uv_process_options_t p_options;
    int nBytes = 0;
    nBytes = usb_control_msg(handle,
            USB_TYPE_VENDOR|USB_RECIP_DEVICE|USB_ENDPOINT_IN,
            SWITCH_STATE, 0, 0, buffer, sizeof(buffer), 5000);
    if(nBytes < 0)
    {
        DBG("USB error : %s\n", usb_strerror());
        return;
    }

    /*We are waiting for a single byte of data */
    if(buffer[0] == 1 && firstPush == 1)
    {
        firstPush = 0;
        DBG("Switch pushed !!!\n");
        memset((void *)&p_options, 0, sizeof(p_options));
        p_options.exit_cb = _player_returned_cb;
        p_options.file = PLAYER_COMMAND;
        p_options.args = (char **)&(PLAYER_ARG);
        if(uv_spawn(&loop_data, &player_handle, &p_options))
        {
            DBG("Cannot spawn player\n");
        }

    }
    else if(buffer[0] == 0)
    {
        firstPush = 1;
    }

}

int main(int argc, char **argv) {
    uv_timer_t main_timer;

	handle = usbOpenDevice(GG_BUTTON_VID, GG_BUTTON_VSTRING, GG_BUTTON_PID, GG_BUTTON_PSTRING);

	if(handle == NULL) {
		DBG("Could not find USB device!\n");
		exit(1);
	}
    else
    {
        DBG("GG Button found !\n");
    }

    uv_loop_init(&loop_data);
    uv_timer_init(&loop_data, &main_timer);
    uv_timer_start(&main_timer, _main_timer_cb, 0, 1);

    uv_run(&loop_data, UV_RUN_DEFAULT);

    DBG("Quitting main loop...");
    uv_loop_close(&loop_data);
	usb_close(handle);

	return 0;
}
