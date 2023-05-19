#include <Wire.h>
#define Dir  19    // pin DIR de A4988 a pin 5
#define Step  18     // pin STEP de A4988 a pin 4
#define enable 5
#define encoder_pin 34
int iteraciones = 0;
int iteraciones2 = 0;
int rpm_deseado = 100;
int rpm_prom;
unsigned long time_old;
unsigned int pulses_per_turn = 20; // Depends on the number of spokes on the encoder wheel
bool flag = false;
int Encoder = 0;
int Estado = 1;
int Pulsos = 0;
int count = 0;
int PulsosRotacion = 20;
int del = 0;
int new_del = 1400;
unsigned long InitialTime = 0;
unsigned long FinalTime;
float RPMs;
int recibido = 0;
bool reci = false;
int rpm_print = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mover(int new_del){
  
  digitalWrite(Dir, 1);
  digitalWrite(Step, 1);
  delayMicroseconds(new_del);
  digitalWrite(Step, 0);
  delayMicroseconds(new_del); //delayMicroseconds
}

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
    Pulsos++;
    Estado = 1;
  }
  if (Pulsos == PulsosRotacion)
  {
    FinalTime = millis();
    RPMs = 60000 / (FinalTime - InitialTime);
    rpm_prom = rpm_prom + RPMs; 
    Pulsos = 0;
    InitialTime = FinalTime;
    iteraciones2++;
    if (iteraciones2 == 4) {
      rpm_print = rpm_prom/4;
      //Serial.print("RPM = ");
      //Serial.println(rpm_prom/4);
      iteraciones2= 0;
      rpm_prom=0;
      } 
  }
  }
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void corregir(int rpm_deseado){
    if (((rpm_deseado - 1 <= RPMs) || (rpm_deseado + 1 >= RPMs) ) && (iteraciones == 5)) {
    if (rpm_deseado == RPMs){
      new_del = new_del;
      flag = true;
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

////////////////////////////////////////////////////////////////////////////////////////
void requestEvent() {
  uint8_t highByte = ( rpm_print >> 8) & 0xFF;
  uint8_t lowByte = rpm_print & 0xFF;
  Wire.write(highByte);
  Wire.write(lowByte);
  }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Wire.begin(0x23);                // join i2c bus with address #8
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent);
  //Serial.begin(9600);
  //Serial.println ("Inicia");
  pinMode(Step, OUTPUT);  // pin 4 como salida
  pinMode(Dir, OUTPUT); 
  pinMode(encoder_pin, INPUT); // pin 5 como salida
  pinMode(enable, OUTPUT);
  digitalWrite(enable, LOW);
  }


 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
mover(new_del);
medir_rpm(); 
corregir(rpm_deseado); 
}