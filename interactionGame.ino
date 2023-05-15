#include <Wire.h>
#include <ADXL345.h>
#include "rgb_lcd.h"
#include "MMA7660.h"
#include <ChainableLED.h>
#include <EEPROM.h>

MMA7660 accelemeterL;
ADXL345 adxlR;
rgb_lcd lcd;

// Joystick values
#define right 0
#define left 1
#define enter 2
#define none 3

//LEDs
#define NUM_LEDS 4
ChainableLED leds(7, 8, NUM_LEDS);

// speaker pin
#define SPEAKER 3

bool joystickPressed;

// speaker tones
int BassTab[] = { 300, 1911, 1000, 500 };


//Directions
char *directions[] = { "LeftOut", "LeftUp", "RightOut", "RightUp" };
int direction;

int highScores[3];
int eeAddressEasy;  // location for storing high score easy
int eeAddressMedium;
int eeAddressHard;

int currentGameScore;

int gameLength = 10;
int gameCount;
int difficulty;
bool tutorialMode;
bool playingGame;  // has game in progress
int gameState;     //0 = not started/ended , 1 = playing, 2 = paused

byte level;
// Menus
String main_menu[] = { "<   Tutorial   >", "<  Easy Level  >", "< Medium Level >", "<  Hard Level  >", "<  Scoreboard  >" };
String pause_menu[] = { "<    Resume    >", "<   Restart    >", "<     Exit     >" };

// Current menu and item to be displayed
int current_menu_item;
int current_pause_menu_item;
bool viewHighScore;
String *current_menu;

// Used to check joystick state
int last_joy_read;
int lightUpTime;


int read_joystick() {
  int output = none;
  // read all joystick x value
  int x = analogRead(A0);
  int y = analogRead(A1);
  if (x > 900) {
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
  lcd.print(text);
}

void move_right(String menu) {
  if (menu == "main") {
    current_menu_item == 4 ? current_menu_item = 0 : current_menu_item += 1;
  } else if (menu == "pause") {
    current_pause_menu_item == 2 ? current_pause_menu_item = 0 : current_pause_menu_item += 1;
  }
}

void move_left(String menu) {
  if (menu == "main") {
    current_menu_item == 0 ? current_menu_item = 4 : current_menu_item -= 1;
  } else if (menu == "pause") {
    current_pause_menu_item == 0 ? current_pause_menu_item = 2 : current_pause_menu_item -= 1;
  }
}

String read_accel_L() {
  String direction = "";

  int8_t x;
  int8_t y;
  int8_t z;
  accelemeterL.getXYZ(&x, &y, &z);
  Serial.print("X:");
  Serial.println(x);
  Serial.print("Y:");
  Serial.println(y);
  
  if (x > -10) {
    direction = "LeftOut";
  }
  if (y < -15) {
    direction = "LeftUp";
  }
  if (x > -25 && x < -18) {
    direction = "LeftCenter";
  }
  return direction;
}

String read_accel_R() {
  String direction = "";

  int x, y, z;
  adxlR.readXYZ(&x, &y, &z);
    Serial.print("X:");
  Serial.println(x);
  Serial.print("Y:");
  Serial.println(y);
  if (x < 100) {
    direction = "RightOut";
  }
  if (y < -150) {
    direction = "RightUp";
  }
  if (x < 250 && x > 230) {
    direction = "RightCenter";
  }
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
  digitalWrite(SPEAKER, LOW);
}

void turnLedsOn() {
  leds.setColorRGB(0, 225, 225, 225);
  leds.setColorRGB(1, 225, 225, 225);
  leds.setColorRGB(2, 225, 225, 225);
  leds.setColorRGB(3, 225, 225, 225);
    digitalWrite(SPEAKER, LOW);
}


void setScoreCursor(int currentScore, int mode) {
  int row = 0;
  if (mode == 3) {
    row = 1;
  }
  //game mode
  if (currentScore < 10) {
    lcd.setCursor(11, row);
  } else if (currentScore < 100) {
    lcd.setCursor(10, row);
  } else if (currentScore < 1000) {
    lcd.setCursor(9, row);
  } else {
    lcd.setCursor(8, row);
  }
  if (gameState != 2) {
    lcd.print(F("Pts:"));
  } else {
    lcd.setCursor(14, 0);
  }
}

int getScore() {
  int score = 10;
  return score;
}

void checkScore(int currentGameScore, int level) {
  lcd.setCursor(0, 1);
  lcd.print(F("Your score:  "));
  lcd.print(currentGameScore);
  delay(3000);
  if (level == 3) {
    lcd.setCursor(0, 0);
    lcd.print(F("Score not saved"));
  } else if (currentGameScore > highScores[level]) {
    highScores[level] = currentGameScore;
    lcd.setCursor(0, 0);
    lcd.print(F("New High Score!"));
    delay(1000);
    if (level == 0) {
      Serial.print(F("SAVING SCORE TO EASY"));
      EEPROM.put(eeAddressEasy, highScores[level]);
    } else if (level == 1) {
      Serial.print(F("SAVING SCORE TO MEDIUM"));
      EEPROM.put(eeAddressMedium, highScores[level]);
    } else if (level == 2) {
      Serial.print(F("SAVING SCORE TO HARD"));
      EEPROM.put(eeAddressHard, highScores[level]);
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print(F("Try again!      "));
  }
  delay(2000);
}

void viewMenu(String menu, int menu_item) {
  viewHighScore = false;
  lcd.setCursor(0, 0);
  if (menu == "main") {
    lcd.clear();
    lcd.print(F("Menu:"));
    print_line(1, current_menu[current_menu_item]);
  } else if (menu == "pause") {
    lcd.print(F("Game paused:  "));
    if (!tutorialMode) {
      lcd.setCursor(14, 0);
      lcd.print(currentGameScore);
    } else {
      lcd.setCursor(14, 0);
      lcd.print("  ");
    }
    print_line(1, pause_menu[current_pause_menu_item]);
  }
}

void viewHighScores() {
  viewHighScore = true;
  lcd.clear();
  lcd.print(F("High Scores:"));

  for (int i = 0; i < 3; i++) {
    if (i == 0) {
      lcd.setCursor(0, 1);
      lcd.print(F("E:"));
      lcd.print(highScores[i]);
    }
    if (i == 1) {
      lcd.setCursor(5, 1);
      lcd.print(F("M:"));
      lcd.print(highScores[i]);
    }
    if (i == 2) {
      lcd.setCursor(10, 1);
      lcd.print(F("H:"));
      if (highScores[i] < 100) {
        lcd.print(highScores[i]);
        lcd.print(" ");
      } else {
        lcd.print(highScores[i]);
      }
    }
  }
}

void setup() {
  // Uncomment the below, run once, then comment out again. To test - scoreboard should show 0 for all levels. 
  for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
   }
  lcd.begin(16, 2);
  accelemeterL.init();
  adxlR.powerOn();
  Serial.begin(9600);
  randomSeed(analogRead(A1));
  pinInit();

  leds.init();
  joystickPressed = false;
  // Init vars
  playingGame = false;
  viewHighScore = false;
  current_menu_item = 0;
  current_pause_menu_item = 0;
  last_joy_read = none;
  current_menu = main_menu;
  currentGameScore = 0;

  gameCount = 0;
  gameState = 0;

  eeAddressEasy = 0;  // location for storing high score easy
  eeAddressMedium += sizeof(eeAddressEasy);
  eeAddressHard += sizeof(eeAddressMedium);
  tutorialMode = false;
  // Print menu template on lcd.
  lcd.print(F("Menu:"));
  print_line(1, main_menu[current_menu_item]);
  EEPROM.get(eeAddressEasy, highScores[0]);
  EEPROM.get(eeAddressMedium, highScores[1]);
  EEPROM.get(eeAddressHard, highScores[2]);
}

void printScore(bool point, int points) {
  if (tutorialMode) {
    lcd.clear();
  }
  if (point) {
    sound(0);
    lcd.setCursor(0, 0);
    lcd.print(F("CORRECT!"));
    lcd.setCursor(0, 1);
    lcd.print(F("+"));
    lcd.setCursor(1, 1);
    lcd.print(10);
  } else {
    sound(1);
    lcd.setCursor(0, 0);
    lcd.print(F("WRONG!"));
  }
}

String getDirection() {
  direction = random(sizeof(directions) / sizeof(char *));
  if (directions[direction] == "LeftOut") {
    leds.setColorRGB(0, 225, 0, 0);
    if (tutorialMode) {
      lcd.setCursor(0, 0);
      lcd.print(F("<- Turn left"));
      lcd.setCursor(0, 1);
      lcd.print(F("arm OVER & hold"));
    }
  } else if (directions[direction] == "LeftUp") {
    leds.setColorRGB(1, 0, 225, 0);
    if (tutorialMode) {
      lcd.setCursor(0, 0);
      lcd.print(F("^ Lift left"));
      lcd.setCursor(0, 1);
      lcd.print(F("arm UP & hold"));
    }
  } else if (directions[direction] == "RightUp") {
    leds.setColorRGB(2, 0, 225, 0);
    if (tutorialMode) {
      lcd.setCursor(0, 0);
      lcd.print(F("Lift right ^"));
      lcd.setCursor(0, 1);
      lcd.print(F("arm UP & hold"));
    }
  } else if (directions[direction] == "RightOut") {
    leds.setColorRGB(3, 225, 0, 0);
    if (tutorialMode) {
      lcd.setCursor(0, 0);
      lcd.print(F("Turn right ->"));
      lcd.setCursor(0, 1);
      lcd.print(F("arm OVER & hold"));
    }
  }
  int current_joy_read = read_joystick();
  checkState(current_joy_read);
  return directions[direction];
}

void checkState(int current_joy_read) {
  if (current_joy_read != last_joy_read) {
    last_joy_read = current_joy_read;
    switch (current_joy_read) {
      case right:
        if (!viewHighScore) {
          move_left("main");
          viewMenu("main", current_menu_item);
        }
        if (gameState == 2) {
          move_left("pause");
          viewMenu("pause", current_pause_menu_item);
        }
        break;
      case left:
        if (!viewHighScore) {
          move_right("main");
          viewMenu("main", current_menu_item);
        }
        if (gameState == 2) {
          move_right("pause");
          viewMenu("pause", current_pause_menu_item);
        }
        break;
      case enter:
        if (!playingGame) {
          if (current_menu_item == 0) {  //tutorial
            gameState = 1;
            playingGame = true;
            tutorialMode = true;
            difficulty = 3000;
            level = 4;
          }
          if (current_menu_item == 1) {  //easy
            playingGame = true;
            difficulty = 2000;
            level = 0;
            gameState = 1;
          }
          if (current_menu_item == 2) {  //medium
            playingGame = true;
            difficulty = 1000;
            level = 1;
            gameState = 1;
          }
          if (current_menu_item == 3) {  //hard
            playingGame = true;
            difficulty = 500;
            level = 2;
            gameState = 1;
          }
          if (current_menu_item == 4) {  //view high score
            if (viewHighScore) {         // If already displaying high score, return to menu
              viewMenu("main", current_menu_item);
            } else {
              viewHighScores();
            }
          }
        } else if (gameState == 2) {           //if game pause
          if (current_pause_menu_item == 0) {  //resume game
            Serial.print("Resuming game: ");
            gameState = 1;
          }
          if (current_pause_menu_item == 1) {  //restart game
            Serial.print(F("Restarting game: "));
            gameState = 1;
            gameCount = 0;
          }
          if (current_pause_menu_item == 2) {  //exit game
            Serial.print("Exiting game: ");
            gameState = 0;
            gameCount = 0;
            playingGame = false;
            tutorialMode = false;
            viewMenu("main", current_menu_item);
          }

        } else if (gameState == 1) {          // if game playing
          Serial.print(F("Pausing game: "));  //pause game
          gameState = 2;
          if (gameCount != gameLength || tutorialMode) {
            viewMenu("pause", current_pause_menu_item);
            print_line(1, pause_menu[current_pause_menu_item]);
          }
        } else if (tutorialMode) {
          Serial.print(F("TUTORIAL MODE PAUSE"));
          viewMenu("pause", current_pause_menu_item);
          gameState = 2;
        }
        break;
      default:
        break;
    }
  }
}

void printCountdown() {
  for (int i = 3; i > -1; i--) {
    lcd.clear();
    lcd.setCursor(0, 1);
    if (i == 0) {
      lcd.print(F("GO!"));
      sound(3);
      break;
    }
    lcd.print(i);
    sound(2);
    delay(1000);
  }
}

void printInstructions() {
  //Instructions
  int current_joy_read = read_joystick();
  checkState(current_joy_read);
  lcd.print(F("Face hands ahead"));
  lcd.setCursor(0, 1);
  lcd.print(F("palms face down "));
  delay(4000);

  if (tutorialMode) {
    current_joy_read = read_joystick();
    checkState(current_joy_read);

    current_joy_read = read_joystick();
    checkState(current_joy_read);

    lcd.setCursor(0, 0);
    lcd.print(F("Wear wristbands "));
    lcd.setCursor(0, 1);
    lcd.print(F("cables go out    "));
    delay(5000);

    current_joy_read = read_joystick();
    checkState(current_joy_read);

    lcd.setCursor(0, 0);
    lcd.print(F("Correct move    "));
    lcd.setCursor(0, 1);
    lcd.print(F("get 10 points  "));
    delay(5000);

    current_joy_read = read_joystick();
    checkState(current_joy_read);

    lcd.setCursor(0, 0);
    lcd.print(F("To pause game  "));
    lcd.setCursor(0, 1);
    lcd.print(F("Hold joystick in"));
    delay(5000);

    current_joy_read = read_joystick();
    checkState(current_joy_read);

    lcd.setCursor(0, 0);
    lcd.print(F("Tutorial scores "));
    lcd.setCursor(0, 1);
    lcd.print(F("won't be saved   "));
    delay(5000);
  }
}

void loop() {
    digitalWrite(SPEAKER, LOW);

  String directionR = read_accel_R();
  String directionL = read_accel_L();
  Serial.println(directionR);
  Serial.println(directionL);
  int current_joy_read = read_joystick();
  checkState(current_joy_read);

  if (gameState == 1) {
    //if game has not played any rounds
    if (gameCount == 0) {
      //start new game
      currentGameScore = 0;
      turnLedsOn();
      lcd.clear();
      printInstructions();
      current_joy_read = read_joystick();
      checkState(current_joy_read);

      String startingPositionL;
      String startingPositionR;
      while (startingPositionL != "LeftCenter" && startingPositionR != "RightCenter" && gameState != 2) {
        current_joy_read = read_joystick();
        checkState(current_joy_read);
        startingPositionL = read_accel_L();
        startingPositionR = read_accel_R();
      }
    }
    if (gameState != 2) {
      printCountdown();
    }
    delay(500);

    while (gameState == 1 && gameCount < gameLength) {
      current_joy_read = read_joystick();
      checkState(current_joy_read);
      lcd.clear();

      if (!tutorialMode) {
        setScoreCursor(currentGameScore, 2);
        lcd.print(currentGameScore);
      }

      // choose random direction
      String randomDirection = getDirection();
      current_joy_read = read_joystick();
      checkState(current_joy_read);
      delay(difficulty);

      // read hand direction
      String newDirectionL = read_accel_L();
      String newDirectionR = read_accel_R();

      if (newDirectionL == randomDirection || newDirectionR == randomDirection) {
        if (!tutorialMode) {
          int pointsToAdd = getScore();
          currentGameScore += pointsToAdd;
          printScore(true, pointsToAdd);
        }
        current_joy_read = read_joystick();
        checkState(current_joy_read);
        printScore(true, 0);
      } else {
        printScore(false, 0);
        current_joy_read = read_joystick();
        checkState(current_joy_read);
      }
      // print current score on the top right corner
      if (!tutorialMode) {
        setScoreCursor(currentGameScore, 2);
        lcd.print(currentGameScore);
      }
      current_joy_read = read_joystick();
      checkState(current_joy_read);
      gameCount++;
      turnLedsOn();
      delay(1000);
    }

    if (gameCount == gameLength) {
      sound(3);
      digitalWrite(SPEAKER, LOW);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Game Over      "));
      if (!tutorialMode) {
        checkScore(currentGameScore, level);
      } else {
        tutorialMode = false;
        delay(2000);
      }
      gameState = 0;
      gameCount = 0;
      playingGame = false;
      viewMenu("main", current_menu_item);
    }
  }
  // delay(1000);
}
