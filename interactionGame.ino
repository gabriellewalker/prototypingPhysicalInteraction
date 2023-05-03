#include <Wire.h>
#include <ADXL345.h>
#include "rgb_lcd.h"
#include <CMBMenu.hpp>
#include "MMA7660.h"
#include <ChainableLED.h>
#include <EEPROM.h>

#define NOTE_E 165

MMA7660 accelemeterL;
ADXL345 adxlR;  //variable adxl is an instance of the ADXL345 library

//Joystick values
#define right 0
#define left 1
#define enter 2
#define none 3

#define NUM_LEDS 4

//speaker pin
#define SPEAKER 3

//speaker tones
int BassTab[] = { 300, 1911 };

rgb_lcd lcd;
ADXL345 adxl;       //variable adxl is an instance of the ADXL345 library
bool playingGame;   //has game been started
bool tutorialMode;  // is the game for the user's practice only?

ChainableLED leds(7, 8, NUM_LEDS);


char *directions[] = { "LeftOut", "LeftUp", "RightOut", "RightUp" };
long direction;

int currentGameScore;
int highScore;
int eeAddress = 0;  //lcation for storing high score

int gameLength = 5;

char difficulty = 'd';

// Menus
String main_menu[] = { "<  Easy Level  >", "< Medium Level >", "<  Hard Level  >", "<   Tutorial   >", "<  Scoreboard  >" };

// Current menu and item to be displayed
int current_menu_item;
bool viewHighScore;
String *current_menu;
// Used to check joystick state
int last_joy_read;

int lightUpTime;         // William

// UNUSED - these variables are likely to be removed
// int startTime = 0;       // William
// int timeElapsed = 0;     // William
// int intervalMillis = 0;  // William
// int intervalStart = 0;  // William

// int8_t x;
// int8_t y;
// int8_t z;

int read_joystick() {
  int output = none;
  // read all joystick x value
  int x = analogRead(A0);
  int y = analogRead(A1);
  // Serial.print("X: ");
  // Serial.print(x);
  // Serial.print("Y: ");
  // Serial.println(y);
  // if (sensorValue1 == 1023 || sensorValue1 >= 650){
  //   output = enter;
  // } else if (sensorValue1 >= 750 || sensorValue1 >= 450) {
  //   output = right;
  // } else if (sensorValue1 <= 270 || sensorValue1 >= 500) {
  //   output = left;
  // }

  if (x == 1023) {
    output = enter;
  } else if (y >= 740) {
    output = right;
  } else if (y <= 275) {
    output = left;
  }

  return output;
}

void print_line(int line, String text) {
  lcd.setCursor(0, line);
  lcd.setCursor(0, line);
  lcd.print(text);
}

void move_right() {
  if (current_menu_item == 4) {
    current_menu_item = 0;
    // loop back to first item
  } else {
    current_menu_item += 1;
  }
}
void move_left() {
  if (current_menu_item == 0) {
    current_menu_item = 4;
    // loop back to last item
  } else {
    current_menu_item -= 1;
  }
}

String read_accel_L() {
  String direction = "";

  int8_t x;
  int8_t y;
  int8_t z;
  accelemeterL.getXYZ(&x, &y, &z);
  Serial.print("LEFT: ");
  Serial.print("x = ");
  Serial.print(x);
  Serial.print("y = ");
  Serial.print(y);
  Serial.print("z = ");
  Serial.println(z);
  if (x > -10) {
    direction = "LeftOut";
  }
  if (y < -15) {
    direction = "LeftUp";
  }
  if (x > -25 && x < -18) {
    direction = "LeftCenter";
  }

  Serial.println(direction);

  return direction;
}

String read_accel_R() {
  String direction = "";

  int x, y, z;
  adxlR.readXYZ(&x, &y, &z);
  Serial.print("RIGHT: ");
  Serial.print("x = ");
  Serial.print(x);
  Serial.print("y = ");
  Serial.print(y);
  Serial.print("z = ");
  Serial.println(z);

  if (x < 100) {
    direction = "RightOut";
  }
  if (y < -150) {
    direction = "RightUp";
  }
  if (x < 250 && x > 230) {
    direction = "RightCenter";
  }
  Serial.println(direction);

  return direction;
}

void pinInit() {
  pinMode(SPEAKER, OUTPUT);
  digitalWrite(SPEAKER, LOW);
}

void sound(uint8_t note_index) {
  for (int i = 0; i < 100; i++) {
    digitalWrite(SPEAKER, HIGH);
    delayMicroseconds(BassTab[note_index]);
    digitalWrite(SPEAKER, LOW);
    delayMicroseconds(BassTab[note_index]);
  }
}

void setup() {

  lcd.begin(16, 2);
  accelemeterL.init();
  adxlR.powerOn();
  Serial.begin(9600);
  randomSeed(666);
  pinInit();

  leds.init();

  // Print menu template on lcd.
  lcd.setCursor(0, 0);
  lcd.print("Menu");
  lcd.setCursor(0, 1);
  lcd.print(main_menu[current_menu_item]);

  EEPROM.get(eeAddress, highScore);
  Serial.println(highScore, 3);

  // Init vars
  playingGame = false;
  tutorialMode = false;
  viewHighScore = false;
  current_menu_item = 0;
  last_joy_read = none;
  current_menu = main_menu;

}

void loop() {

  int current_joy_read = read_joystick();  // Continuously read the joystick position
  // Serial.println(current_joy_read);
  if (current_joy_read != last_joy_read) {  // If joystick position has changed, respond accordingly
    last_joy_read = current_joy_read;

    switch (current_joy_read) {
      case right:
        move_right();
        break;
      case left:
        move_left();
        break;
      case enter:  // If joystick is pressed down
        Serial.print("View high score?");
        Serial.println(viewHighScore);
        if (current_menu_item == 0) {  // Start easy-level game
          difficulty = 'e';
          playingGame = true;
          break;
        }
        if (current_menu_item == 1) {  // Start medium-level game
          difficulty = 'm';
          playingGame = true;
          break;
        }
        if (current_menu_item == 2) {  // Start hard-level game
          difficulty = 'h';
          playingGame = true;
          break;
        }
        if (current_menu_item == 3) {  // Start tutorial game (for practising)
          difficulty = 't';
          playingGame = true;
          break;
        }
        if (current_menu_item == 4) {  // If program is on "Scoreboard" option
          if (viewHighScore) {         // If already displaying high score, return to menu
            viewHighScore = false;
          } else {  // Else, display high score now
            viewHighScore = true;
          }
          break;
        }
      default: break;
    }
  }
  delay(100);
  lcd.clear();
  lcd.setCursor(0, 0);
  // if game has not been started - show the menu
  if (!playingGame && !viewHighScore) {
    lcd.print("Menu");
    print_line(1, current_menu[current_menu_item]);
    if (highScore < 10) {  // Aligns score to the right depending on number of digits
      lcd.setCursor(12, 0);
    } else if (highScore < 100) {
      lcd.setCursor(11, 0);
    } else if (highScore < 1000) {
      lcd.setCursor(10, 0);
    } else {
      lcd.setCursor(9, 0);
    }
    lcd.print("HS:");
    lcd.print(highScore);
    currentGameScore = 0;
  }
  //if view high score has been selected
  else if (viewHighScore) {
    lcd.print("High Score:");
    lcd.setCursor(0, 1);
    lcd.print(highScore);
  } else {  //if game has started
    // make sure hand starts in center
    // for(int x = 0; x < NUM_LEDS; x++){
    //   leds.setColorRGB(x, 225, 225, 225);
    //   delay(200);
    // }

    leds.setColorRGB(0, 0, 0, 0);
    leds.setColorRGB(1, 0, 0, 0);
    leds.setColorRGB(2, 0, 0, 0);
    leds.setColorRGB(3, 0, 0, 0);

    if (tutorialMode) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tutorial scores");
      lcd.setCursor(0, 1);
      lcd.print("won't be saved");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wear wristbands,");
      lcd.setCursor(0, 1);
      lcd.print("cables face-down");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Flick outward/up");
      lcd.setCursor(0, 1);
      lcd.print("if light flashes");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Faster reactions");
      lcd.setCursor(0, 1);
      lcd.print("= Higher scores");
      delay(2000);
    }

    lcd.setCursor(0, 0);
    lcd.print("Face hands ahead");
    lcd.setCursor(0, 1);
    lcd.print("palms face down");
    String startingPositionL = read_accel_L();
    String startingPositionR = read_accel_R();

    int gameCount = 0;

    Serial.println(startingPositionL);
    Serial.println(startingPositionR);
    delay(3000);
    //once centered
    while (startingPositionL == "LeftCenter" && startingPositionR == "RightCenter" && gameCount < gameLength) {

      lcd.clear();
      lcd.setCursor(0, 0);
      //choose random direction
      direction = random(sizeof(directions) / sizeof(char *));
      lcd.write(directions[direction]);
      Serial.print(directions[direction]);
      if (directions[direction] == "LeftOut") {
        leds.setColorRGB(0, 225, 0, 0);
        lightUpTime = millis();
        if (tutorialMode) {
          lcd.setCursor(0, 0);
          lcd.print("<- Flick left");
          lcd.setCursor(0, 1);
          lcd.print("  wrist OUT");
        }
      } else if (directions[direction] == "LeftUp") {
        leds.setColorRGB(1, 0, 0, 255);
        lightUpTime = millis();
        if (tutorialMode) {
          lcd.setCursor(0, 0);
          lcd.print("^ Flick left");
          lcd.setCursor(0, 1);
          lcd.print("  hand UP");
        }
      } else if (directions[direction] == "RightUp") {
        leds.setColorRGB(2, 0, 0, 255);
        lightUpTime = millis();
        if (tutorialMode) {
          lcd.setCursor(0, 0);
          lcd.print("Flick right ^");
          lcd.setCursor(0, 1);
          lcd.print("hand UP");
        }
      } else if (directions[direction] == "RightOut") {
        leds.setColorRGB(3, 225, 0, 0);
        lightUpTime = millis();
        if (tutorialMode) {
          lcd.setCursor(0, 0);
          lcd.print("Flick right ->");
          lcd.setCursor(0, 1);
          lcd.print("wrist OUT");
        }
      }

      //print current score on the bottom right corner
      setScoreCursor();
      lcd.print(currentGameScore);
      delay(getInterval(difficulty));  // WIP DIFFICULTY LEVELS
                           //   //read hand direction
      String newDirectionL = read_accel_L();
      String newDirectionR = read_accel_R();
      if (newDirectionL == directions[direction]) {
        sound(0);
        lcd.setCursor(0, 0);
        lcd.print("CORRECT!");
        Serial.println("CORRECT!");
        int pointsToAdd = getScore();
        lcd.setCursor(0, 1); // this is so tedious because i can't figure out how to concatenate in c++ lmao
        lcd.print("+");
        lcd.setCursor(1, 1);
        lcd.print(pointsToAdd);
        lcd.setCursor(6, 1);
        lcd.print("pts");

        // Serial.println("+" + to_string(pointsToAdd) + " pts");

        currentGameScore += pointsToAdd;

      } else if (newDirectionR == directions[direction]) {
        sound(0);
        lcd.setCursor(0, 0);
        lcd.print("CORRECT!");
        Serial.println("CORRECT!");
        int pointsToAdd = getScore();
        lcd.setCursor(0, 1);
        lcd.print("+");
        lcd.setCursor(1, 1);
        lcd.print(pointsToAdd);
        lcd.setCursor(6, 1);
        lcd.print("pts");

        // Serial.println("+" + to_string(pointsToAdd) + " pts");

        currentGameScore += pointsToAdd;

      } else {
        sound(1);
        lcd.setCursor(0, 0);
        lcd.print("WRONG!");
        Serial.println("WRONG!");
      }
      gameCount++;

      leds.setColorRGB(0, 0, 0, 0);
      leds.setColorRGB(1, 0, 0, 0);
      leds.setColorRGB(2, 0, 0, 0);
      leds.setColorRGB(3, 0, 0, 0);
      delay(1000);
    }

    if (gameCount == gameLength) {

      if (tutorialMode) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Tutorial Done");
        lcd.setCursor(0, 1);
        lcd.print("Try it yourself!");
      } else {

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.write("Game Over");
        if (currentGameScore > highScore) {
          highScore = currentGameScore;
          EEPROM.put(eeAddress, highScore);
          lcd.setCursor(0, 1);
          lcd.write("New High Score!");
          setScoreCursor();
          lcd.print(currentGameScore);
        } else {
          lcd.setCursor(0, 1);
          lcd.write("Try harder");
          setScoreCursor();
          lcd.print(currentGameScore);
        }
      }
      delay(3000);
      playingGame = false;
    }
  }
}

void setScoreCursor() {
  if (currentGameScore < 10) {
    lcd.setCursor(15, 1);
  } else if (currentGameScore < 100) {
    lcd.setCursor(14, 1);
  } else if (currentGameScore < 1000) {
    lcd.setCursor(13, 1);
  } else {
    lcd.setCursor(12, 1);
  }
}

int getInterval(char difficulty) {
  switch (difficulty) {
    case 'e': return 1500;
    case 'm': return 1000;
    case 'h': return 500;
    case 't': return 5000;
    default: return 1500;
  }
}

int getScore() {
  int reactionTime = millis() - lightUpTime;
  if (reactionTime <= 250) {
    return 100;
  } else if (reactionTime <= 500) {
    return 85;
  } else if (reactionTime <= 750) {
    return 70;
  } else if (reactionTime <= 1000) {
    return 55;
  } else if (reactionTime <= 1250) {
    return 40;
  } else if (reactionTime <= 1500) {
    return 25;
  } else {
    return 10;
  }
}
