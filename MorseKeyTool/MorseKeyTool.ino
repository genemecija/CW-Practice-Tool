#include <EEPROM.h>

//LCD SETUP
#include <LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// this constant won't change:
const int optionDown = A0;
const int optionUp = A1;
const int optionSelector = 9;
const int ditButton = 2;    // the pin that the pushbutton is attached to
const int dahButton = 7;    // the pin that the pushbutton is attached to

// Variables will change:
//unsigned long startPressTime;
unsigned long endPressTime;
unsigned long gapTime;
int buttonPushCounter = 0;   // counter for the number of button presses
int ditButtonState = 0;         // current state of the button
int ditLastButtonState = 0;     // previous state of the button
int dahButtonState = 0;         // current state of the button
int dahLastButtonState = 0;     // previous state of the button
bool ditPlaying = false;
bool dahPlaying = false;
bool ditPressed = false;
bool dahPressed = false;
bool gapTimerRunning = false;
bool gapTimerMaxReached = false; // once gap reaches 7 * wpm, gaptimer stops
bool interCharacterTimeReached = false; // once gap reaches 3 *8 wpm, denotes start of next letter
char lastPlayed = "";
int wpm = 16;
float ditSpeed = 75; // length of a dit in ms
int pitch = 600;
int mode = 0;

// wpm
// wpm = (60/(wpm*50))*1000

int option = 0;
int optionButtonPrevState = 0;
bool optionsDisplaying = false;
unsigned long optionsDisplayStart;

bool optionUpPrevState = 0;
bool optionDownPrevState = 0;
unsigned long optionHoldStart = 0;
int keyType = 0;
unsigned long straightKeyToneLength = 0;
bool straightKeyPressed = false;
bool paddlePressed = false;
int ditPaddle = 0;
//bool audioOutConnected = false;
int audioOut = 0; // 0 = Speaker, 1 = AUX
const int speakerOut = A3;

String morseBuffer = "";
String letterBuffer = "";


// On startup, LCD should show "Start Keying!"
// When dial is turned, option should show up for one second after the dial stops turning

void setup() {
  lcd.begin(16, 2);

  // initialize the button pin as a input:
  pinMode(ditButton, INPUT);
  pinMode(dahButton, INPUT);
  pinMode(optionSelector, INPUT);
  pinMode(optionDown, INPUT);
  pinMode(optionUp, INPUT);

//  pinMode(A5, INPUT_PULLUP);
//  pinMode(A4, OUTPUT);

  // initialize serial communication:
  Serial.begin(19200);
}


// MORSE BUFFER FUNCTIONS
void addToMorseBuffer(char morse) {
  morseBuffer += String(morse);
}
void clearMorseBuffer() {
  morseBuffer = "";
}

// TONE FUNCTIONS
void playDit() {
  if ((ditButtonState == LOW or (dahButtonState == LOW and ditPaddle == 1)) and ditPlaying == false) {
    ditPlaying = true;
    addToMorseBuffer('.');
    if (audioOut == 1) {
      tone(6, pitch, ditSpeed);
    } else if (audioOut == 0) {
      tone(A3, pitch, ditSpeed);
    }
    
    delay((float)ditSpeed*2);
    ditPlaying = false;
    lastPlayed = '.';
  }
}
void playDah() {
  if ((dahButtonState == LOW or (ditButtonState == LOW and ditPaddle == 1)) and dahPlaying == false) {
    dahPlaying = true;
    addToMorseBuffer('-');
    if (audioOut == 1) {
      tone(6, pitch, ditSpeed*3);
    } else if (audioOut == 0) {
      tone(A3, pitch, ditSpeed*3);
    }
    delay((float)ditSpeed*4);
    dahPlaying = false;
    lastPlayed = '-';
  }
}

// GAP TIMER FUNCTIONS
void startGapTimer() {
  gapTimerRunning = true;
  endPressTime = millis();
}
void stopGapTimer() {
  gapTimerRunning = false;
//  startPressTime = millis();
//  unsigned long t = startPressTime - endPressTime;
//  return t;
}

float ratio = 1;
void checkGapTime(unsigned long t) {
  
  // if ditDah gap time reaches inter-character gap time (3 * ditSpeed)
  unsigned long gapTime = millis() - t;
  if (keyType < 3) {
    ratio = 0.5;
  } else {ratio = 1;}
  if ((gapTime > (ditSpeed * ratio * 3)) and gapTime < ditSpeed * 7) {
    interCharacterTimeReached = true;
    
    if (morseBuffer[0] != 0) {
      convertMorseBufferToLetter();
      clearMorseBuffer();
    }
  }
  
  // if ditDah gap time reaches inter-word gap time (7 * ditSpeed)
  else if (gapTime > (ditSpeed * 7)) {
    if (letterBuffer.length() > 0) {
      addSpaceToLetterBuffer();
    }
    stopGapTimer();
    gapTimerMaxReached = true;
    clearMorseBuffer();
  }
}

// LETTER BUFFER FUNCTIONS
void checkLetterBufferLength() {
  if (letterBuffer.charAt(0) == " ") {
    letterBuffer.remove(0, 1);
  }
  while (letterBuffer.length() > 32) {
    letterBuffer.remove(0, 1);
  }
}
void addSpaceToLetterBuffer() {
  letterBuffer += " ";
  checkLetterBufferLength();
}

// LCD FUNCTIONS
void updateLCD() {
  String line1 = "";
  String line2 = "";
  
  if (optionsDisplaying) {
    lcd.clear();
  }

  if (letterBuffer.length() > 16) {
    line1 = letterBuffer.substring(0, 16); // First line of LCD
    line2 = letterBuffer.substring(16,32); // Second line of LCD
  }
  else {
    line1 = letterBuffer;
  }

  lcd.setCursor(0,0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}
void updateLCD_options(String option, int value) {
//  if (optionsDisplaying) {
    lcd.clear();
//  }
  
  String line1 = option;
  String line2 = String(value);
  if (option == "Key Type") {
    if (value == 0) {
      line2 = "Iambic - Mode A";
    } else if (value == 1) {
      line2 = "Iambic - Mode B";
    } else if (value == 2) {
      line2 = "Single Lever";
    } else if (value == 3) {
      line2 = "Sideswiper";
    } else if (value == 4) {
      line2 = "Straight";
    }
  } else if (option == "Dit Paddle") {
    if (value == 0) {
      line2 = "Left";
    } else if (value == 1) {
      line2 = "Right";
    }
  } else if (option == "Audio Output") {
    if (value == 0) {
      line2 = "Speaker";
    } else if (value == 1) {
      line2 = "Aux Out";
    }
  }

  lcd.setCursor(0,0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}

// MORSE CONVERSION FUNCTION
void convertMorseBufferToLetter() {
  String ditDahs = "";
  String letter = "";

  int i = 0;
  while (morseBuffer[i] != 0) {
    ditDahs += String(morseBuffer[i]);
    i++;
  }
  
  clearMorseBuffer();
  
  if (ditDahs == ".-") { letter = "A"; }
    else if (ditDahs == "-...") { letter = "B"; }
    else if (ditDahs == "-.-.") { letter = "C"; }
    else if (ditDahs == "-..") { letter = "D"; }
    else if (ditDahs == ".") { letter = "E"; }
    else if (ditDahs == "..-.") { letter = "F"; }
    else if (ditDahs == "--.") { letter = "G"; }
    else if (ditDahs == "....") { letter = "H"; }
    else if (ditDahs == "..") { letter = "I"; }
    else if (ditDahs == ".---") { letter = "J"; }
    else if (ditDahs == "-.-") { letter = "K"; }
    else if (ditDahs == ".-..") { letter = "L"; }
    else if (ditDahs == "--") { letter = "M"; }
    else if (ditDahs == "-.") { letter = "N"; }
    else if (ditDahs == "---") { letter = "O"; }
    else if (ditDahs == ".--.") { letter = "P"; }
    else if (ditDahs == "--.-") { letter = "Q"; }
    else if (ditDahs == ".-.") { letter = "R"; }
    else if (ditDahs == "...") { letter = "S"; }
    else if (ditDahs == "-") { letter = "T"; }
    else if (ditDahs == "..-") { letter = "U"; }
    else if (ditDahs == "...-") { letter = "V"; }
    else if (ditDahs == ".--") { letter = "W"; }
    else if (ditDahs == "-..-") { letter = "X"; }
    else if (ditDahs == "-.--") { letter = "Y"; }
    else if (ditDahs == "--..") { letter = "Z"; }
    else if (ditDahs == "-----") { letter = "0"; }
    else if (ditDahs == ".----") { letter = "1"; }
    else if (ditDahs == "..---") { letter = "2"; }
    else if (ditDahs == "...--") { letter = "3"; }
    else if (ditDahs == "....-") { letter = "4"; }
    else if (ditDahs == ".....") { letter = "5"; }
    else if (ditDahs == "-....") { letter = "6"; }
    else if (ditDahs == "--...") { letter = "7"; }
    else if (ditDahs == "---..") { letter = "8"; }
    else if (ditDahs == "----.") { letter = "9"; }
    else if (ditDahs == "--..--") { letter = ","; }
    else if (ditDahs == "..--..") { letter = "?"; }
    else if (ditDahs == "-.-.--") { letter = "!"; }
    else if (ditDahs == "-....-") { letter = "-"; }
    else if (ditDahs == "-..-.") { letter = "/"; }
    else if (ditDahs == ".--.-.") { letter = "@"; }
    else if (ditDahs == "-.--.") { letter = "("; }
    else if (ditDahs == "-.--.-") { letter = ")"; }
    else if (ditDahs == ".----.") { letter = "'"; }
    else if (ditDahs == ".-..-.") { letter = "\""; }
    else if (ditDahs == ".-...") { letter = "&"; }
    else if (ditDahs == "---...") { letter = ") {"; }
    else if (ditDahs == "-.-.-.") { letter = ";"; }
    else if (ditDahs == "-...-") { letter = "="; }
    else if (ditDahs == ".-.-.") { letter = "+"; }
    else if (ditDahs == "..--.-") { letter = "_"; }
    else if (ditDahs == "...-..-") { letter = "$"; }
    else if (ditDahs == ".-.-.-") { letter = "."; }
    else if (ditDahs == "........") { letter = "<HH>"; }
    else if (ditDahs == ".-.-") { letter = "<AA>"; }
    else if (ditDahs == ".-...") { letter = "<AS>"; }
    else if (ditDahs == "-...-.-") { letter = "<BK>"; }
    else if (ditDahs == "-.-..-..") { letter = "<CL>"; }
    else if (ditDahs == "-.-.-") { letter = "<CT>"; }
    else if (ditDahs == "-..---") { letter = "<DO>"; }
    else if (ditDahs == "-.-.-") { letter = "<KA>"; }
    else if (ditDahs == "-.--.") { letter = "<KN>"; }
    else if (ditDahs == "...-.-") { letter = "<SK>"; }
    else if (ditDahs == "...-.") { letter = "<SN>"; }
    else if (ditDahs == "...---...") { letter = "<SOS>"; }
    else {
      letter = "<?>";
      }
  
  letterBuffer += letter;
  checkLetterBufferLength();

  updateLCD();
}


void updateOption(int value) {
  if (option == 0) {
    wpm = wpm + value;
    if (wpm > 30) { wpm = 30; }
    if (wpm < 5) { wpm = 5; }
    ditSpeed = (float)1200/wpm;
    optionsDisplaying = true;
    updateLCD_options("WPM", wpm);
    optionsDisplayStart = millis();
  } 
  else if (option == 1) {
    pitch = pitch + (value*10);
    if (pitch > 1000) { pitch = 1000; }
    if (pitch < 400) { pitch = 400; }
    optionsDisplaying = true;
    updateLCD_options("Pitch", pitch);
    optionsDisplayStart = millis();
  }
  else if (option == 2) {
    keyType = keyType + value;
    if (keyType > 4) {
      keyType = 0;
    } else if (keyType < 0) {
      keyType = 4;
    }
    if (keyType > 1 and option == 3) {
      option += 1;
    }
    
    // Iambic Mode A, Iambic Mode B, Single Lever, Straight
    updateLCD_options("Key Type", keyType);
    optionsDisplaying = true;
    optionsDisplayStart = millis();
  }
  else if (option == 3) {
    ditPaddle = ditPaddle + value;
    if (ditPaddle > 1) {
      ditPaddle = 0;
    } else if (ditPaddle < 0) {
      ditPaddle = 1;
    }
    updateLCD_options("Dit Paddle", ditPaddle);
    optionsDisplaying = true;
    optionsDisplayStart = millis();
  }
//  else if (option == 4) {
//    audioOut = audioOut + 1;
//    if (audioOut > 1) {
//      audioOut = 0;
//    } else if (audioOut < 0) {
//      audioOut = 1;
//    }
//    updateLCD_options("Audio Output", audioOut);
//    optionsDisplaying = true;
//    optionsDisplayStart = millis();
//  }
}

void loop() {

//  if (digitalRead(A5) == HIGH) {
////    Serial.println("AUX CONNECTED");
//    audioOutConnected = true;
//    digitalWrite(A4, LOW);
//  } else if (digitalRead(A5) == LOW) {
////    Serial.println("AUX DISCONNECTED");
//    audioOutConnected = false;
//    digitalWrite(A4, HIGH);
//  }
  
//  if (digitalRead(A4) == HIGH) {
//    Serial.println("disconnected!");
//  }

  // Clear OPTIONS from LCD screen after 5 seconds
  if (optionsDisplaying == true and (millis() - optionsDisplayStart) > 5000) {
    updateLCD();
    optionsDisplaying = false;
  }

  // Handle OPTION UP button press/depress
  if (digitalRead(optionUp) == 1 and optionUpPrevState == 0) {
    optionUpPrevState = 1;
    optionHoldStart = millis();
    updateOption(1);
  }
  else if (digitalRead(optionUp) == 1 and optionUpPrevState == 1 and ((millis() - optionHoldStart)>2000)) {
    updateOption(1);
  }
  else if (digitalRead(optionUp) == 0) {
    optionUpPrevState = 0;
  }
  // Handle OPTION DOWN button press/depress
  if (digitalRead(optionDown) == 1 and optionDownPrevState != 1) {
    optionDownPrevState = 1;
    optionHoldStart = millis();
    updateOption(-1);
  }
  else if (digitalRead(optionDown) == 1 and optionDownPrevState == 1 and ((millis() - optionHoldStart)>2000)) {
    updateOption(-1);
  }
  else if (digitalRead(optionDown) == 0) {
    optionDownPrevState = 0;
  }


  // Handle OPTIONS button press/depress
  if (digitalRead(optionSelector) == 0 and optionButtonPrevState != 0) {
    optionButtonPrevState = 0;
    option += 1;
    if (option > 3) { option = 0; }
    // Skip option 3 if not in Iambic (double-paddle) mode
    if (option == 3 and (keyType > 1)) {
      option += 1;
    }
    if (option == 0) {
      optionsDisplaying = true;
      updateLCD_options("WPM", wpm);
      optionsDisplayStart = millis();
    }
    else if (option == 1) {
      optionsDisplaying = true;
      updateLCD_options("Pitch", pitch);
      optionsDisplayStart = millis();
    }
    else if (option == 2) {
      optionsDisplaying = true;
      updateLCD_options("Key Type", keyType);
      optionsDisplayStart = millis();
    }
    else if (option == 3) {
      optionsDisplaying = true;
      updateLCD_options("Dit Paddle", ditPaddle);
      optionsDisplayStart = millis();
    }
//    else if (option == 4) {
//      optionsDisplaying = true;
//      updateLCD_options("Audio Output", 0);
//      optionsDisplayStart = millis();
//    }
//    else if (option == 5) {
//      optionsDisplaying = true;
//      updateLCD_options("Save Config", 1);
//      optionsDisplayStart = millis();
//    }
  }
  else if (digitalRead(optionSelector) == 1 and optionButtonPrevState == 0) {
    optionButtonPrevState = 1;
  }

  // Handle CW Key press/depress
  if (keyType < 3) {
    ditButtonState = digitalRead(ditButton);
    dahButtonState = digitalRead(dahButton);
    
    // If either paddle is pressed...
    if (ditButtonState == LOW or dahButtonState == LOW) {
      gapTimerMaxReached = false;
//      interCharacterTimeReached = false;
      checkGapTime(millis());
      if (gapTimerRunning == true) {
        stopGapTimer();
      }
      paddlePressed = true;
    }
  
    // Handle Iambic Mode A or B paddle presses
    // BOTH paddles pressed
    if (ditButtonState == LOW and dahButtonState == LOW and (keyType == 0 or keyType == 1)) {
      if (lastPlayed == '.') {
        playDah();
        if (keyType == 1) { // If in Iambic Mode B
          playDit();
        }
      }
      else if (lastPlayed == '-') {
        playDit();
        if (keyType == 1) {
          playDah();
        }
      }
      else {
        playDit();
        playDah();
        if (keyType == 1) { // If in Iambic Mode B
          playDit();
        }
      }
    }
    // Iambic Mode A, Iambic Mode B, or Single Lever
    else if (dahButtonState == LOW and ditButtonState == HIGH and keyType < 3) {
      Serial.print("dah");
      // Paddles in normal orientation
      if (ditPaddle == 0) {
        playDah();
      }
      // Paddles in switched orientation
      else if (ditPaddle == 1) {
        playDit();
      }
    }
    // Iambic Mode A, Iambic Mode B, or Single Lever
    else if (ditButtonState == LOW and dahButtonState == HIGH and keyType < 3) {
      Serial.print("dit");
      if (ditPaddle == 0) {
        playDit();
      }
      // Paddles in switched orientation
      else if (ditPaddle == 1) {
        playDah();
      }
    }

    // If no paddle is pressed and gapTimer is not running and hasn't maxed out...
    else if (ditButtonState == HIGH and dahButtonState == HIGH and gapTimerRunning == false and gapTimerMaxReached == false) {
      if (paddlePressed) {
        startGapTimer();
        paddlePressed = false;
      }
      
    }
  }

  // Sideswiper Key
  if (keyType == 3) {
    ditButtonState = digitalRead(ditButton);
    dahButtonState = digitalRead(dahButton);

    if (ditButtonState == LOW or dahButtonState == LOW) {
      tone(speakerOut, pitch);
      
      if (!straightKeyPressed) {
        straightKeyToneLength = millis();
        gapTimerMaxReached = false;
        interCharacterTimeReached = false;
        if (gapTimerRunning == true) {
          stopGapTimer();
        }
      }
      
      straightKeyPressed = true;
  //    Serial.
    }
    else if (ditButtonState == HIGH and dahButtonState == HIGH) {
      noTone(speakerOut);

      
      if (straightKeyPressed) {
        unsigned long toneLength = millis() - straightKeyToneLength;

        if (toneLength < ditSpeed*1.5 and toneLength > 5) {
          addToMorseBuffer('.');
        } else if (toneLength > 5) {
          addToMorseBuffer('-');
        }
        startGapTimer();
        straightKeyPressed = false;
      }
    }
  }
  
  // Straight Key
  if (keyType == 4) {
    ditButtonState = digitalRead(ditButton);

    if (ditButtonState == LOW) {
      tone(speakerOut, pitch);
      
      if (!straightKeyPressed) {
        straightKeyToneLength = millis();
        gapTimerMaxReached = false;
        interCharacterTimeReached = false;
        if (gapTimerRunning == true) {
          stopGapTimer();
        }
      }
      
      straightKeyPressed = true;
  //    Serial.
    }
    else if (ditButtonState == HIGH) {
      noTone(speakerOut);

      
      if (straightKeyPressed) {
        unsigned long toneLength = millis() - straightKeyToneLength;

        if (toneLength < ditSpeed*1.5 and toneLength > 5) {
          addToMorseBuffer('.');
        } else if (toneLength > 5) {
          addToMorseBuffer('-');
        }
        startGapTimer();
        straightKeyPressed = false;
      }
    }
  }
  

  if (gapTimerRunning == true) {
    checkGapTime(endPressTime);
  }

}
