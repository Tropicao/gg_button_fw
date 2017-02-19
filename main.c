#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "usbdrv/usbdrv.h"

#define F_CPU 16500000L
#include <util/delay.h>

#define SWITCH_STATE        1

static uint8_t switchState = 0;

/*************************************************
 * USB HID descriptor
 ************************************************/

/*
 * Custom HID descriptor : we want to report one button state, sized on one bit
 * To understand the set fields, please report to :
 *  * USB HID device class definition 1.11 : http://www.usb.org/developers/hidpage/HID1_11.pdf
 *  * USB HID usage tables : http://www.usb.org/developers/hidpage/HID1_11.pdf
 *  * V-USB example on Automator : https://www.obdev.at/products/vusb/automator.html
 */
const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] =
{
    0x06, 0x00, 0xff,   /* USAGE_PAGE(Generic Desktop) */
    0x09, 0x01,         /* USAGE(Vendor Usage) */
    0xa1, 0x01,         /* COLLECTION (Application) */
    0x15, 0x00,         /* LOGICAL_MINIMUM(0) */
    0x25, 0x01,         /* LOGICAL_MAXIMUM(1) */
    0x75, 0x01,         /* REPORT_SIZE(1) */
    0x95, 0x01,         /* REPORT_COUNT(1) */
    0xc0,
};

/*************************************************
 * Switch state functions
 ************************************************/

static void switchInit()
{
    /* Set pin as input */
    DDRB &= ~(1 << PB3);
    /* Enable Pull Up resistor */
    PORTB |= (1 << PB3);
}

static void updateSwitchState()
{
    /*
     * When switch is idle, the switch pin is pulled up (so we read 1),
     * but we want to retain the idle state as 0
     */
    switchState = 1-((PINB & (1 << PB3)) >> PB3);
}


/*************************************************
 * USB functions
 ************************************************/

USB_PUBLIC uchar usbFunctionSetup(uchar data[8])
{
    usbRequest_t *rq = (void *)data;
    uint8_t ret = 0;
    switch (rq->bRequest)
    {
        case SWITCH_STATE:
            updateSwitchState();
            /* Send response */
            usbMsgPtr = (unsigned short)&switchState;
            ret = sizeof(switchState);
            break;
        default:
            break;
    }
    return ret;
}

#define abs(x) ((x) > 0 ? (x) : -(x))

//Called by V-USB after device reset
void hadUsbReset() {
    int frameLength, targetLength = (unsigned)(1499 * (double)F_CPU / 10.5e6
            + 0.5);
    int bestDeviation = 9999;
    uchar trialCal = 0, bestCal = 0, step = 0, region = 0;

    //do a binary search in regions 0-127 and 128-255 to get optimum OSCCAL
    for(region = 0; region <= 1; region++) {
        frameLength = 0;
        trialCal = (region == 0) ? 0 : 128;

        for(step = 64; step > 0; step >>= 1)
        {
            if(frameLength < targetLength) // true for initial iteration
                trialCal += step; // frequency too low
            else
                trialCal -= step; // frequency too high

            OSCCAL = trialCal;
            frameLength = usbMeasureFrameLength();

            if(abs(frameLength-targetLength) < bestDeviation) {
                bestCal = trialCal; // new optimum found
                bestDeviation = abs(frameLength -targetLength);
            }
        }
    }
    OSCCAL = bestCal;
}

int main()
{
    uchar i = 0;

    wdt_enable(WDTO_1S);

    usbInit();
    switchInit();

    usbDeviceDisconnect();
    for(i = 0; i<250; i++)
    {
        wdt_reset();
        _delay_ms(2);
    }
    usbDeviceConnect();

    /* Enable interrupts */
    sei();

    while(1)
    {
        wdt_reset();
        usbPoll();
    }

    return 0;
}
