/*
  ===========================================================
  Maker Uno POV Display, Sync with Hall Sensor
  Version 0.4.3
  By: 1487Quantum (https://github.com/1487quantum)
  ===========================================================
  > Uses moving average, with a period of 8
  > Reorganised code
  > Able to display alphanumeric chracters & some symbols (0-9, A-Z)

  Ref: https://blog.blinkenlight.net/experiments/basic-effects/pov-reloaded/

  ===========================================================
  CHANGELOG
  ===========================================================
  v0.4.3
  - Removed moving average
  - Fixed LED update duration to 0.0001ms
  v0.4.2
  - Shifted var to config.h
  - 'f7x5.h' replaces 'f7x7.h' as font table header
  - Updated parsing fx to dynamically add the "blank" columns
    instead of hardcoding it int0 the font table.
  - Removed update timer fx: Keeping refresh rate constant at 0.001
  v0.4.1
  - Added few more test constants
  - Updated to parse some symbols: : ! " # $ % ' + , - . /

  v0.4.0
  - Updated f7x7.h, includes uppercase character & new number style
  - Updated message parsing fx
  - Added alphabet parsing functions (Characters must be CAPITALISED!)
  - Added constants to display text numbers & characters
  - Added display test function for the font table

  v0.3.3
  - Set limit of number of characters that can be displayed at once: 35

  v0.3.2
  - Expanded font file to 7x7, includes uppercase character
  - 'f7x7.h' replaces 'num6x5.h' as font table header
  - Added integer parsing function by getting ASCII
    (Able to display numbers ONLY (for now) using Strings Var)
  - Changed MA Period: 10 -> 5

  v0.3.1
  - Added moving average (period: 10) for LED active (ON) duration
  - Uses interrupt to refresh POV display (active high) instead of setting a fixed refresh rate
    (MsTimer2 delay in setup)
  - Reorganised code

  v0.3.0
  - Added Hall Sensor to POV Display Setup for display synchronisation

  v0.2.1
  - Added "num6x5.h" as font reference, consists of only numbers (0-9)

  v0.2.0
  - Uses MsTimer2 library instead of 'default' delay function to refresh
  - Update function to utilize bitwise operation to display the chracters

  v0.1.1
  - Use PROGMEM to store the display characters 0-4
  - Added functions to parse the display characters

  v0.1.0
  - Initial Release
  - Able to display the following characters: 0 - 4
*/

#include "f7x5.h"
#include "MsTimer2.h"
#include "config.h"

//Ouput display colmn
//If upper set true, (upper bit) value would be returned
uint8_t fmt_pattern(bool upper, uint16_t p) {
  uint8_t q;
  if (upper) {
    //Shift by LEDOFFSET to left as LED starts from D2, not D0.
    //After that, Shift right by 8 to remove lower bits.
    q = (p << LEDOFFSET) >> 8;
  } else {
    //Lower half
    q = (p << LEDOFFSET) & 0xff; //Shift by LEDOFFSET to left as LED starts from D2, not D0
  }
  return q;
}

//Display string on POV
void dispMsg() {
  //printMsg(T_AP);
  char charBuf[MAXCHAR];
  String buf = "RPM - ";
  buf += pd ;
  buf.toCharArray(charBuf, MAXCHAR);
  printMsg(charBuf);
}


//Push the columns
//Font table, font element, column index
void pushCol(uint16_t *fnt[], uint16_t ch, uint16_t i) {
  if (DEBUG) {
    Serial.print(ch);
    Serial.print(F(","));
    Serial.print(i);
    Serial.print(F(","));
    Serial.println(pgm_read_word(fnt[ch] + i), HEX);
  }
  if (ch == -1) {
    //Turn off all LEDs
    PORTD = fmt_pattern(false, 0);
    PORTB = fmt_pattern(true, 0);
  } else {
    PORTD = fmt_pattern(false, pgm_read_word(fnt[ch] + i));
    PORTB = fmt_pattern(true, pgm_read_word(fnt[ch] + i));
  }
}


//Parse str -> char
void printMsg(char cmsg[]) {
  int char_k = 0;
  if (ch == strlen(cmsg) || ch >= MAXCHAR ) {
    //If msg end reach or char limit reach -> Blank the rest
    ch = -1;
    i = 0;
    done = 1;
  } else {
    char_k = cmsg[ch]; //Individual char ASCII
  }
  if (DEBUG) {
    Serial.print("Char: ");
    Serial.print(ch == -1 ? '~' : cmsg[ch] );
    Serial.print(",Blank: ");
    Serial.print(blank);
    Serial.print(",Done: ");
    Serial.println(done);
  }
  if (!done) {
    // make sure the character is within the alphabet bounds (defined by the font.h file)
    // if it's not, make it a blank character ('z'->122)
    if (char_k < 32 || char_k > 122) {
      char_k = 32;
    }
    //Convert lower to upper case if necessary
    // ASCII for a-z: 97-122
    if (char_k > 96 && char_k <= 122) {
      char_k -= 32;
    }
    // Converts ASCII num to the font index number
    if (char_k == 32 || blank) {
      pushCol(f_sym, -1, i); //Display nothing
    } else {
      //Check for symbols
      if (char_k > 32 && char_k <= 47) {
        char_k -= 33;
        pushCol(f_sym, char_k, i);
      } else {
        char_k -= 48;   //Since ascii for '1' is 49
        if (char_k > -1 && char_k < 10) {
          //Numbers 0 - 9
          pushCol(f_num, char_k, i);
        } else if (char_k > 15 && char_k <= 42) {
          //Alphabet A-Z, subtract 17 for alphabet font table
          pushCol(f_alp - 17, char_k, i);
        } else {
          //Display nothing
          pushCol(f_sym, ch, i);
        }
      }
    }

    if (i >= CH_WIDTH + CH_SPACE) {
      //All columns displayed, move to next char
      i = 0;
      blank = 0;
      ch++;
    } else {
      if (i >= CH_WIDTH - 1) {
        //Space between the char
        blank = 1;
      }
      //Mov to nxt column of char
      i++;
    }
  } else {
    //Display nothing
    pushCol(f_sym, ch, i);
  }
}


void setup() {
  if (DEBUG) {
    Serial.begin(115200);
  }
  // Leave pin 0 (serial receive) as input, otherwise serial port will stop working!) ...
  DDRD = 0b11111010; // set digital  1,3- 7 to output, 2 as input (interrupt)
  DDRB = 0b00111111; // set digital  8-13 to output
  //Attach interrupt for hall sensor
  attachInterrupt(digitalPinToInterrupt(INT_PIN), updatePd, CHANGE);

  //Start display update timer
  MsTimer2::set(UPDATE_LED, dispMsg);
  MsTimer2::start();
}

void updatePd() {
  state = !state;
  //Rising edge would be time marker/trigger
  if (state) {
    unsigned long tmp_t = millis();             //Tmp var

    //long r_duration = tmp - lastUpdate; //raw duration

    //Duration of 1 rotation/revolution (in ms)
    //float s = int((tmp_t - lastUpdate) % 1000);
    pd = tmp_t - lastUpdate;
    pd %= 1000;
    lastUpdate = tmp_t;

    // Rotation in 1ms (ms/rev)
    //pd = 1/pd;
    if (DEBUG) {
      Serial.print("Pd:");
      Serial.println(pd);
    }

    //Refresh display
    dispMsg();
    ch = 0; i = 0; done = 0;
  }
}

void loop() {
}
