/**
 * This code generates a simple VGA signal that shows red, green, and blue bars across the screen.
 * 
 * A VGA signal is relatively simple. I'm only using 6 pins: ground, vsync, hsync, red, green, blue.
 * VGA draws the picture from top to bottom, left to right. I'm using a standard 640x480 resolution.
 * The monitor draws 480 vertical lines, each of which take about 32us.
 * The signal also contains 32 additional blank lines called the vertical blanking interval at the beginning.
 * 
 * The vsync pin is used to send a pulse every time a new frame should start.
 * The standard is 60hz, so it is set to pulse for 64us every 1/60th of a second.
 * This along with the vertical blanking interval helps the monitor know where the picture should start.
 * 
 * The hsync pin is used to send a pulse every time a new line should start.
 * Since we have 480 lines for 480 vertical pixels, we pulse once every line (32us).
 * 
 * The red, green, and blue pins are used to control the color the monitor draws.
 * Their voltage corresponds to the color intensity of whatever pixel we happen to be drawing.
 * As the monitor scans from left to right during a line, it reads the analog RGB values and draws them on the screen.
 * 
 * I used this wikipedia page to gather all of this info: https://en.wikipedia.org/wiki/Video_Graphics_Array
 */
 
#include <avr/sleep.h>

//Macro for converting define to string
#define _STR(X) #X
#define STR(X) _STR(X)

#include <avr/sleep.h>

//Macro for converting define to string
#define _STR(X) #X
#define STR(X) _STR(X)

#define VSYNC_PIN 11
#define HSYNC_PIN 9
#define RED_PIN1 24
#define RED_PIN2 25
#define GREEN_PIN1 26
#define GREEN_PIN2 27
#define BLUE_PIN1 28
#define BLUE_PIN2 29
#define DISPLAY_PIN 4

#define VERTICAL_SKIP 90 //Vertical lines to skip during blanking interval

#define RES_WIDTH 640
#define RES_HEIGHT 480

//How many pixels wide the display buffer is
#define PIXELS_WIDTH 120
//How many pixels high the display buffer is
#define PIXELS_HEIGHT 60

//6-bit color (2 bits per R/G/B). This takes up almost all of the dynamic memory on
//the Arduino Mega 2560 with a 120x60 framebuffer. In BGR format: 0bBBGGRR__
byte displayBuffer[PIXELS_WIDTH * PIXELS_HEIGHT]; 

//This stores the image we're going to display.
#define IMAGE_MONA6
#include "image.h"

int blankLinesLeft; //How many lines are left in the blanking interval
byte line; //What line we're drawing in the framebuffer (NOT THE ACTUAL SCREEN LINE)
byte sLine; //What subline we're on
#define SUBLINES_PER_LINE 6

void setup() {
  //Setup pins
  pinMode(VSYNC_PIN, OUTPUT);
  pinMode(HSYNC_PIN, OUTPUT);
  pinMode(RED_PIN1, OUTPUT);
  pinMode(RED_PIN2, OUTPUT);
  pinMode(GREEN_PIN1, OUTPUT);
  pinMode(GREEN_PIN2, OUTPUT);
  pinMode(BLUE_PIN1, OUTPUT);
  pinMode(BLUE_PIN2, OUTPUT);

  //Copy the image (tiled) into the display buffer
  for(int y = 0; y < PIXELS_HEIGHT; y++){
    for(int x = 0; x < PIXELS_WIDTH; x++) {
      displayBuffer[x + y * PIXELS_WIDTH] = pgm_read_byte_near(IMAGE + (x % IMAGE_WIDTH) + ((y % IMAGE_HEIGHT) * IMAGE_WIDTH));
    }
  }

  //Copy the image (tiled) into the display buffer
  for(int y = 0; y < PIXELS_HEIGHT; y++){
    for(int x = 0; x < PIXELS_WIDTH; x++) {
      displayBuffer[x + y * PIXELS_WIDTH] = pgm_read_byte_near(IMAGE + (x % IMAGE_WIDTH) + ((y % IMAGE_HEIGHT) * IMAGE_WIDTH));
    }
  }

  cli(); //Clear interrupts

  //disable timer 0 because it interferes with the others that we need to be precise
  TIMSK0=0;
  TCCR0A=0;
  TCCR0B=(1 << CS00); //enable 16MHz counter
  OCR0A=0;
  OCR0B=0;
  TCNT0=0;

  /**
   * Setup timer for fast PWM for pulsing vsync.
   * This needs to be used because the built-in PWM functions are not precise enough.
   * This is pin 11 on the Arduino Mega 2560.
   * The way this works is that there's a counter which starts at 0 and ends at a predetermined value.
   * The counter goes up by one every (Clock speed (16Mhz) / Prescaler) microseconds.
   * We apply a prescaler of 1024 here, which means that the timer will count up every 64us.
   * Once it reaches the value we want, the pin is switched on. It stays on for another arbitrary amount of counts.
   * The timer also calls an interrupt (defined above) everytime it fires.
   * https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
  **/
  TCCR1A = bit(WGM11) | bit(COM1A1); //9-bit counter, fast PWM, clear A when counter reaches target (This means a 64us duty cycle)
  TCCR1B = bit(WGM12) | bit(WGM13) | bit(CS12) | bit(CS10); //Prescaler of 1024

  // These next two lines set up the counter
  ICR1 = 259; // Generate a pulse after the timer counts to 259, which is approximately every 260*64us, which is 60Hz
  OCR1A = 0;   // Keep the pulse on for 1 count (64us)
  TIFR1 = bit(TOV1); // Clear the overflow flag just in case
  TIMSK1 = bit(TOIE1); // Call interrupt ISR (TIMER1_OVF_vect) when counter reaches target

  /**
   * Same deal, but for horizontal sync.
   * We want this one to go off every 32us, so we use a prescaler of 8 to count up every 0.5us this time.
   * This is pin 9 on the Mega 2560.
   */
  TCCR2A = bit(WGM20) | bit(WGM21) | bit(COM2B1); //Fast PWM, 8-bit, pin 5
  TCCR2B = bit(WGM22) | bit(CS21); //Prescaler of 8
  OCR2A = 63; // Generate pulse when counter gets to 63
  OCR2B = 7; // Keep the pulse on for 8 counts (8us)
  TIFR2 = bit(TOV2); // Clear overflow flag just in case
  TIMSK2 = bit(TOIE2); // Call interrupt ISR (TIMER2_OVF_vect) when counter reaches target

  sei(); //Set interrupts
  set_sleep_mode (SLEEP_MODE_IDLE); //Processor should sleep in between interrupts so timing is right
}

void loop() {
  // Wait for interrupt. Without this, the display shakes because the CPU is still working when the interrupt
  // happens so the delay to switch from the running code to the interrupt code varies by a few microseconds
  // this also means that I can't do anything in loop, so I'll have to find a workaround to the shaking
  sleep_mode();
}

//Vertical sync interrupt. Called every time a new frame starts.
ISR (TIMER1_OVF_vect) {
  //Set blankLinesLeft to VERTICAL_SKIP because we need to not draw anything during veritcal blanking
  blankLinesLeft = VERTICAL_SKIP;
  //Set line to 0 since we're back at the first line
  sLine = -1;
  line = 0;
}

//Horizontal sync interrupt. Called every time a new line starts.
ISR (TIMER2_OVF_vect) {
  //If we're still in the vertical blanking interval, do nothing
  if(blankLinesLeft) {
    blankLinesLeft--;
    return;
  }

  if(line < PIXELS_HEIGHT) {
    /**
     * Here, we write to the PORTA register directly. The last 3 bits on
     * PORTA correspond to pins 27, 28, and 29, so by writing to it we
     * can change the state of those pins all at the same time. This is
     * also done with assembly to avoid C++ overhead, since we need to
     * have this be as fast as possible. The below assembly code is approx.
     * equal to this pseudocode:
     * for(i = 0..PIXELS_WIDTH){
     *   pixels = displayBuffer[line/8][i]
     *   PORTA = pixels
     * }
     */
    asm volatile(
      ".rept 30                   \n\t" // Rept is just a macro that actually copies the below code x amount of times
      "  nop                      \n\t" // Do nothing for 30 cycles to align the picture
      ".endr                      \n\t" // End repeat
      ".rept " STR(PIXELS_WIDTH) "\n\t" // Repeat the below code PIXELS_WIDTH times
      "  ld r16, Z+               \n\t" // Load the byte in the current position in the display buffer to r16 and add one to the display buffer pointer
      "  out %[port], r16         \n\t" // Write r16 to PORTA
      ".endr                      \n\t" // End repeat
      "nop                        \n\t" // Do nothing for one clock cycle to expand the last pixel
      "ldi r16,0                  \n\t" // Load 0 into r16
      "out %[port], r16           \n\t" // Write that to PORTA to turn off the pins
      :
      : [port] "I" (_SFR_IO_ADDR(PORTA)),                // This specifies that %[port] will correspond to PORTA
        "z" "I" (displayBuffer + line * PIXELS_WIDTH)    // This specifies that the display buffer pointer at the current line will be stored in Z
      : "r16", "r20", "memory"                           // This specifies r16 and r20 for use for storing "variables"
    );
    
    if(++sLine == SUBLINES_PER_LINE - 1) {
      sLine = -1;
      line++;
    }
  }
}

// Convenient method to set a pixel at an x and y position in the display buffer.
// Inlined so that the code is actually copied to where it is called instead of calling the function for performance
inline void setPixel(byte x, byte y, byte color) {
  displayBuffer[x + y * PIXELS_WIDTH] = color;
}
