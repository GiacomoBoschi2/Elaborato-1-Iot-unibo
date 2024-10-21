#include "TimerOne.h"
#include <LiquidCrystal_I2C.h>
#include <stdlib.h>
#include <time.h>


#define LED_PINB4 5
#define LED_PINB3 4
#define LED_PINB2 3
#define LED_PINB1 2
#define LED_OUTPUT 6
#define OUTPUT_LED_OFFSET 2     
#define TIME_LIMIT_FOR_STARTING 10000000
#define TICK 10000
#define TICK_PER_SECOND 1000000
#define TICK_LED_OUTPUT 1000
#define STARTING_END_TIME 20000000
#define POTENTIOMETER A0
#define GET_SCORE(left,level,max_time) (((level)*10)+((((max_time)-(left))*100)/(max_time)))
#define LOWER_TIME_BOUND 5000000
#define MAX_OUTPUT_BRIGHTNESS 255
#define REFRESH_LCD_RATE 100000

enum GAMESTATUS{
  OFF = 0,
  READY = 1,
  RUNNING = 2,
};

//variabili
GAMESTATUS status = READY;

long score = 0;
int current_loading_screen_brightness = 0;
int increasing_bright = 1;
int difficulty = 0;
int flagState = 1;
long timeOff = 0;
long time_limits[] = {500000,750000,1000000,1500000};
int ready_timeout = 0;
int current_guess = 0;
int to_guess = -1;
int current_round = 1;
long time_limit = 0;
int led_status[] = {0,0,0,0};
int led_changed_status[] = {0,0,0,0};
const int output_leds[] = {9,10,11,12};
int button_pression[] = {0,0,0,0};
int refresh_seconds = 0;
LiquidCrystal_I2C lcd(0x27,20,4);

void start_beginning_timer(int restarting=0){
  timeOff = 0;
  if(status == READY) {
    Timer1.initialize(TICK_LED_OUTPUT); 
    Timer1.attachInterrupt(check_status_loop);
  }
  else if(status == RUNNING){
    Timer1.initialize(TICK);
    Timer1.attachInterrupt(running_status_loop);
  }

  if(!restarting){
    Timer1.start();
  }
  else{
    Timer1.restart();
  }
}

//funzioni di display dello schermo lcd
void display_starting_screen(){
   
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Welcome to GMB!");
  lcd.setCursor(1,1);
  lcd.print("Press B1 to start.");
  lcd.setCursor(1,2);

  char* difficulty_display = "Difficulty:";
  lcd.setCursor(1,3);
  lcd.print(difficulty_display);
  //lcd.setCursor(12,3);
  lcd.print(difficulty+1);


}

void animate_start_round(){
  lcd.clear();
  lcd.setCursor(7,1);
  lcd.print("GO!");
  delay(2000);
}

void gameOver(){
  Serial.println("Game Over!");
  Serial.print("Your final score:");
  Serial.println(score);
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Game Over!");
  lcd.setCursor(1,1);
  lcd.print("Your final score:");
  lcd.setCursor(1,2);
  lcd.print(score);
  score = 0;
  Timer1.detachInterrupt();

  //reset led
  for(int i=0;i<4;i++){
    led_status[i] = 0;
    button_pression[i] = 0;
    digitalWrite(output_leds[i],LOW);
  }

  timeOff = 0;
  digitalWrite(LED_OUTPUT,HIGH);
  delay(1000);
  digitalWrite(LED_OUTPUT,LOW);
  delay(9000);
  status = READY;
  
  display_starting_screen();
  start_beginning_timer();
}

void display_round(){
  lcd.clear();
  long seconds_left = (time_limit-timeOff)/TICK_PER_SECOND;
  Serial.println(seconds_left);

  lcd.setCursor(1,0);
  lcd.print("Number to guess: ");
  lcd.print(to_guess);

  lcd.setCursor(1,1);
  lcd.print("Round. ");
  lcd.print(current_round);
  
  lcd.setCursor(1,2);
  lcd.print("Time left: ");
  lcd.print(seconds_left);

  lcd.setCursor(1,3);
  lcd.print("Your guess: ");
  lcd.print(current_guess);
}

void display_success(){
      lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("GOOD!");
        lcd.setCursor(1,1);
        lcd.print("Your score:");
        lcd.setCursor(1,2);
        lcd.print(score);
        delay(1500);
}


//inizializza l'ambiente di gioco per un round in base al numero del round
void start_game(){
  to_guess = rand()%16;
  Serial.print("Number to guess:");
  Serial.println(to_guess);
  current_guess = 0;
  time_limit = max(LOWER_TIME_BOUND,STARTING_END_TIME - time_limits[difficulty]*(current_round-1));
  //reset leds
  for(int i=0;i<4;i++){
    led_status[i] = 0;
    digitalWrite(output_leds[i],LOW);
  }
  if(current_round<=1){
    animate_start_round();
  }
  start_beginning_timer();
  display_round();
}

void check_ready(){
  int isB1Pressed = digitalRead(LED_PINB1);
  if(isB1Pressed == HIGH){
    status = RUNNING;
    difficulty = analogRead(POTENTIOMETER)/256; //TODO: leggere valore potenziometro.
    Serial.println("difficulty selected:");
    Serial.println(difficulty);
    Timer1.detachInterrupt();
    digitalWrite(LED_OUTPUT,LOW);
    Serial.println("Ready to play");
    current_round = 1;
    start_game();
  }
  else if(timeOff>TIME_LIMIT_FOR_STARTING){
    status = OFF;
    timeOff = 0;
    Timer1.detachInterrupt();
    digitalWrite(LED_OUTPUT,LOW);
    Serial.println("Went offline");
    lcd.clear();
    lcd.noBacklight();
  }
  else{
    //scrivi difficolata attualmente selezionata
    int old = difficulty;
    difficulty = analogRead(POTENTIOMETER)/256; 
    if(old!=difficulty){
      display_starting_screen();
    }
  }
}

void check_off(){

  int b1 = digitalRead(LED_PINB1);
  int b2 = digitalRead(LED_PINB2);
  int b3 = digitalRead(LED_PINB3);
  int b4 = digitalRead(LED_PINB4);

  int toReboot = (b1==HIGH) || (b2==HIGH) || (b3==HIGH) || (b4==HIGH);

  if(toReboot){
    status = READY;
    start_beginning_timer(1);
    Serial.println("Ready to restart");
    lcd.backlight();
    display_starting_screen();
    delay(300);
  }
}

//controlla se un bottone è premuto
//

void  check_status_loop(){
  if(current_loading_screen_brightness + increasing_bright > MAX_OUTPUT_BRIGHTNESS || current_loading_screen_brightness + increasing_bright < 0){
    increasing_bright*=-1;
  }

  current_loading_screen_brightness+=increasing_bright;
  analogWrite(LED_OUTPUT, current_loading_screen_brightness);  
  timeOff+=TICK_LED_OUTPUT;
  flagState = !flagState; 
}

void running_status_loop(){
  timeOff+=TICK;
  if(timeOff%TICK_PER_SECOND==0){
    refresh_seconds = 1;
  }
}

int getGuessChanges(){
  int sum = 0;
  int vals[] = {1,2,4,8};
  for(int i = 0;i<4;i++){
      int state = digitalRead(i+OUTPUT_LED_OFFSET);
      if(state == HIGH && button_pression[i]==LOW){
          button_pression[i] = HIGH;
          led_status[i] = !led_status[i];
          if(led_status[i]){
            sum+=vals[3-i];
          }
          else{
            sum-=vals[3-i];
          }
          digitalWrite(output_leds[i],led_status[i]);
      }
      if(state==LOW && button_pression[i]){
        button_pression[i] = LOW;
      }
  }
  return sum;
}


void play(){
    if(current_guess==to_guess){
        //round vinto,generazione successivo
        current_round+=1;
        score += GET_SCORE(timeOff,current_round,time_limit);
        Serial.println("Guessed correctly");
        Serial.print("Your score:");
        Serial.println(score);
        display_success();
        start_game();
    }
    else{
      int old = current_guess;
      current_guess+=getGuessChanges();
      if(refresh_seconds){
        refresh_seconds = 0;
        display_round();
      }
      else if (old!=current_guess){
        display_round();
      }

      //controlla se il tempo è esaurito
      if(timeOff > time_limit){
        gameOver();
      }
    }
    
}

void setup() {
  //randomizza seed per i numeri da generare
  srand(analogRead(-1));
  //setup pins
  pinMode(LED_OUTPUT, OUTPUT);
  for(int i = 0;i<4;i++){
    pinMode(output_leds[i],OUTPUT);
  }
  pinMode(LED_PINB4, INPUT);
  pinMode(LED_PINB3, INPUT);
  pinMode(LED_PINB2, INPUT);
  pinMode(LED_PINB1, INPUT);
  pinMode(POTENTIOMETER,INPUT);
  //serial setup
  Serial.begin(9600);     
  status = READY;
  //inizializza timer
  start_beginning_timer();

  //inizializza LCD
  lcd.init();
  lcd.backlight();
}

void loop() {
  switch(status){
    case OFF:check_off();break;
    case READY:check_ready();break;
    case RUNNING:play();
  }
}
