#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// JSON data buffer
StaticJsonDocument<1000> jsonDocument;
char buffer[1000];

//Funciones de manejo JSON Auxiliares
void create_json(float temp, int rpm, float peso, String estado) {  
  jsonDocument.clear();
  jsonDocument["temp"] = temp;
  jsonDocument["rpm"] = rpm;
  jsonDocument["peso"] = peso;
  jsonDocument["estado"] = estado;
  serializeJson(jsonDocument, buffer);  
}

namespace AppREST{

  WebServer* _serverPointer;
  int* _valorPointer;
  float* _valorPeso;
  int* _valorRPM;
  float* _valorTemp;
  int* _valorRMPDeseado;
  float* _valorPesoDeseado;
  String* _valorEstado;

  void linkServer(WebServer* serverPointer){
      _serverPointer = serverPointer;
  }

  //MÉTODOS GET

  void GETValores(){
    Serial.println("GET valor");
    create_json((*_valorTemp), (*_valorRPM), (*_valorPeso), (*_valorEstado));
    (*_serverPointer).send(200, "application/json", buffer);
  }

  //MÉTODOS PUT
  void PUTrpmDeseado(){
    Serial.println("PUT rpm Deseado");
    if ((*_serverPointer).hasArg("plain") == false){
      Serial.print("Esperaba un pepe, recibi nada");
    }
    String body = (*_serverPointer).arg("plain");
    deserializeJson(jsonDocument,body);
    (*_valorRMPDeseado) = (int) jsonDocument["rpm"];
    GETValores();
  }


  void PUTPesoDeseado(){
    Serial.println("PUT peso Deseado");
    if ((*_serverPointer).hasArg("plain") == false){
      Serial.print("Esperaba un pepe, recibi nada");
    }
    String body = (*_serverPointer).arg("plain");
    deserializeJson(jsonDocument,body);
    (*_valorPesoDeseado) = (int) jsonDocument["peso"];
    GETValores(); 
  }
  

  //METODOS INK
  void linkPeso(float* valorPeso){
    _valorPeso = valorPeso;
  }

  void linkRPM(int* valorRPM){
    _valorRPM = valorRPM;
  }

  void linkTemp(float* valorTemp){
    _valorTemp = valorTemp;
  }

  void linkRPMDesado(int* valorRMPDeseado){
    _valorRMPDeseado = valorRMPDeseado;
  }

  void linkPesoDeseado(float* valorPesoDeseado){
    _valorPesoDeseado = valorPesoDeseado;
  }

  void linkEstado(String* valorEstado){
    _valorEstado = valorEstado;
  }
}


