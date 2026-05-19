#include "HX711.h"

// pini HX711
const int LOADCELL_DOUT_PIN = A2;
const int LOADCELL_SCK_PIN = A3;

HX711 scale;

float calibration_factor = 100.0; 

void setup() {
  Serial.begin(9600);
  Serial.println("Asteapta");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  scale.set_scale(calibration_factor);
  scale.tare(); 

  Serial.println("Pune sticla");
}

void loop() {
  scale.set_scale(calibration_factor);

  Serial.print("Greutate citita: ");
  Serial.print(scale.get_units(10), 1); 
  Serial.print(" grame");
  Serial.print("  |  Factor curent: ");
  Serial.println(calibration_factor);

  if (Serial.available()) {
    char temp = Serial.read();
    if (temp == '+' || temp == 'a') {
      calibration_factor += 10;
    } else if (temp == '-' || temp == 'z') {
      calibration_factor -= 10;
    } else if (temp == 'A') {
      calibration_factor += 100;
    } else if (temp == 'Z') {
      calibration_factor -= 100;
    }
  }
}