#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <EEPROM.h>
#include "HX711.h"

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2); 
HX711 scale;

const int pinButon1 = 2; // PD2 / INT0 (Start/Stop/Pauza)
const int pinButon2 = 3; // PD3 / INT1 (Schimbare Afisaj)
const int pinReleu = A1; // Pompa de apa
const int pinDT = A2;    // HX711 Data
const int pinSCK = A3;   // HX711 Clock

const int piniLed[] = {0, 1, 4, 5, 6, 7, 8, 9, 10, 11};
const int numarLeduri = 10;

const float FACTOR_CALIBRARE = 1100.0;
const float TINTA_APA_ML = 250.0; 

float greutateCurenta = 0.0;
float greutateStart = 0.0;
float apaTurnataCurent = 0.0;

const int ADRESA_ZIUA = 0; 
const int ADRESA_APA = 5;  

bool pompaPornita = false;
unsigned long totalApaZiMl = 0;      
int ziuaAnterioara = -1;             

int modAfisaj = 0;                   
unsigned long ultimulRefreshLCD = 0; 

volatile bool flagBtn1 = false;
volatile bool flagBtn2 = false;
volatile unsigned long ultimulTimpB1 = 0;
volatile unsigned long ultimulTimpB2 = 0;
const unsigned long timpDebounce = 200;
void ISR_Btn1() {
  unsigned long timp = millis();
  if (timp - ultimulTimpB1 > timpDebounce) {
    flagBtn1 = true;
    ultimulTimpB1 = timp;
  }
}

void ISR_Btn2() {
  unsigned long timp = millis();
  if (timp - ultimulTimpB2 > timpDebounce) {
    flagBtn2 = true;
    ultimulTimpB2 = timp;
  }
}

void stingeToateLedurile() {
  for (int i = 0; i < numarLeduri; i++) {
    digitalWrite(piniLed[i], LOW);
  }
}

void setup() {
  pinMode(pinButon1, INPUT_PULLUP);
  pinMode(pinButon2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinButon1), ISR_Btn1, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinButon2), ISR_Btn2, FALLING);
  pinMode(pinReleu, OUTPUT);
  digitalWrite(pinReleu, LOW);

  for (int i = 0; i < numarLeduri; i++) {
    pinMode(piniLed[i], OUTPUT);
    digitalWrite(piniLed[i], LOW);
  }

  Wire.begin();
  rtc.begin();
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("Pornire cantar..");
  scale.begin(pinDT, pinSCK);
  scale.set_scale(FACTOR_CALIBRARE);
  scale.tare(); 
  delay(1000);
  lcd.clear();
  
  DateTime now = rtc.now();
  
  EEPROM.get(ADRESA_ZIUA, ziuaAnterioara);
  EEPROM.get(ADRESA_APA, totalApaZiMl);

  if (ziuaAnterioara < 1 || ziuaAnterioara > 31) {
    ziuaAnterioara = now.day();
    totalApaZiMl = 0;
    EEPROM.put(ADRESA_ZIUA, ziuaAnterioara);
    EEPROM.put(ADRESA_APA, totalApaZiMl);
  }
  
  if (now.day() != ziuaAnterioara) {
    totalApaZiMl = 0;
    ziuaAnterioara = now.day();
    EEPROM.put(ADRESA_ZIUA, ziuaAnterioara);
    EEPROM.put(ADRESA_APA, totalApaZiMl);
  }
}

void loop() {
  DateTime now = rtc.now();

  if (now.day() != ziuaAnterioara) {
    totalApaZiMl = 0;
    ziuaAnterioara = now.day();
    EEPROM.put(ADRESA_ZIUA, ziuaAnterioara);
    EEPROM.put(ADRESA_APA, totalApaZiMl);
  }

  if (scale.is_ready()) {
    greutateCurenta = scale.get_units(1);
    if (greutateCurenta < 0) greutateCurenta = 0; 
  }

  if (flagBtn1) {
    flagBtn1 = false;
    
    if (pompaPornita == false) {
      pompaPornita = true;
      digitalWrite(pinReleu, HIGH);
      
      if (apaTurnataCurent == 0) {
        greutateStart = greutateCurenta;
      }
      lcd.clear();
    } else {
      pompaPornita = false;
      digitalWrite(pinReleu, LOW); 
      lcd.clear();
    }
  }
  if (flagBtn2) {
    flagBtn2 = false;
    modAfisaj = !modAfisaj;
    lcd.clear();
  }
  if (pompaPornita == true) {
    apaTurnataCurent = greutateCurenta - greutateStart;
    if (apaTurnataCurent < 0) apaTurnataCurent = 0;

    if (apaTurnataCurent >= TINTA_APA_ML) {
      pompaPornita = false;
      digitalWrite(pinReleu, LOW); 
      
      totalApaZiMl += (unsigned long)TINTA_APA_ML; 
      EEPROM.put(ADRESA_APA, totalApaZiMl);
      
      apaTurnataCurent = 0;
      stingeToateLedurile();
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  Sticla Plina! ");
      lcd.setCursor(0, 1);
      lcd.print("   ( 250 ML )   ");
      delay(3000); 
      lcd.clear();
    }
  }

  if (apaTurnataCurent > 0 && pompaPornita) {
    int leduriDeAprins = (apaTurnataCurent * numarLeduri) / TINTA_APA_ML;
    for (int i = 0; i < numarLeduri; i++) {
      if (i < leduriDeAprins) {
        digitalWrite(piniLed[i], HIGH);
      } else {
        digitalWrite(piniLed[i], LOW);
      }
    }
  }

  if (millis() - ultimulRefreshLCD > 500) {
    ultimulRefreshLCD = millis();

    if (modAfisaj == 0) {
      
      if (pompaPornita == false && apaTurnataCurent == 0) {
        lcd.setCursor(0, 0);
        lcd.print("Pune sticla...  ");
        lcd.setCursor(0, 1);
        char rand1[17];
        sprintf(rand1, "Masa: %4d g   ", (int)greutateCurenta);
        lcd.print(rand1);
      } 
      else {
        lcd.setCursor(0, 0);
        lcd.print("Tinta: 250 ml   ");

        lcd.setCursor(0, 1);
        char rand1[17];
        if (pompaPornita) {
          sprintf(rand1, "Turnat: %3d ml ", (int)apaTurnataCurent);
        } else {
          sprintf(rand1, "PAUZA:  %3d ml ", (int)apaTurnataCurent);
        }
        lcd.print(rand1);
      }

    } else {
      lcd.setCursor(0, 0);
      char rand0[17];
      sprintf(rand0, "%02d:%02d %02d/%02d    ", now.hour(), now.minute(), now.day(), now.month());
      lcd.print(rand0);

      lcd.setCursor(0, 1);
      char rand1[17];
      unsigned long totalDisplay = totalApaZiMl + (unsigned long)apaTurnataCurent;
      sprintf(rand1, "Baut azi:%4lu ml", totalDisplay);
      lcd.print(rand1);
    }
  }
}