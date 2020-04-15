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

#define VSYNC_PIN 11
#define HSYNC_PIN 9
#define RED_PIN 7
#define GREEN_PIN 6
#define BLUE_PIN 5

#define VERTICAL_SKIP 32 //Vertical lines to skip during blanking interval

#define RES_HEIGHT 480

byte blankLinesLeft;
unsigned short line;

//Vertical sync interrupt. Called every time a new frame starts.
ISR (TIMER1_OVF_vect) {
  //Set blankLinesLeft to VERTICAL_SKIP because we need to not draw anything during veritcal blanking
  blankLinesLeft = VERTICAL_SKIP;
  //Set line to 0 since we're back at the first line
  line = 0;
}

//Horizontal sync interrupt. Called every time a new line starts.
ISR (TIMER2_OVF_vect) {
  //If we're still in the vertical blanking interval, do nothing
  if(blankLinesLeft) {
    blankLinesLeft--;
    return;
  }

  //If we're past the end of the screen, do nothing
  if(line >= RES_HEIGHT) {
    return;
  }

  //Red, green, or blue, depending on what line we're on.
  //Currently not precise at all, but it gets the job done.

  if(line < 160) {
    digitalWrite(RED_PIN, HIGH);
    delay(1);
    digitalWrite(RED_PIN, LOW);
  } else if(line < 320) {
    digitalWrite(GREEN_PIN, HIGH);
    delay(1);
    digitalWrite(GREEN_PIN, LOW);
  } else {
    digitalWrite(BLUE_PIN, HIGH);
    delay(1);
    digitalWrite(BLUE_PIN, LOW);
  }
  
  line++;
}

void setup() {
  pinMode(VSYNC_PIN, OUTPUT);
  pinMode(HSYNC_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

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
  TCCR2B=bit(WGM22) | bit(CS21); //Prescaler of 8
  OCR2A=63; // Generate pulse when counter gets to 63
  OCR2B=7; // Keep the pulse on for 8 counts (8us)
  TIFR2=bit(TOV2); // Clear overflow flag just in case
  TIMSK2=bit(TOIE2); // Call interrupt ISR (TIMER2_OVF_vect) when counter reaches target

  sei(); //Set interrupts
}

void loop() {
  // put your main code here, to run repeatedly:
}
