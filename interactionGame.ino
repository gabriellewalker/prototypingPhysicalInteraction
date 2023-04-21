#include <Wire.h>
#include <ADXL345.h>
#include "rgb_lcd.h"
#include <CMBMenu.hpp>
#include "MMA7660.h"
MMA7660 accelemeter;


//Joystick values
#define right 0
#define left  1
#define enter 2
#define none  3

#define ledLeft 4

//speaker pin
#define SPEAKER 3

//speaker tones
int BassTab[]={300,1911};

rgb_lcd lcd; 
ADXL345 adxl; //variable adxl is an instance of the ADXL345 library
bool playingGame; //has game been started 
char *directions[]={"Left", "Up"};
long direction;

int currentGameScore;
int highScore;

int gameLength = 5;

// Menus
String main_menu[] = {"  Start Game  >","<View High Score"};

// Current menu and item to be displayed
int current_menu_item;
bool viewHighScore;
String *current_menu;
// Used to check joystick state
int last_joy_read;

int read_joystick() {
  int output = none;
  // read all joystick x value
  int sensorValue1 = analogRead(A0);

  if (sensorValue1 == 1023){
    output = enter;
  } else if (sensorValue1 >= 750) {
    output = right;
  } else if (sensorValue1 <= 270) {
    output = left;
  }
  return output;
}

void print_line(int line, String text) {
    lcd.setCursor(0, line);
    lcd.setCursor(0, line);
    lcd.print(text);
}

void move_right(){
 if (current_menu_item == 1){
    current_menu_item=0;
  }else{
    current_menu_item=1;
  }
}

void move_left(){

   if (current_menu_item == 1){
    current_menu_item=0;
  } else {
    current_menu_item=1;
  } 
}

String read_accel(){
  String direction;

  int8_t x;
  int8_t y;
  int8_t z;
  float ax, ay, az;
  accelemeter.getXYZ(&x, &y, &z);

  Serial.print("x = ");
  Serial.println(x);
  Serial.print("y = ");
  Serial.println(y);
  Serial.print("z = ");
  Serial.println(z);

  if(x < -15){
  direction = "Left";
  }
  else if (y < -15){
    direction =  "Up";
  }
  else if (-15 < x < 50){
    direction =  "Center";
  }
  return direction;
}

void pinInit()
{
    pinMode(SPEAKER,OUTPUT);
    digitalWrite(SPEAKER,LOW);
}

void sound(uint8_t note_index)
{
    for(int i=0;i<100;i++)
    {
        digitalWrite(SPEAKER,HIGH);
        delayMicroseconds(BassTab[note_index]);
        digitalWrite(SPEAKER,LOW);
        delayMicroseconds(BassTab[note_index]);
    }
}

void setup() {

    lcd.begin(16, 2);
    accelemeter.init();
    Serial.begin(9600);
    randomSeed(666);
    pinInit();

    pinMode(ledLeft, OUTPUT);
    // Print menu template on lcd.
    lcd.setCursor(0, 0);
    lcd.print("Menu:");
    lcd.setCursor(0, 1);
    lcd.print(main_menu[current_menu_item]);
  
    // Init vars
    playingGame = false;
    viewHighScore = false;
    current_menu_item = 0;
    last_joy_read = none;
    current_menu = main_menu;
    adxl.powerOn();
}

void loop() {
  int current_joy_read = read_joystick();
  // Serial.print("Current Menu Item");
  // Serial.println(current_menu_item);
  if (current_joy_read != last_joy_read) {
    last_joy_read = current_joy_read;

    switch (current_joy_read) {
      case right:
        move_right();
        break;
      case left:
        move_left();
        break;
      case enter:
        Serial.print("View high schore?" );
        Serial.println(viewHighScore);
        if(current_menu_item == 0){
          playingGame = true;
          break;
        }
        if(current_menu_item == 1){
          if(viewHighScore){
            viewHighScore = false;
          }else{
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
    if(!playingGame && !viewHighScore){
      lcd.print("Menu:");
      print_line(1, current_menu[current_menu_item]);
      currentGameScore = 0;
    }
    //if view high score has been selected
    else if(viewHighScore){
      lcd.print("High Score:");
      lcd.setCursor(0, 1);
      lcd.print(highScore);
    }
    else { //if game has started
      //make sure hand starts in center

      lcd.write("Center hand");
      String startingPosition = read_accel();
      int gameCount = 0;
      //once centered
      while (startingPosition=="Center" && gameCount < gameLength){
        lcd.clear();
        lcd.setCursor(0, 0);
        //choose random direction (this will change to LEDs once we have them connected)
        direction = random(sizeof(directions)/sizeof(char*));
        lcd.write(directions[direction]);

        if(directions[direction]=="Left"){
          digitalWrite(ledLeft, HIGH);
        }

        //print current score on the bottom right corner
        if(currentGameScore < 10){
          lcd.setCursor(15,1);
        }else {
          lcd.setCursor(14,1);
        }
        lcd.print(currentGameScore);

        delay(1500);
        //read hand direction
        String newDirection = read_accel();
        if (newDirection == directions[direction]){
          sound(0);

          lcd.setCursor(0, 1);
          lcd.write("CORRECT!");
          currentGameScore++;
        }
        else{
          sound(1);

          lcd.setCursor(0, 1);
          lcd.write("WRONG!");
        }
        gameCount++;
        digitalWrite(ledLeft, LOW);

        delay(500); 
      } 
      
      if(gameCount == gameLength) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.write("Game Over");
        if (currentGameScore > highScore){
          highScore = currentGameScore;
          lcd.setCursor(0, 1);
          lcd.write("New High Score!");
        }
        else {
          lcd.setCursor(0, 1);
          lcd.write("Try harder");
        }
        delay(3000);
        playingGame = false;
      }
    } 
}