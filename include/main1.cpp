// Librerias
#include <Arduino.h>  // This library is just needed if you run the code using Platformio
#include <Wire.h>  // Library for the i2c communication protocol
#include "HX711.h"  // HX711 Module
#include <OneWire.h> // Communication protocol used in the temperature sensor
#include <DallasTemperature.h> 
#include <WiFi.h> // For the app
#include <WiFiClient.h> 
#include <WebServer.h>
#include <AppREST.h>

// Motor pump control pins
#define canal_b  32
// Scale control pins
#define DOUT  26
#define CLK  27
HX711 balanza(DOUT, CLK);
// Temperature sensor pin
#define tempSensor 13
OneWire oneWireObjeto(tempSensor);
DallasTemperature sensorDS18B20(&oneWireObjeto);

// Variables are all global and may change in the functions and depending on the state of the machine
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float peso_deseado = 0;
float peso_deseado_anterior = 0;
float peso_inicial = 0;
float peso = 0;
float porc1;
float porc2;
float porc3;
float peso_ant;
bool valido;
bool continuar = false;
int rpm_deseado = 0;
int rpm_deseado_anterior = 0;
bool agregar = false;
float peso_acumulado = 0;
int rpm_recibido = 100;
float temp = 0;
String estado = "Esperando";

// App configuration
using namespace AppREST;
bool wireless_mode = true;
float* pesoPointer;
int* rpmPointer;
float* tempPointer;
int* rpmDeseadoPointer;
float* pesoDeseadoPointer;
String* estadoPointer;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// This function defines the mass percentages at which the motor pump should change its pwm or stop serving. For example, if the mass requiered is 
// less than 50g, when the mass reaches 90% of this value the pwm input of the pump will decrease to 150, and when it reaches the 92% it will turn off
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void definir_variables(float peso_deseado){
  if(peso_deseado <=50) {
    porc1 = 0.8;
    porc2 = 0.9;
    porc3 = 0.92;
    }
  else if( (50< peso_deseado) && (peso_deseado <= 100)) {
    porc1 = 0.8;
    porc2 = 0.8;
    porc3 = 0.85;
  }
  else if((100 < peso_deseado)&& (peso_deseado <= 150)) {
    porc1 = 0.8;
    porc2 = 0.8;
    porc3 = 0.85;
  }
  else if((150 < peso_deseado)&& (peso_deseado <= 200)) {
    porc1 = 0.8;
    porc2 = 0.85;
    porc3 = 0.9;
  }
  else if((200 < peso_deseado)&& (peso_deseado <= 300)) {
    porc1 = 0.8;
    porc2 = 0.9;
    porc3 = 0.96;
  }

  else {
    porc1 = 0.8;
    porc2 = 0.9;
    porc3 = 0.91;
    }
  }    

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// This function is used to ask the user the mass to complete, to access this function you must send a 2 in the serial terminal or input mass 
// in the app. 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void preguntar_solvente(){
  continuar = false;
  Serial.println("Introduzca el peso que desea completar con el solvente"); 
  String peso_string = Serial.readString();
  peso_deseado = peso_string.toInt();
  if (peso_deseado <10){ // The user must input more than 10 grams in order to have a precise outcome
      preguntar_solvente(); }    
  else {
    Serial.print("Peso a completar: ");
    Serial.println(peso_deseado);
    definir_variables(peso_deseado); // The function previously mentioned to define the global values of the percentages
    peso_ant = balanza.get_units(4) - peso_inicial;
    agregar = true;
    }
  } 


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// This function updates the percentages values of the pouring function if the mass has not been reached
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

void echarAgua(){
  continuar = false;
  Serial.print("Peso a completar: ");
  Serial.println(peso_deseado);
  definir_variables(peso_deseado);
  peso_ant = balanza.get_units(4) - peso_inicial;
  agregar = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// This function is used in case the user wants to define a new zero. WHen the function is called, the scale tares itself again.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void agregar_soluto(){
  peso = balanza.get_units(10) - peso_inicial; // Entrega el peso actualment medido en gramos
  String ingreso = Serial.readString(); // Reads the command inserted in the loop
  Serial.print("Ingreso: ");
  Serial.println(ingreso);
  Serial.print("Masa del soluto: ");
  Serial.println(peso); // Prints the actual mass after taring the scale
  if(ingreso!="")
  {
     preguntar_solvente();
    }
  else {
    agregar_soluto();
    }      
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// Function called to changes the mixing speed. The main esp sends the desired mixing speed to the other esp via i2c
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void preguntar_rpms(){
  Serial.println("Introduzca las RPMs a las que desea mezclar"); 
  String rpm_string = Serial.readString();
  rpm_deseado = rpm_string.toInt();
  if (rpm_deseado <20){ //Mixing speed must be higher than 20 rpm
      preguntar_rpms(); }    
  else {
    Serial.print("RPMs deseadas: ");
    Serial.println(rpm_deseado);
    Wire.beginTransmission(0x23); // transmit to device #8
    Wire.write((uint8_t*)&rpm_deseado, sizeof(rpm_deseado)); // Send integer value to slave
    Wire.endTransmission(); // End transmission
    }
  } 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// THis function is called in case the rpm value recieved in the previous function has not been reached. The system keeps sending the desired
// value until it is exactly the one that the user asked for
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

void cambiar_rpms(){
  if (rpm_deseado <10){
      Serial.print("ELejir otro valor"); 
  }    
  else {
    Serial.print("RPMs deseadas: ");
    Serial.println(rpm_deseado);
    Wire.beginTransmission(0x23); // transmit to device #8
    Wire.write((uint8_t*)&rpm_deseado, sizeof(rpm_deseado)); // Send integer value to slave
    Wire.endTransmission(); // End transmission
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// Function to pour the water, the function recieves the values of the percentages defined before and the current and previous values of the weight
// This function has an special consideration: the noise of the mesurement. In some occassions, when the motor pump is turned on the weight measured
//  by the scale takes negative or non reasonable values. For this reason, we decided to define those measurments as "not valid" and stop the motor pump.
// Once the values are recieved well again, the motor pump continues pouring the liquid. 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void agregar_solvente(float peso, float peso_ant,float porc1, float porc2, float porc3){
  Serial.println ("Debe agregar el solvente");
  valido = true; // Control value to check if the value is not valid
  if (abs(peso) -abs( peso_ant) >= 20){  // In case there is noise in the measurement
        Serial.println("invalido");
        analogWrite(canal_b,0); // the motor pump is turned off
        delay(20);
        valido = false;
        }  
  if (valido == true){
    if (peso_deseado*porc1 >= peso)
      analogWrite(canal_b,200);
    if (peso_deseado*porc1 < peso <= peso_deseado*porc2)
      analogWrite(canal_b, 150);
    if (peso>= peso_deseado*porc3){ // Stop if the mass percentage 3 is reached
        analogWrite(canal_b,0);
        agregar = false; // Boolean value to stop function in loop
            } 
  }
}


void setup() {
  /////////////////////////////////////////////////////
  Serial.begin(9600);
  //Apuntadores
  pesoPointer = &peso;
  rpmPointer = &rpm_recibido;
  tempPointer = &temp;
  rpmDeseadoPointer = &rpm_deseado;
  pesoDeseadoPointer = &peso_deseado;
  estadoPointer = &estado;

  //Wifi network
   if (wireless_mode){
    //Set la ip
    //IPAddress local_ip(192,168,1,1);
    //IPAddress gateway(192,168,1,1);
    //IPAddress subnet(255,255,255,0);
//
    //WiFi.softAP("wifi", "123456789");
    //WiFi.softAPConfig(local_ip, gateway, subnet);
    //delay(100);
//
    //IPAddress IP = WiFi.softAPIP();
    //Serial.print("AP IP address");
    //Serial.println(IP);

    //connect to WIFI
    
    WiFi.mode(WIFI_STA);
    WiFi.begin("robocol_router","12345678");
    
    while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); 

    //Set webserver
    _serverPointer = new WebServer(80); 
    _serverPointer->enableCORS();

    //Vincular datos
    AppREST::linkServer(_serverPointer);
    AppREST::linkPeso(pesoPointer);
    AppREST::linkRPM(rpmPointer);
    AppREST::linkTemp(tempPointer);
    AppREST::linkRPMDesado(rpmDeseadoPointer);
    AppREST::linkPesoDeseado(pesoDeseadoPointer);
    AppREST::linkEstado(estadoPointer);

    //Configurar GET
    _serverPointer->on("/valores", HTTP_GET, AppREST::GETValores);
    _serverPointer->on("/rpmDeseado", HTTP_PUT, AppREST::PUTrpmDeseado);
    _serverPointer->on("/pesoDeseado", HTTP_PUT, AppREST::PUTPesoDeseado);

    //Inicializar
    _serverPointer->begin();
  }


  ////////////////////////////////////////////////////////

  balanza.set_scale(1084.9); // Tare the scale when the code is initialized
  sensorDS18B20.begin(); // Initialize the temperature sensor
  
  pinMode(canal_b, OUTPUT); // Motorpump pin as output
  peso_inicial = balanza.get_units(40); // Measure the inicial value of the scale after being tared
  Serial.println(peso_inicial);
  Serial.println ("CALIBRADO"); // Confirm the scale is ready
  Wire.begin(); // join i2c bus (address optional for master)

 }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() { 
  //SErvidor de REST
  _serverPointer->handleClient();


  peso = balanza.get_units(4) - peso_inicial; // Entrega el peso actualment medido en gramos
  //Serial.print("Masa actual: "); 
  //Serial.println(peso); 

  //Para obtener RPM del segundo micro
  Wire.requestFrom(0X23, 2);//Direccion, bytes
  uint8_t highByte = Wire.read();
  uint8_t lowByte = Wire.read();
  rpm_recibido = (highByte << 8) | lowByte;
  //Serial.print("RPM actual: ");
  //Serial.println(rpm_recibido);

  //Recibir valor temperatura
  sensorDS18B20.requestTemperatures();
  temp = sensorDS18B20.getTempCByIndex(0) + 1;
  //Serial.print("Temperatura actual: ");
  //Serial.println(temp, 1);

  //Serial.println("--------------");
  //Serial.println(agregar);

  if (agregar == true){    
    agregar_solvente(peso, peso_ant, porc1,porc2,porc3);  // Pour liquid 
    }

  else{
    estado = "Esperando";
  }

  peso_ant = peso; // Present value of the weight is stored as previous for the agregar_solvente function
  
  if (Serial.available() > 0){ // Read the commands recieved from serial
      int comando = Serial.readString().toInt();
      Serial.println(comando);
   
    if (comando == 1){ // Change rpm value
      preguntar_rpms(); // Call rpm change function
    }
    else if (comando == 2){ // Pour liquid
      agregar==true; // Boolean to control loop of agregar_solvente as true
      peso_acumulado = peso;
      preguntar_solvente();
      }
    else if (comando  == 3){ // Tare the scale again and define a new zero
      peso_acumulado = peso;
      peso_inicial = (balanza.get_units(20)); // Entrega el peso actualment medido en gramos
      agregar_soluto();
      }
  }

  if(rpm_deseado_anterior!= rpm_deseado){ // Keep correcting the rpm if the desired value has not been reached
    //Serial.print("Cambiar RPMs");
    estado = "Cambiando RPM";
    rpm_deseado_anterior = rpm_deseado;
    cambiar_rpms();
  }

  if(peso_deseado_anterior!=peso_deseado){ // Keep adding water if the mass has not been reached 
    estado = "Cambiando Peso";
    peso_deseado_anterior = peso_deseado;
    agregar==true;
    peso_acumulado = peso;
    echarAgua(); // 
  }
}
