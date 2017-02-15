/**
 * @brief Code used to check if the ATTiny has been properly flashed
 * When flashed, juste connect a led with a 1k resistor on pin 3
 * The led should blink with 1s cycle
 */
#include <avr/io.h>

#define F_CPU 1000000UL
#include <util/delay.h>

#define LED_BIT 8

int main() {
    DDRB |= LED_BIT;

    while(1)
    {
        PORTB |= LED_BIT;
        _delay_ms(500);
        PORTB &= ~LED_BIT;
        _delay_ms(500);
    }

    return 1;
}
