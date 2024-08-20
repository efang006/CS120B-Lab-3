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

/* 
    Implement a function that takes in the dist (in cm) and spits out two duty cycles for the Red and Green LEDs
    Distance > 15cm yield Green
    Distance < 7cm yield Red
    Distances in between yield Yellow
    (Distance) => 16 - 15 - 14 - 13 - 12 - 11 - 10 - 9 - 8 - 7 - 6
    (Green/Red) => 100/0 - 90/10 - 80/20 - 70/30 - 60/40 - 50/50 - 40/60 - 30/70 - 20/80 - 10/90 - 0/100
*/
void map_distance_to_color(int dist) 
{
    if (dist > 15) 
    {
        // Set to full Green
        duty_green = duty_max;
    } 
    else if (dist < 7) 
    {
        // Set to full Red
        duty_green = 0;
    } 
    else 
    {
        // Map distance to intermediate Yellow (mix of red and green)
        int range = 15 - 7; // The range between 7 and 15 cm
        int position = 16 - dist; // Position within the range

        // Linearly map position to duty cycles
        duty_green = (duty_max * (range - position)) / range;  // Scale down Green
    }
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
    set<B>(1, 0);
    for (int i = 7; i >= 0; --i) { // for each bit in dat
        // reset SRCLK line
        set<B>(2, 0);
        // extract i'th bit from dat and assign to SER line
        int bit = (dat >> i) & 1;
        set<B>(0, bit);
        _delay_us(10); // wait 10us---you're welcome to add a slight delay, although I didn't need it
        // set SRCLK line
        set<B>(2, 1);
    }
    // set RCLK line
    set<B>(1, 1);
}

typedef struct _task{
    // Tasks should have members that include: state, period,
    //a measurement of elapsed time, and a function pointer.
    signed char state; //Task's current state
    unsigned long period; //Task period
    unsigned long elapsedTime; //Time elapsed since last task tick
    int (*TickFct)(int); //Task tick function
} task;

// Other global variables
double distance = 0;
const double threshold = 5;

unsigned long LED_counter = 0;
unsigned long buzzer_counter = 0;

unsigned long RGB_counter = 0;

enum sevSegStates {Digit1, Digit2, Digit3} sevSegState;
enum sonarStates {Hold, Scan} sonarState;
enum LEDStates {LEDOff, LEDOn} LEDState;
enum buzzerStates {buzzerOff, buzzerOn} buzzerState;
enum RGBStates {RGBRed, RGBGreen} RGBState;

void TickFct_SevSeg()
{
    int distance_to_int = static_cast<int>(distance);
    int digit1 = distance_to_int % 10;
    int digit2 = (distance_to_int / 10) % 10;
    int digit3 = (distance_to_int / 100) % 10;

    switch(sevSegState)
    {
        case Digit1:
            sevSegState = Digit2;
            break;
        case Digit2:
            sevSegState = Digit3;
            break;
        case Digit3:
            sevSegState = Digit1;
            break;
        default:
            sevSegState = Digit1;
            break;
    }

    switch(sevSegState)
    {
        case Digit1:
            set<C>(PORTC3, 1);
            set<C>(PORTC4, 1);
            set<C>(PORTC5, 1);
            PORTD = seg7[digit1] << 1;
            // PORTD = seg7[3] << 1;
            set<C>(PORTC3, 0);
            break;
        case Digit2:
            set<C>(PORTC3, 1);
            set<C>(PORTC4, 1);
            set<C>(PORTC5, 1);
            PORTD = seg7[digit2] << 1;
            // PORTD = seg7[1] << 1;
            set<C>(PORTC4, 0);
            break;
        case Digit3:
            set<C>(PORTC3, 1);
            set<C>(PORTC4, 1);
            set<C>(PORTC5, 1);
            PORTD = seg7[digit3] << 1;
            // PORTD = seg7[9] << 1;
            set<C>(PORTC5, 0);
            break;
    }
}

void TickFct_Sonar()
{
    switch(sonarState)
    {
        case Hold:
            sonarState = Scan;
            break;
        case Scan:
            sonarState = Hold;
            break;
        default:
            sonarState = Hold;
            break;
    }

    switch(sonarState)
    {
        case Hold:
            break;
        case Scan:
            distance = read_sonar();
            break;
    }
}

void TickFct_LED()
{
    switch(LEDState)
    {
        case LEDOff:
            if(distance <= threshold)
            {
                LEDState = LEDOn;
                LED_counter = 0;
            }
            else
            {
                set<B>(PORTB5, 0);
            }
            break;
        case LEDOn:
            if(distance > threshold)
            {
                LEDState = LEDOff;
            }
            else
            {
                if(LED_counter <= 50)
                {
                    set<B>(PORTB5, 1);
                }
                else
                {
                    set<B>(PORTB5, 0);
                    if(LED_counter >= 500)
                    {
                        LED_counter = 0;
                    }
                }
            }
            break;
        default:
            LEDState = LEDOff;
            break;
    }

    LED_counter++;
}

void TickFct_Buzzer()
{
    switch(buzzerState)
    {
        case buzzerOff:
            if(distance <= threshold)
            {
                buzzerState = buzzerOn;
                buzzer_counter = 0;
            }
            else
            {
                set<C>(PORTC0, 0);
            }
            break;
        case buzzerOn:
            if(distance > threshold)
            {
                buzzerState = buzzerOff;
            }
            else
            {
                if(buzzer_counter <= 20)
                {
                    set<C>(PORTC0, 1);
                }
                else
                {
                    set<C>(PORTC0, 0);
                    if(buzzer_counter >= 400)
                    {
                        buzzer_counter = 0;
                    }
                }
            }
            break;
        default:
            buzzerState = buzzerOff;
            break;
    }

    buzzer_counter++;
}

void TickFct_RGB()
{
    map_distance_to_color(distance);
    switch(RGBState)
    {
        case RGBGreen:
            if(RGB_counter < duty_green)
            {
                set<B>(GREEN_PIN, 1);
            }
            else if(RGB_counter >= duty_green)
            {
                set<B>(GREEN_PIN, 0);
                RGBState = RGBRed;
                RGB_counter = 0;
            }
            break;
        case RGBRed:
            if(RGB_counter < (duty_max - duty_green))
            {
                set<B>(RED_PIN, 1);
            }
            else if(RGB_counter >= (duty_max - duty_green))
            {
                set<B>(RED_PIN, 0);
                RGBState = RGBGreen;
                RGB_counter = 0;
            }
            break;
    }
    RGB_counter++;
}

int main() 
{
    // set<B>(PORTB3, 1);

    unsigned long sevSeg_elapsedTime = 10;
    unsigned long sonar_elapsedTime = 75;
    const unsigned long timerPeriod = 1;

    DDRB = 0xFF;
    DDRC = 0xFD;
    DDRD = 0xFF;

    TimerSet(timerPeriod);
    TimerOn();

    sevSegState = Digit1;

    while (1) {
        // int sonarResult = (int) read_sonar();
        // distance = sonarResult;

        // your tick function calls go here
        // TickFct_SevSeg();
        // while (!TimerFlag);
        // TimerFlag = false;

        // TickFct_SevSeg();

        TickFct_LED();
        TickFct_Buzzer();
        TickFct_RGB();

        if(sevSeg_elapsedTime >= 10)
        {
            TickFct_SevSeg();
            sevSeg_elapsedTime = 0;
        }
        if(sonar_elapsedTime >= 75)
        {
            TickFct_Sonar();
            sonar_elapsedTime = 0;
        }

        while(!TimerFlag){}

        TimerFlag = false;

        sevSeg_elapsedTime += timerPeriod;
        sonar_elapsedTime += timerPeriod;
    }

    return 0;
}