#include <Wire.h>
#define Dir  19    // pin DIR de A4988 a pin 5
#define Step  18     // pin STEP de A4988 a pin 4
#define enable 5    // pin ENABLE de A4988 a pin 3
#define encoder_pin 34  // pin for edge detection
int iteraciones = 0; //num of iterations for correction
int iteraciones2 = 0; //num of iterations for rpm average
int rpm_deseado = 100; // rpm desired
int rpm_prom; // rpm average
unsigned long time_old; // time for rpm
unsigned int pulses_per_turn = 20; // Depends on the number of spokes on the encoder wheel
bool flag = false; // flag for correction
int Encoder = 0; // first variable used to save the state of the encoder pin
int Estado = 1; // second variable used to save the state of the encoder pin
int Pulsos = 0; // Variable used to save the number of pulses
int PulsosRotacion = 20; // Number of pulses per rotation
int del = 0; // delay for stepper
int new_del = 1400; // new delay for stepper
unsigned long InitialTime = 0; // Variable used to save the initial time
unsigned long FinalTime; // Variable used to save the final time
float RPMs; // Variable used to save the RPMs
int recibido = 0; // Data received by I2C
bool reci = false; // If data was received by I2C
int rpm_print = 0; // rpm to print
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function is for moving the stepper. Pulses are sent using the step pin. The time between pulses is defined by new_del
// which depends on the desired RPM.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void mover(int new_del){
  
  digitalWrite(Dir, 1);
  digitalWrite(Step, 1);
  delayMicroseconds(new_del);
  digitalWrite(Step, 0);
  delayMicroseconds(new_del); //delayMicroseconds
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to measure the current RPM. It uses the encoder detection pins. Since the encoder is optical, the incoming pulses indicate that
// a step has been taken..
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void medir_rpm(){
  Encoder = digitalRead(encoder_pin);
  if (iteraciones2 == 0)
  {
    rpm_prom = 0;
  }
  if (Encoder == LOW && Estado == 1)
  {
    Estado = 0;
  }
  if (Encoder == HIGH && Estado == 0)
  {
    Pulsos++; // If there was a complete rising edge, increment the pulse count by 1
    Estado = 1;
  }
  if (Pulsos == PulsosRotacion) // If the number of pulses is equivalent to one rotation, measure the time it took to complete the rotation
  {
    FinalTime = millis(); // Take the final time
    RPMs = 60000 / (FinalTime - InitialTime); // Calculate the RPMs
    rpm_prom = rpm_prom + RPMs; 
    Pulsos = 0;
    InitialTime = FinalTime;
    iteraciones2++;
    if (iteraciones2 == 4) { // Calculate the average RPM of the last 4 iterations
      rpm_print = rpm_prom/4; // The average is the measured RPM to be sent to the other esp device
      iteraciones2= 0;
      rpm_prom=0;
      } 
  }
  }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to correct the value of the delay in case the desired RPM has not been reached. If the RPM is higher than the desired value,
// the delay should be increased; if it is lower, the delay should be decreased.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void corregir(int rpm_deseado){
    if (((rpm_deseado - 1 <= RPMs) || (rpm_deseado + 1 >= RPMs) ) && (iteraciones == 5)) { // The correction is done every 5 iterations to allow the system to stabilize.
    if (rpm_deseado == RPMs){
      new_del = new_del;
      flag = true; // If the desired RPM has been reached, keep the delay at that value.
      }   
    else if ((rpm_deseado > RPMs)&& (flag == false)) {
      new_del = new_del-1;
    }
    else if ((rpm_deseado < RPMs) && (flag == false)) {
      new_del = new_del+1;
    }
    iteraciones = 0;
  }
  iteraciones ++;
  }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Receive the desired RPM value via I2C. This value is sent from the main code on the other device (ESP)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void receiveEvent(int numBytes) {
  int value = 0; // Variable to hold incoming integer value

  while (Wire.available() > 0) {
    Wire.readBytes((uint8_t*)&recibido, sizeof(rpm_deseado)); // Read incoming integer value
    if(recibido>0){
      rpm_deseado = recibido;
      flag = false;
      //Serial.print("Recibido: ");
      //Serial.println(rpm_deseado);
      }
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// To send the current RPM value, the request for the data is made in the main loop, and this function is executed whenever it is requested.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void requestEvent() {
  uint8_t highByte = ( rpm_print >> 8) & 0xFF; // Se manda elvalor actual del rpm
  uint8_t lowByte = rpm_print & 0xFF;
  Wire.write(highByte);
  Wire.write(lowByte);
  }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Wire.begin(0x23);                // This is the I2C slave address of this device
  Wire.onReceive(receiveEvent); // Triggered whenever data is received
  Wire.onRequest(requestEvent); // This function is activated whenever the main code requests 
  //Serial.begin(9600);
  //Serial.println ("Inicia");
  pinMode(Step, OUTPUT);  // Pin for sending 1 or 0 to perform stepper motor steps
  pinMode(Dir, OUTPUT);  // Stepper motor direction pin
  pinMode(encoder_pin, INPUT); // Encoder edge detection pin
  pinMode(enable, OUTPUT);
  digitalWrite(enable, LOW);
  }


 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Loop of functios that are executed continuously
void loop() {
mover(new_del);
medir_rpm(); 
corregir(rpm_deseado); 
}
