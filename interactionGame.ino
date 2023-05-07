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

bool playingGame;  // has game been started

//Directions
char *directions[] = { "LeftOut", "LeftUp", "RightOut", "RightUp" };
int direction;

int highScores[3];
int eeAddressEasy;  // location for storing high score easy
int eeAddressMedium;
int eeAddressHard;

int currentGameScore;

int gameLength = 3;
int gameCount;
int difficulty;
bool tutorialMode;
int gameState; //0 = not started/ennded , 1 = playing, 2 = paused

byte level;
// Menus
String main_menu[] = { "<   Tutorial   >", "<  Easy Level  >", "< Medium Level >", "<  Hard Level  >", "<  Scoreboard  >" };

// Current menu and item to be displayed
int current_menu_item;
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

void move_right() {
  current_menu_item == 4 ? current_menu_item = 0 : current_menu_item += 1;
}

void move_left() {
  current_menu_item == 0 ? current_menu_item = 4 : current_menu_item -= 1;
}

String read_accel_L() {
  String direction = "";

  int8_t x;
  int8_t y;
  int8_t z;
  accelemeterL.getXYZ(&x, &y, &z);
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
}

void turnLedsOn() {
  leds.setColorRGB(0, 225, 225, 225);
  leds.setColorRGB(1, 225, 225, 225);
  leds.setColorRGB(2, 225, 225, 225);
  leds.setColorRGB(3, 225, 225, 225);
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
  lcd.print(F("Pts:"));
}

int getScore() {
  int score = 10;
  return score;
}

void checkScore(int currentGameScore, int level) {
  Serial.print(F("LEVEL"));
  Serial.println(level);
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
}

void viewMenu() {
  viewHighScore = false;
  lcd.clear();
  lcd.print(F("Menu:"));
  print_line(1, current_menu[current_menu_item]);
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
      lcd.setCursor(6, 1);
      lcd.print(F("M:"));
      lcd.print(highScores[i]);
    }
    if (i == 2) {
      lcd.setCursor(12, 1);
      lcd.print(F("H:"));
      lcd.print(highScores[i]);
    }
  }
}

void setup() {
  // for (int i = 0 ; i < EEPROM.length() ; i++) {
  //     EEPROM.write(i, 0);
  //  }
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
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("WRONG!"));
  }
}

void displayMenu(int current_joy_read) {
  if (current_joy_read != last_joy_read) {
    last_joy_read = current_joy_read;
    switch (current_joy_read) {
      case right:
        move_right();
        viewMenu();
        break;
      case left:
        move_left();
        viewMenu();
        break;
      case enter:  // If joystick is pressed down
        if (current_menu_item == 0) { //tutorial
          gameState = 1;
          playingGame = true;
          tutorialMode = true;
          difficulty = 3000;
          level = 4;
        }
        if (current_menu_item == 1) { //easy
          playingGame = true;
          difficulty = 3000;
          level = 0;
          gameState = 1;
        }
        if (current_menu_item == 2) { //medium
          playingGame = true;
          difficulty = 2000;
          level = 1;
          gameState = 1;
        }
        if (current_menu_item == 3) { //hard
          playingGame = true;
          difficulty = 1000;
          level = 2;
          gameState = 1;
        }
        if (current_menu_item == 4) {
          if (viewHighScore) {  // If already displaying high score, return to menu
            viewMenu();
          } else {
            viewHighScores();
          }
        }
        break;
      default:
        break;
    }
  }
}

String getDirection(){
  direction = random(sizeof(directions) / sizeof(char *));
  if (directions[direction] == "LeftOut") {
    leds.setColorRGB(0, 225, 0, 0);
    if (tutorialMode) {
      lcd.setCursor(0, 0);
      lcd.print(F("<- Flip left"));
      lcd.setCursor(0, 1);
      lcd.print(F("  wrist "));
    }
  } else if (directions[direction] == "LeftUp") {
    leds.setColorRGB(1, 0, 225, 0);
    if (tutorialMode) {
      lcd.setCursor(0, 0);
      lcd.print(F("^ Lift left"));
      lcd.setCursor(0, 1);
      lcd.print(F("  arm UP"));
    }
  } else if (directions[direction] == "RightUp") {
    leds.setColorRGB(2, 0, 225, 0);
    if (tutorialMode) {
      lcd.setCursor(0, 0);
      lcd.print(F("Lift right ^"));
      lcd.setCursor(0, 1);
      lcd.print(F("arm"));
    }
  } else if (directions[direction] == "RightOut") {
    leds.setColorRGB(3, 225, 0, 0);
    if (tutorialMode) {
      lcd.setCursor(0, 0);
      lcd.print(F("Flip right ->"));
      lcd.setCursor(0, 1);
      lcd.print(F("arm OUT"));
    }
  }
  return directions[direction];
}

void checkState(){
  bool joystickClicked = false;
  int joystick = read_joystick();
  switch (joystick) {
    case enter:
      joystickClicked = true;
      break;
    default:
      joystickClicked = false;
      break;
    }

  if(joystickClicked && gameState == 2){
    Serial.print("joystickClicked: ");
    Serial.println(joystickClicked);
    gameState =1;
  }else if(joystickClicked && gameState == 1){
    Serial.print("joystickClicked: ");
    Serial.println(joystickClicked);
    gameState = 2;
  }
}

void printCountdown(){
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

void printInstructions(){
  //Instructions
  lcd.print(F("Face hands ahead"));
  lcd.setCursor(0, 1);
  lcd.print(F("palms face down "));
  delay(2000);

  if (tutorialMode) {
    lcd.setCursor(0, 0);
    lcd.print(F("Tutorial scores "));
    lcd.setCursor(0, 1);
    lcd.print(F("won't be saved "));
    delay(2000);
    lcd.setCursor(0, 0);
    lcd.print(F("To exit game"));
    lcd.setCursor(0, 1);
    lcd.print(F("Hold joystick in"));
    delay(2000);
    lcd.setCursor(0, 0);
    lcd.print(F("Face hands ahead"));
    lcd.setCursor(0, 1);
    lcd.print(F("palms face down "));
    delay(2000);
  }
}

void loop() {
  checkState();

  if(playingGame == false){
    int current_joy_read = read_joystick();
    displayMenu(current_joy_read);
  }
  //  if game has been paused
  else if(gameState == 2){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Game Paused"));
    lcd.setCursor(0,1);
    lcd.print(F("<Resume    Exit>"));
  }
  // if game has begun
  else if(gameState == 1){
    //if game has not played any rounds 
    if(gameCount == 0){
      //start new game
      currentGameScore = 0;
      turnLedsOn();
      lcd.clear();
      printInstructions();
      checkState();

      String startingPositionL = read_accel_L();
      String startingPositionR = read_accel_R();
      while (startingPositionL != "LeftCenter" && startingPositionR != "RightCenter") {
        checkState();
        startingPositionL = read_accel_L();
        startingPositionR = read_accel_R();
      }
    }

    printCountdown();
    delay(500);

    while (gameState == 1 && gameCount < gameLength) {
      checkState();
      lcd.clear();

      if (!tutorialMode) {
        setScoreCursor(currentGameScore, 2);
        lcd.print(currentGameScore);
      }

      // choose random direction
      String randomDirection = getDirection();
      checkState();
      delay(difficulty);

      // read hand direction
      String newDirectionL = read_accel_L();
      String newDirectionR = read_accel_R();

      if (newDirectionL == randomDirection || newDirectionR == randomDirection) {
        sound(0);
        if (!tutorialMode) {
          int pointsToAdd = getScore();
          currentGameScore += pointsToAdd;
          printScore(true, pointsToAdd);
        }
        printScore(true, 0);
      } else {
        sound(1);
        printScore(false, 0);
      }
      // print current score on the top right corner
      if (!tutorialMode) {
        setScoreCursor(currentGameScore, 2);
        lcd.print(currentGameScore);
      }
      checkState();
      gameCount++;
      turnLedsOn();
      delay(1000);
    }

    if (gameCount == gameLength) {
      sound(3);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Game Over      "));
      if (!tutorialMode) {
        checkScore(currentGameScore, level);
      } else {
        tutorialMode = false;
      }
      gameState = 0;
      gameCount = 0;
      playingGame = false;
      viewMenu();
    } 
  }
}
