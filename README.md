# ChocolandesBACK
In this repository you can find the codes that allow you to build the application  that controls and are suposed to be uploaded in the microcontrollers of an electronic device that has the ability to measure mass, dispense a liquid, mix the liquid with a stepper motor and measure the temperature of the liquid, all synchronously and continous.
## Authors:
* Guerrero Rios Sebastian 
* Guayacan Mesa Sebastian 
* Ramirez Cepeda Yaisa Catalina
* Reyes Corredor Juan Camilo

## Material's list:
### In this project we use two ESP32 microcontrollers. One is dedicated to the stepper motor movement (ESP-MOT) and the other is in charge of all the other functionalities (ESP-MAIN).
### L293D
Dual DC motor driver 

* In1: D33 ESP-MAIN
* In2: D32 ESP-MAIN
* Out1: Red wire motor pump
* Out2: Black wire motor pump
* En1: 5V
* VCC: 5V
* Vss: 12V
* Pin 4 y 5: GND

### HX711
Load cell ADC converter

* E+: Red
* E-: Black
* A+: Withe
* A-: Green
* Dt: D26 ESP-MAIN
* Sck: D27 ESP-MAIN
* Vcc: 5V

### A4988
Stepper motor driver 
* Dir: D19  ESP-MOT
* Step: D18  ESP-MOT
* Enable: D5  ESP-MOT
* Vdd: 5V
* Reset & Sleep Are connected between them
* 2B: Black 
* 2A: Green
* 1A: Blue
* 1B: Red
* Vmot: External power (12V)

### NEMA 17HS4401s
Stepper motor with 1.8° resolution
* Black: 2B
* Green: 2A
* Blue: 1A
* Red: 1B
### HC020P
* Vdd: 5V
* Data: D34 ESP-MOT
### DS18B20
* Vdd: 5V
* Data: D13 ESP-MAIN
### Motor Pump
* Red: Out1 L293D
* Black: Out2 L293D
### Load Cell
* Red: E+
* Black: E-
* Withe: A+
* Green: A-

### ESP-MAIN
* D33: In1 L293D
* D32: In2 L293D
* D26: Dt HX711
* D27: Sck HX711
* D13: Data DS18B20
### ESP-MOT
* D5: Enable A4988
* D19: DIR A4988
* D18: STEP A4988
* D34: Data HC020P

## Repository's codes:
* /include/main1.cpp:
  Code that runs on the ESP-MAIN.  It measures mass, dispenses water, measures temperature, communicates via I2C with the ESP-MOT to move and measure the speed of the stepper motor, and communicates via Wi-Fi with the device's application.


* /include/main2.cpp:
  Code that runs on the ESP-MOT. It communicates via I2C with the ESP-MAIN to receive the instructions that move the  stepper motor.

  
