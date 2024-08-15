#include <avr/io.h>
#include <util/delay.h>
#include "timer.h"

// ****************************** Utility Functions ****************************** //
// You don't need to use these
// but they could help you in
// writing less code.

// Don't worry about how these work--just look below struct definitions to learn how to use them
struct B {
    bool operator()(int pad) { return PINB & (1 << pad); } 
    void operator()(int pad, bool value) { 
        PORTB = (value) ? PORTB | (1 << pad) : PORTB & ~(1 << pad);
    }
};

struct C {
    bool operator()(int pad) { return PINC & (1 << pad); } 
    void operator()(int pad, bool value) {
        PORTC = (value) ? PORTC | (1 << pad) : PORTC & ~(1 << pad);
    }
};

struct D {
    bool operator()(int pad) { return PIND & (1 << pad); } 
    void operator()(int pad, bool value) { 
        PORTD = (value) ? PORTD | (1 << pad) : PORTD & ~(1 << pad);
    }
};

// get function: fetches values from pin registers
// Sample Usage:
// get<B>(7) -- gets the value at PINB7 (returns 1 or 0)
// get<D>(5) -- gets the value at PIND5 (returns 1 or 0)
template <typename Group>
bool get(int pad) {
    return Group()(pad);
}

// set function: sets value at port regsiter
// Note: This function DOES NOT disturb the other bits in the register
// Sample Usage:
// set<C>(5, 0) -- sets PORTC5 to 0
// set<B>(6, 1) -- sets PORTC6 to 1
// set<D>(7, true) -- sets PORTD7 to 1
template <typename Group>
void set(int pad, bool value) {
    Group()(pad, value);
}
// ******************************************************************************* //





// 7segment display 0-9 (don't need A-F)
unsigned char seg7[] = {
//    gfedcba
    0b0111111, // 0
    0b0000110, // 1
    0b1011011, // 2
    0b1001111, // 3
    0b1100110, // 4
    0b1101101, // 5
    0b1111101, // 6
    0b0000111, // 7
    0b1111111, // 8
    0b1101111, // 9
};
// stepper motor phases
unsigned char phases[] = {
    0b0001, 
    0b0011, 
    0b0010, 
    0b0110, 
    0b0100, 
    0b1100, 
    0b1000, 
    0b1001
};

/* GLOBAL VARIABLES */
unsigned int gDistance  = 0; /* your global distance variable */
unsigned int gThreshold = 0; /* choose a threshold distance */

/* PIN DECLARATIONS */
// RGB LED Pins
#define RED_PIN    PORTB4
#define GREEN_PIN  PORTB3
// Sonar Pins
#define SONAR_TRIG  PORTC2
#define SONAR_ECHO  PINC1

const int min_dist = 5;
const int max_dist = 30;
const float duty_max = 15;
const float duty_map = duty_max / (max_dist - min_dist);
unsigned duty_green = 0;


void map_distance_to_color(int dist) {
    /* Implement a function that takes in the dist (in cm) and spits out two duty cycles for the Red and Green LEDs*/
}


// pings the SONAR sensor and returns the distance in cm
double read_sonar() {
    int count = 0;

    // generate 10us pulse
    set<C>(SONAR_TRIG, 1);
    _delay_us(10);
    set<C>(SONAR_TRIG, 0);

    // wait for the echo line to rise
    while (!get<C>(SONAR_ECHO));
    // wait for the echo line to drop then count us
    while (get<C>(SONAR_ECHO)) {
        count++;
        _delay_us(1);
    }

    // use speed of sound to determine distance
    return (0.034 / 2) * count;
}


// given some 8-bit data: dat, load it into the 8-bit shift register
// TODO! Translate each comment into code
void tx_shift(unsigned char dat) {
    // reset RCLK line
    for (int i = 7; i >= 0; --i) { // for each bit in dat
        // reset SRCLK line
        // extract i'th bit from dat and assign to SER line
        // _delay_us(10); // wait 10us---you're welcome to add a slight delay, although I didn't need it
        // set SRCLK line
    }
    // set RCLK line

}



int main() {

    DDRB = 0xFF;
    DDRC = 0xFD;
    DDRD = 0xFF;

    TimerSet(1);
    TimerOn();

    while (1) {

        // your tick function calls go here

        while (!TimerFlag);
        TimerFlag = false;
    }

    return 0;
}
