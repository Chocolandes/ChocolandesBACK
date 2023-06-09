#include <Arduino.h>
#include <Wire.h>
#include "HX711.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <AppREST.h>

// Pines Motobomb
#define canal_a  33
#define canal_b  32
// Pines Bal
#define DOUT  26
#define CLK  27
HX711 balanza(DOUT, CLK);
// Pines Temp
#define tempSensor 13
OneWire oneWireObjeto(tempSensor);
DallasTemperature sensorDS18B20(&oneWireObjeto);

// Variables
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int pasos;
bool inicio = false;
float peso_deseado = 0;
float peso_deseado_anterior = 0;
float peso_inicial = 0;
float peso = 0;
int pwm;
int adicion;
float masa;
float porc1;
float porc2;
float porc3;
float peso_ant;
float pesito;
bool valido;
bool continuar = false;
bool iniciar_mezclar = false;
int rpm_deseado = 0;
int rpm_deseado_anterior = 0;
int del = 0;
bool agregar = false;
float peso_acumulado = 0;
int rpm_recibido = 100;
float temp = 0;
String estado = "Esperando";

// Configuracion para el rest
using namespace AppREST;
bool wireless_mode = true;
float* pesoPointer;
int* rpmPointer;
float* tempPointer;
int* rpmDeseadoPointer;
float* pesoDeseadoPointer;
String* estadoPointer;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void definir_variables(float peso_deseado){
  if(peso_deseado <=50) {
    Serial.println("MENOR A 50");
    pwm = 180;
    adicion = 0;
    porc1 = 0.8;
    porc2 = 0.9;
    porc3 = 0.92;
    }
  else if( (50< peso_deseado) && (peso_deseado <= 100)) {
    Serial.println("ENTRE 50 Y 100");
    pwm = 180;
    adicion = 1;
    porc1 = 0.8;
    porc2 = 0.8;
    porc3 = 0.85;
  }
  else if((100 < peso_deseado)&& (peso_deseado <= 150)) {
    Serial.println("ENTRE 100 Y 200");
    pwm = 180;
    adicion = 1;
    porc1 = 0.8;
    porc2 = 0.8;
    porc3 = 0.85;
  }
  else if((150 < peso_deseado)&& (peso_deseado <= 200)) {
    Serial.println("ENTRE 100 Y 200");
    pwm = 180;
    adicion = 1;
    porc1 = 0.8;
    porc2 = 0.85;
    porc3 = 0.9;
  }
  else if((200 < peso_deseado)&& (peso_deseado <= 300)) {
    Serial.println("ENTRE 200 Y 300");
    pwm = 180;
    adicion = 0;
    porc1 = 0.8;
    porc2 = 0.9;
    porc3 = 0.96;
  }

  else {
    pwm = 180;
    adicion = 0;
    porc1 = 0.8;
    porc2 = 0.9;
    porc3 = 0.91;
    }
  }    

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void preguntar_solvente(){
  continuar = false;
  Serial.println("Introduzca el peso que desea completar con el solvente"); 
  String peso_string = Serial.readString();
  peso_deseado = peso_string.toInt();
  if (peso_deseado <10){
      preguntar_solvente(); }    
  else {
    Serial.print("Peso a completar: ");
    Serial.println(peso_deseado);
    definir_variables(peso_deseado);
    peso_ant = balanza.get_units(4) - peso_inicial;
    agregar = true;
    }
  } 


void echarAgua(){
  continuar = false;
  Serial.print("Peso a completar: ");
  Serial.println(peso_deseado);
  definir_variables(peso_deseado);
  peso_ant = balanza.get_units(4) - peso_inicial;
  agregar = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void agregar_soluto(){
  peso = balanza.get_units(10) - peso_inicial; // Entrega el peso actualment medido en gramos
  //Serial.println(peso);
  String ingreso = Serial.readString();
  Serial.print("Ingreso: ");
  Serial.println(ingreso);
  Serial.print("Masa del soluto: ");
  Serial.println(peso);
  if(ingreso!="")
  {
     preguntar_solvente();
    }
  else {
    agregar_soluto();
    }      
}



  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void preguntar_rpms(){
  Serial.println("Introduzca las RPMs a las que desea mezclar"); 
  String rpm_string = Serial.readString();
  rpm_deseado = rpm_string.toInt();
  if (rpm_deseado <10){
      preguntar_rpms(); }    
  else {
    Serial.print("RPMs deseadas: ");
    Serial.println(rpm_deseado);
    Wire.beginTransmission(0x23); // transmit to device #8
    Wire.write((uint8_t*)&rpm_deseado, sizeof(rpm_deseado)); // Send integer value to slave
    Wire.endTransmission(); // End transmission
    }
  } 

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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void agregar_solvente(float peso, float peso_ant,int pwm, int adicion, float porc1, float porc2, float porc3){
  Serial.println ("Debe agregar el solvente");
  valido = true; 
  if (abs(peso) -abs( peso_ant) >= 20){
        Serial.println("invalido");
        analogWrite(canal_b,0);
        delay(20);
        valido = false;
        }  
  if (valido == true){
    if (peso_deseado*porc1 >= peso)
      //digitalWrite(canal_b,HIGH);
      analogWrite(canal_b,200);
    if (peso_deseado*porc1 < peso <= peso_deseado*porc2)
      //digitalWrite(canal_b,HIGH);
      //digitalWrite(Step, 0);
      analogWrite(canal_b, 150);
    if (peso>= peso_deseado*porc3){
        analogWrite(canal_b,0);
        Serial.println("Mayor, mezclando ");
        agregar = false;
        iniciar_mezclar = true;
        //preguntar_rpms();
    } 
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  //////////////////////////////////////////////////////
  //Configurar lo de aplicacion
  Serial.begin(9600);

  //Apuntadores
  pesoPointer = &peso;
  rpmPointer = &rpm_recibido;
  tempPointer = &temp;
  rpmDeseadoPointer = &rpm_deseado;
  pesoDeseadoPointer = &peso_deseado;
  estadoPointer = &estado;

  //Red wifi
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

  balanza.set_scale(1084.9);
  sensorDS18B20.begin(); 
  
  Serial.println("HOLAAAAAAAAAAAAa");
  pinMode(canal_a,OUTPUT);
  pinMode(canal_b, OUTPUT);
  peso_inicial = balanza.get_units(40); // No se por que carajo el peso estaba llegando negativo pero weno
  Serial.println(peso_inicial);
  Serial.println ("CALIBRADO");
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
    agregar_solvente(peso, peso_ant,pwm,adicion,porc1,porc2,porc3);  
    }

  else{
    estado = "Esperando";
  }

  peso_ant = peso;
  
  if (Serial.available() > 0){
      int comando = Serial.readString().toInt();
      Serial.println(comando);
   
    if (comando == 1){ // Volver a mandar rpm del motor
      preguntar_rpms();
    }
    else if (comando == 2){ //Volver a servir liquido
      agregar==true;
      peso_acumulado = peso;
      //peso_inicial = (balanza.get_units(20)); // Entrega el peso actualment medido en gramos
      preguntar_solvente();
      }
    else if (comando  == 3){ // Volver a tarar
      peso_acumulado = peso;
      peso_inicial = (balanza.get_units(20)); // Entrega el peso actualment medido en gramos
      agregar_soluto();
      }
  }

  if(rpm_deseado_anterior!= rpm_deseado){
    //Serial.print("Cambiar RPMs");
    estado = "Cambiando RPM";
    rpm_deseado_anterior = rpm_deseado;
    cambiar_rpms();
  }

  if(peso_deseado_anterior!=peso_deseado){
    estado = "Cambiando Peso";
    //Serial.print("Echar Agua");
   // Serial.print(peso_deseado);
    peso_deseado_anterior = peso_deseado;
    agregar==true;
    peso_acumulado = peso;
    echarAgua();
  }
}