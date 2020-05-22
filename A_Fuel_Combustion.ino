#include <Wire.h> // standardowa biblioteka Arduino
#include <EEPROM.h>
#include <Bounce2.h>
#include <Timers.h>
#include <LiquidCrystal_I2C.h> // dolaczenie pobranej biblioteki I2C dla LCD

#define RESTART_BUTTON PD5
#define AVERAGE_BUTTON PD4
#define INPUT_FLOW_METER PD2
#define OUTPUT_FLOW_METER PD3

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Ustawienie adresu ukladu na 0x27
Bounce debouncerRestart = Bounce();
Bounce debouncerAverage = Bounce();
Timers<2> akcja;


float inputAmoutPerImpuls = 0.5336; // ml per each impuls
float outputAmoutPerImpuls = 0.5336; // ml per each impuls
// float inputAmoutPerImpuls  = 0.31;

// Current combustion
float previousCombustion; // L/H
float currentCombustion; // L/H
float totalCombustion; // L

// Average combustion
boolean calculateAverage = false;
float averageCombustion; // L
float averageTotalCombustion; // L/H

// Impuls
unsigned long totalInputImpuls; // impulsy na wejsciu
unsigned long totalOutputImpuls; // impulsy na powrocie
unsigned long averageStartInputImpuls;
unsigned long averageStartOutputImpuls;

// Display
unsigned long totalInputFuel; // ml
unsigned long totalOutputFuel; // ml

// Times
unsigned long lastCalculationTime;
unsigned long averageStartTime;

boolean debugMode = false;

void setup()
{

    // Restore from memory
    totalInputImpuls = EEPROMReadlong(5);
    totalOutputImpuls = EEPROMReadlong(10);

    if (debugMode)
        Serial.begin(9600);
    lcd.begin(16, 2); // Inicjalizacja LCD 2x16

    debouncerRestart.attach(RESTART_BUTTON, INPUT_PULLUP);
    debouncerRestart.interval(25);

    debouncerAverage.attach(AVERAGE_BUTTON, INPUT_PULLUP);
    debouncerAverage.interval(25);

    pinMode(INPUT_FLOW_METER, INPUT_PULLUP);
    pinMode(OUTPUT_FLOW_METER, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(INPUT_FLOW_METER), increaseInputImpuls, FALLING);
    attachInterrupt(digitalPinToInterrupt(OUTPUT_FLOW_METER), increaseOutputImpuls, FALLING);
    akcja.attach(0, 1300, calculate);
    akcja.attach(1, 100, checkButtons);
}

void loop() {
  akcja.process();
  }

void calculate()
{
    //checkButtons();
    countFuel();
    countCurrentFuel();
    displayInfo();
    if (debugMode)
        debug();
    // save to memeory
    EEPROMWritelong(5, totalInputImpuls);
    EEPROMWritelong(10, totalOutputImpuls);
}
void checkButtons()
{
    debouncerRestart.update(); // Update the Bounce instance
    debouncerAverage.update(); // Update the Bounce instance

    if (debouncerRestart.fell()) { // Call code if button transitions from HIGH to LOW
        if (debugMode)
            Serial.print("\nRestart");
        totalInputImpuls = 0;
        totalOutputImpuls = 0;
		averageCombustion = 0;
        averageTotalCombustion = 0;
        displayInfo();
    }
    if (debouncerAverage.fell()) { // Call code if button transitions from HIGH to LOW
        if (debugMode)
            Serial.print("\nSrednia");
        if (calculateAverage) {
            calculateAverage = false;
        }
        else {
            calculateAverage = true;
            averageStartTime = millis();
            averageStartInputImpuls = totalInputImpuls;
            averageStartOutputImpuls = totalOutputImpuls;
            averageCombustion = 0;
            averageTotalCombustion = 0;
        }
        displayInfo();
    }
}

void debug()
{
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
    Serial.print(totalCombustion, 5);
    Serial.print("L");
    Serial.print("\t(stare)chwilowe spalanie:    ");
    Serial.print(currentCombustion, 5);
    Serial.print("L/h");

    Serial.print("\n(nowe)Calkowite spalanie:    ");
    Serial.print(totalCombustion, 5);
    Serial.print("L");
    Serial.print("\t(nowe)chwilowe spalanie:     ");
    Serial.print(currentCombustion, 5);
    Serial.print("L/h");
}

void displayInfo()
{
    lcd.clear();

    if (!calculateAverage) {
        lcd.setCursor(4, 0); // (kolumna,wiersz)
        lcd.print(currentCombustion, 1);
        lcd.print(" l/h");
        lcd.setCursor(0, 1); //Ustawienie kursora w pozycji 0,0 (drugi wiersz, pierwsza kolumna)
        lcd.print("Calkowite: ");
        lcd.print(totalCombustion, 1);
    }
    else {
        lcd.setCursor(4, 0); // (kolumna,wiersz)
        lcd.print(currentCombustion, 1);
        lcd.print(" l/h");
        lcd.setCursor(0, 1); //Ustawienie kursora w pozycji 0,0 (drugi wiersz, pierwsza kolumna)
        lcd.print(totalCombustion, 1);
        lcd.setCursor(6, 1);
        lcd.print(" | ");
        lcd.print(averageCombustion, 1);
    }
}

void increaseInputImpuls()
{
    totalInputImpuls++;
}
void increaseOutputImpuls()
{
    totalOutputImpuls++;
}
void countFuel()
{
    totalInputFuel = totalInputImpuls * inputAmoutPerImpuls;
    totalOutputFuel = totalOutputImpuls * outputAmoutPerImpuls;
    totalCombustion = (float)(totalInputFuel - totalOutputFuel) / 1000.0;
    if(calculateAverage)
    {
      averageTotalCombustion = (float)( ((totalInputImpuls-averageStartInputImpuls)*inputAmoutPerImpuls)- ((totalOutputImpuls-averageStartOutputImpuls)*outputAmoutPerImpuls)) / 1000.0;
    }
}
void countCurrentFuel()
{
    currentCombustion = ((totalCombustion - previousCombustion) / (millis() - lastCalculationTime)) * 3600000;
    previousCombustion = totalCombustion;
    if(calculateAverage)
    {
      averageCombustion = averageTotalCombustion / (millis() - averageStartTime) * 3600000;
    }
    lastCalculationTime = millis();
}

void EEPROMWritelong(int address, long value)
{
    //Decomposition from a long to 4 bytes by using bitshift.
    //One = Most significant -> Four = Least significant byte
    byte four = (value & 0xFF);
    byte three = ((value >> 8) & 0xFF);
    byte two = ((value >> 16) & 0xFF);
    byte one = ((value >> 24) & 0xFF);

    //Write the 4 bytes into the eeprom memory.
    EEPROM.write(address, four);
    EEPROM.write(address + 1, three);
    EEPROM.write(address + 2, two);
    EEPROM.write(address + 3, one);
}

long EEPROMReadlong(long address)
{
    //Read the 4 bytes from the eeprom memory.
    long four = EEPROM.read(address);
    long three = EEPROM.read(address + 1);
    long two = EEPROM.read(address + 2);
    long one = EEPROM.read(address + 3);

    //Return the recomposed long by using bitshift.
    return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
