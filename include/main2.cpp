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
// Esta funcion es para mover el stepper. Se mandan impulsos usando el pin step. El tiempo que transcurre entre impulsos se define con new_del 
// la cual depende del rpm deseado. 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void mover(int new_del){
  
  digitalWrite(Dir, 1);
  digitalWrite(Step, 1);
  delayMicroseconds(new_del);
  digitalWrite(Step, 0);
  delayMicroseconds(new_del); //delayMicroseconds
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funcion para medir el rpm actual. Usa los pines de deteccion del encoder. Como el encoder es optico, los impulsos de entrada indica que
// se avanzo un paso.
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
    Pulsos++; // Si hubo un flanco completo de subida, se incrementa en 1 el numero de pulsos que se han dado
    Estado = 1;
  }
  if (Pulsos == PulsosRotacion) // Si el numero de pulsos es equivalente a una vuelta, se mide cuanto tiempo se tardo en dar la vuelta
  {
    FinalTime = millis(); // Se toma el tiempo final
    RPMs = 60000 / (FinalTime - InitialTime); // Se calculan los rpms
    rpm_prom = rpm_prom + RPMs; 
    Pulsos = 0;
    InitialTime = FinalTime;
    iteraciones2++;
    if (iteraciones2 == 4) { // Se calcula el rpm promedio de las ultimas 4 iteraicones
      rpm_print = rpm_prom/4; // El promedio es el rpm medido que se va a mandar a la otra esp
      iteraciones2= 0;
      rpm_prom=0;
      } 
  }
  }
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funcion para corregir el valor del delay en caso de que no se haya llegado al rpm deseado. Si el rpm es mayor al deseado, se debe aumentar
// el delay, si es menor se debe aumentar. 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void corregir(int rpm_deseado){
    if (((rpm_deseado - 1 <= RPMs) || (rpm_deseado + 1 >= RPMs) ) && (iteraciones == 5)) { // La correccion se hace cada 5 iteraciones para dejar que el sistema se estabilice
    if (rpm_deseado == RPMs){
      new_del = new_del;
      flag = true; // Se llego al deseado, mantener el delay en ese valor
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
// Recibir por i2c el valor de rpm deseado. Este valor se manda desde la esp con el codigo main
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
// Para mandar el valor de rpm actual. La solicitud del dato se hace en el loop del main y esta funcion se ejecuta siempre que se solicite
////////////////////////////////////////////////////////////////////////////////////////
void requestEvent() {
  uint8_t highByte = ( rpm_print >> 8) & 0xFF; // Se manda elvalor actual del rpm
  uint8_t lowByte = rpm_print & 0xFF;
  Wire.write(highByte);
  Wire.write(lowByte);
  }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Wire.begin(0x23);                // Esta es la direccion i2c slave de esta esp
  Wire.onReceive(receiveEvent); // cada vez que reciba un dato
  Wire.onRequest(requestEvent); // Esta funcion se activa cada vez que main le solicite el valor actual del rpm
  //Serial.begin(9600);
  //Serial.println ("Inicia");
  pinMode(Step, OUTPUT);  // Pin para mandar 1 o 0 para hacer los pasos del stepper
  pinMode(Dir, OUTPUT);  // Pin de direccion del stepper
  pinMode(encoder_pin, INPUT); // Pin de deteccion de flancos del encoder
  pinMode(enable, OUTPUT);
  digitalWrite(enable, LOW);
  }


 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
mover(new_del);
medir_rpm(); 
corregir(rpm_deseado); 
}
