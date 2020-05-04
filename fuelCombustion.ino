#include <Wire.h>   // standardowa biblioteka Arduino
#include <LiquidCrystal_I2C.h> // dolaczenie pobranej biblioteki I2C dla LCD

#define RESTART_BUTTON 9 //PB1
#define AVERAGE_BUTTON 4 //PD4
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Ustawienie adresu ukladu na 0x27

byte inputMeter = PD2;            // 0 = digital pin 2
byte outputMeter = PD3;           // 1 = digital pin 3

int averageButtonStatus = 0;
int obsluga=1;
float inputAmoutPerImpuls  = 0.5336;      // ml per each impuls
float outputAmoutPerImpuls  = 0.5336;      // ml per each impuls
// float inputAmoutPerImpuls  = 0.31;

float currentCombustion;        // L/H

float previousCombustion;       // L/H
float totalCombustion;          // L

float currentCombustionNew;     // L/H
float totalCombustionNew;       // L


unsigned long totalInputImpuls;           // impulsy na wejsciu
unsigned long totalOutputImpuls;          // impulsy na powrocie

unsigned long totalInputFuel;           // ml
unsigned long totalOutputFuel;          // ml

unsigned long oldTime;          // ms

float change;
float previousCombustionNew;       // L/H
unsigned long oldTimeNew;          // ms

// Average
boolean average = false;
float averageCombustion;        // L
unsigned long averageStartTime; // ms
float averageTotalCombustion;   // L/H

unsigned long lastClick; // prevent beetwen long pressing 


 
void setup()  
{
  Serial.begin(9600);
  lcd.begin(16,2);   // Inicjalizacja LCD 2x16
  
  
  
  
  //lcd.noBacklight(); // zalaczenie podwietlenia 

  pinMode(inputMeter, INPUT_PULLUP);
  pinMode(outputMeter, INPUT_PULLUP);
  pinMode(RESTART_BUTTON, INPUT_PULLUP);
  pinMode(AVERAGE_BUTTON, INPUT_PULLUP);

  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 4000;            // compare match register 16MHz/256/2Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt

  attachInterrupt( digitalPinToInterrupt(inputMeter), increaseInputImpuls, FALLING);
  attachInterrupt( digitalPinToInterrupt(outputMeter), increaseOutputImpuls, FALLING);
  interrupts();
}


void loop() 
{
  checkButtons();
  if( millis() - oldTime >= 2000 ) {
    checkButtons();
    countFuel();
    countCurrentFuel();
    displayInfo();
    oldTime=millis();
    //debug();
  }
   
   
}

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
 //checkButtons();
}

void checkButtons() {
  if( millis() - lastClick  > 200 ) {
    
      if( digitalRead(RESTART_BUTTON) == LOW ) {
        Serial.print("\nRestart");
        totalInputImpuls=0;
        totalOutputImpuls=0;
        totalCombustion = 0;
        totalCombustionNew = 0;
        displayInfo();
      }
      if( digitalRead(AVERAGE_BUTTON) == LOW ) {
        Serial.print("\nSrednia");
        if( average ) {
          average = false;
        } else {
          average = true;
          averageStartTime = millis();
          averageCombustion = 0;
          averageTotalCombustion = 0;
        }
        displayInfo();
      }
  
   lastClick = millis();
   
  }
}


void debug() {
  Serial.print("\n\nCalkowite impulsy wej: ");
  Serial.print(totalInputImpuls);
  Serial.print("\tCalkowite impulsy wyj: ");
  Serial.print(totalOutputImpuls);
  
  Serial.print("\nCalkowite paliwo wej: ");
  Serial.print(totalInputFuel, 5);
  Serial.print("ml");
  Serial.print("\tCalkowite paliwo wyj: ");
  Serial.print(totalOutputFuel, 5);
  Serial.print("ml");
  
  Serial.print("\n(stare)Calkowite spalanie:   ");
  Serial.print(totalCombustion,5);
  Serial.print("L");
  Serial.print("\t(stare)chwilowe spalanie:    ");
  Serial.print(currentCombustion,5);
  Serial.print("L/h");

  Serial.print("\n(nowe)Calkowite spalanie:    ");
  Serial.print(totalCombustionNew, 5);
  Serial.print("L");
  Serial.print("\t(nowe)chwilowe spalanie:     ");
  Serial.print(currentCombustionNew, 5);
  Serial.print("L/h");
  
}

void displayInfo() {
  lcd.clear();

  if( !average ) {
  lcd.setCursor(4,0); // (kolumna,wiersz)
  lcd.print(currentCombustionNew, 1);
  lcd.print(" l/h");
  lcd.setCursor(0,1); //Ustawienie kursora w pozycji 0,0 (drugi wiersz, pierwsza kolumna)
  lcd.print("Calkowite: ");
  lcd.print(totalCombustionNew, 1);
  } else {
  lcd.setCursor(4,0); // (kolumna,wiersz)
  lcd.print(currentCombustionNew, 1);
  lcd.print(" l/h");
  lcd.setCursor(0,1); //Ustawienie kursora w pozycji 0,0 (drugi wiersz, pierwsza kolumna)
  lcd.print(totalCombustionNew, 1);
  lcd.setCursor(6,1); 
  lcd.print(" | ");
  lcd.print(averageCombustion, 1);
  }
}


void increaseInputImpuls() {
  totalInputImpuls++;
}
void increaseOutputImpuls() {
  totalOutputImpuls++;
}
void countFuel() {
  totalInputFuel = totalInputImpuls * inputAmoutPerImpuls;
  totalOutputFuel = totalOutputImpuls * outputAmoutPerImpuls;
  totalCombustion = (totalInputFuel - totalOutputFuel) / 1000;
  oldTimeNew = (millis()-oldTime);
}
void countCurrentFuel() {
  currentCombustion = ( ( totalCombustion - previousCombustion ) / oldTimeNew  ) * 3600000;
  
  currentCombustionNew = ( ( totalCombustion - previousCombustionNew ) / oldTimeNew  ) * 3600000;

// calkowite bez ujemnych

  change = currentCombustionNew * oldTimeNew / 3600000;
  if( change > 0 ) {
    totalCombustionNew += change;
    currentCombustionNew = currentCombustion;
      if( average ) {
      averageTotalCombustion += change;
  }
  
  } else {
    currentCombustionNew = 0;
  }
  averageCombustion = averageTotalCombustion/ ( millis()-averageStartTime ) * 3600000;
  previousCombustion = totalCombustion;
  previousCombustionNew = totalCombustionNew;
  
  /*
  currentCombustionNew = ( ( totalCombustionNew - previousCombustionNew ) / oldTimeNew  ) * 3600000;
  
  float currentCombustionNew2 = ( ( totalCombustionNew - previousCombustionNew )   ) ;
  Serial.print("NOWE");
  Serial.print(currentCombustionNew2, 6);

  
if ( currentCombustionNew > 0 ) {
    totalCombustionNew += currentCombustionNew * oldTimeNew * 3600000;
    if( average ) {
      averageTotalCombustion += currentCombustionNew;
    }
  } else {
    currentCombustionNew = 0;
  } */
  
  /*
  if ( currentCombustionNew > 0 ) {
    totalCombustionNew += currentCombustionNew * oldTimeNew * 3600000;
    if( average ) {
      averageTotalCombustion += currentCombustionNew;
    }
  } else {
    currentCombustionNew = 0;
  }
  */
  
  

  /*
  change = currentCombustion/3600 * ( millis()-oldTime) * 1000;
  if( change > 0 ) {
    totalCombustionNew += change;
    currentCombustionNew = currentCombustion;
      if( average ) {
      averageTotalCombustion += change;
  }
  } else {
    currentCombustionNew = 0;
  }
  */
  
}
