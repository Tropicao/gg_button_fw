#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "usbdrv/usbdrv.h"

#define F_CPU 12000000L
#include <util/delay.h>

USB_PUBLIC uchar usbFunctionSetup(uchar data[8])
{
    return 0; // do nothing for now
}

#define abs(x) ((x) > 0 ? (x) : -(x))

//Called by V-USB after device reset
void hadUsbReset() {
    int frameLength, targetLength = (unsigned)(1499 * (double)F_CPU / 10.5e6
            + 0.5);
    int bestDeviation = 9999;
    uchar trialCal, bestCal, step, region;


    //do a binary search in regions 0-127 and 128-255 to get optimum OSCCAL
    for(region = 0; region <= 1; region++) {
        frameLength = 0;
        trialCal = (region == 0) ? 0 : 128;

        for(step = 64; step > 0; step >>= 1) { 
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

    wdt_enable(WDTO_1S); // enable 1s watchdog timer

    usbInit();

    usbDeviceDisconnect(); // enforce re-enumeration
    for(i = 0; i<250; i++)
    { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    usbDeviceConnect();

    sei(); // Enable interrupts after re-enumeration

    while(1)
    {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
    }

    return 0;
}
