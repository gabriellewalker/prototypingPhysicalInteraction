#include <Wire.h>
#include <ADXL345.h>
#include "rgb_lcd.h"
#include <CMBMenu.hpp>
#include "MMA7660.h"
#include <ChainableLED.h>
#include <EEPROM.h>

#define NOTE_E 165

MMA7660 accelemeterL;
ADXL345 adxlR; // variable adxl is an instance of the ADXL345 library

// Joystick values
#define right 0
#define left 1
#define enter 2
#define none 3

#define NUM_LEDS 4

// speaker pin
#define SPEAKER 3

// speaker tones
int BassTab[] = {300, 1911, 1000, 500};

rgb_lcd lcd;
ADXL345 adxl;     // variable adxl is an instance of the ADXL345 library
bool playingGame; // has game been started

ChainableLED leds(7, 8, NUM_LEDS);

char *directions[] = {"LeftOut", "LeftUp", "RightOut", "RightUp"};
long direction;

int currentGameScore;
int highScore;
int eeAddress = 0; // lcation for storing high score

int gameLength = 5;

// Menus
String main_menu[] = {"<  Start Game  >", "<  Scoreboard  >"};

// Current menu and item to be displayed
int current_menu_item;
bool viewHighScore;
String *current_menu;
// Used to check joystick state
int last_joy_read;

int read_joystick()
{
  int output = none;
  // read all joystick x value
  int x = analogRead(A0);
  int y = analogRead(A1);

  if (x == 1023)
  {
    output = enter;
  }
  else if (y >= 740)
  {
    output = right;
  }
  else if (y <= 275)
  {
    output = left;
  }

  return output;
}

void print_line(int line, String text)
{
  lcd.setCursor(0, line);
  lcd.setCursor(0, line);
  lcd.print(text);
}

void move_right()
{
  if (current_menu_item == 1)
  {
    current_menu_item = 0;
  }
  else
  {
    current_menu_item = 1;
  }
}

void move_left()
{

  if (current_menu_item == 1)
  {
    current_menu_item = 0;
  }
  else
  {
    current_menu_item = 1;
  }
}

String read_accel_L()
{
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
  if (x > -10)
  {
    direction = "LeftOut";
  }
  if (y < -15)
  {
    direction = "LeftUp";
  }
  if (x > -25 && x < -18)
  {
    direction = "LeftCenter";
  }
  return direction;
}

String read_accel_R()
{
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

  if (x < 100)
  {
    direction = "RightOut";
  }
  if (y < -150)
  {
    direction = "RightUp";
  }
  if (x < 250 && x > 230)
  {
    direction = "RightCenter";
  }
  return direction;
}

void pinInit()
{
  pinMode(SPEAKER, OUTPUT);
  digitalWrite(SPEAKER, LOW);
}

void sound(uint8_t note_index)
{
  for (int i = 0; i < 100; i++)
  {
    digitalWrite(SPEAKER, HIGH);
    delayMicroseconds(BassTab[note_index]);
    digitalWrite(SPEAKER, LOW);
    delayMicroseconds(BassTab[note_index]);
  }
}

void setup()
{

  lcd.begin(16, 2);
  accelemeterL.init();
  adxlR.powerOn();
  Serial.begin(9600);
  randomSeed(666);
  pinInit();

  leds.init();

  // Print menu template on lcd.
  lcd.setCursor(0, 0);
  lcd.print("Menu:");
  lcd.setCursor(0, 1);
  lcd.print(main_menu[current_menu_item]);

  EEPROM.get(eeAddress, highScore);

  // Init vars
  playingGame = false;
  viewHighScore = false;
  current_menu_item = 0;
  last_joy_read = none;
  current_menu = main_menu;
}

void loop()
{
  int current_joy_read = read_joystick();
  if (current_joy_read != last_joy_read)
  {
    last_joy_read = current_joy_read;

    switch (current_joy_read)
    {
    case right:
      move_right();
      break;
    case left:
      move_left();
      break;
    case enter:
      Serial.print("View high schore?");
      Serial.println(viewHighScore);
      if (current_menu_item == 0)
      {
        playingGame = true;
        break;
      }
      if (current_menu_item == 1)
      {
        if (viewHighScore)
        {
          viewHighScore = false;
        }
        else
        {
          viewHighScore = true;
        }
        break;
      }
    default:
      break;
    }
  }
  delay(100);
  lcd.clear();
  lcd.setCursor(0, 0);
  // if game has not been started - show the menu
  if (!playingGame && !viewHighScore)
  {
    lcd.print("Menu:");
    print_line(1, current_menu[current_menu_item]);
    if (highScore < 10)
    {
      lcd.setCursor(12, 0);
    }
    else
    {
      lcd.setCursor(11, 0);
    }
    lcd.print("HS:");
    lcd.print(highScore);
    currentGameScore = 0;
  }
  // if view high score has been selected
  else if (viewHighScore)
  {
    lcd.print("High Score:");
    lcd.setCursor(0, 1);
    lcd.print(highScore);
  }
  else
  { // if game has started

    leds.setColorRGB(0, 225, 225, 225);
    leds.setColorRGB(1, 225, 225, 225);
    leds.setColorRGB(2, 225, 225, 225);
    leds.setColorRGB(3, 225, 225, 225);
    lcd.write("Center Arms");
    String startingPositionL = read_accel_L();
    String startingPositionR = read_accel_R();

    int gameCount = 0;

    Serial.println(startingPositionL);
    Serial.println(startingPositionR);
    delay(3000);

    bool countdown = false;
    // once centered
    while (startingPositionL == "LeftCenter" && startingPositionR == "RightCenter" && gameCount < gameLength)
    {
      // start 3 second countdown
      if(!countdown){
        for (int i = 3; i > -1; i--){
          lcd.clear();
          lcd.setCursor(0, 1);
          if(i==0){
            lcd.print("GO!");
            sound(3);
            countdown = true;
            break;
          }
          lcd.print(i);
          sound(2);
          delay(1000);
        }
      }

      delay(1500);
      lcd.clear();
      lcd.setCursor(0, 0);
      // choose random direction
      direction = random(sizeof(directions) / sizeof(char *));
      Serial.print(directions[direction]);
      if (directions[direction] == "LeftOut")
      {
        leds.setColorRGB(0, 225, 0, 0);
      }
      else if (directions[direction] == "LeftUp")
      {
        leds.setColorRGB(1, 0, 0, 255);
      }
      else if (directions[direction] == "RightUp")
      {
        leds.setColorRGB(2, 0, 0, 255);
      }
      else if (directions[direction] == "RightOut")
      {
        leds.setColorRGB(3, 225, 0, 0);
      }
      // print current score on the bottom right corner
      if (currentGameScore < 10)
      {
        lcd.setCursor(15, 1);
      }
      else
      {
        lcd.setCursor(14, 1);
      }
      lcd.print(currentGameScore);

      delay(1500);

      // read hand direction
      String newDirectionL = read_accel_L();
      String newDirectionR = read_accel_R();
      if (newDirectionL == directions[direction])
      {
        sound(0);
        lcd.setCursor(0, 1);
        lcd.write("CORRECT!");

        currentGameScore++;
      }
      else if (newDirectionR == directions[direction])
      {
        sound(0);
        lcd.setCursor(0, 1);
        lcd.write("CORRECT!");
        currentGameScore++;
      }
      else
      {
        sound(1);
        lcd.setCursor(0, 1);
        lcd.write("WRONG!");
      }
      gameCount++;

      leds.setColorRGB(0, 225, 225, 225);
      leds.setColorRGB(1, 225, 225, 225);
      leds.setColorRGB(2, 225, 225, 225);
      leds.setColorRGB(3, 225, 225, 225);
      delay(1000);
    }

    if (gameCount == gameLength)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.write("Game Over");
      if (currentGameScore > highScore)
      {
        highScore = currentGameScore;
        EEPROM.put(eeAddress, highScore);
        lcd.setCursor(0, 1);
        lcd.write("New High Score!");
        if (currentGameScore < 10)
        {
          lcd.setCursor(15, 1);
        }
        else
        {
          lcd.setCursor(14, 1);
        }
        lcd.print(currentGameScore);
      }
      else
      {
        lcd.setCursor(0, 1);
        lcd.write("Try harder");
        if (currentGameScore < 10)
        {
          lcd.setCursor(15, 1);
        }
        else
        {
          lcd.setCursor(14, 1);
        }
        lcd.print(currentGameScore);
      }
      delay(3000);
      playingGame = false;
    }
  }
}