#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <AppREST.h>

using namespace AppREST;

bool wireless_mode = true;
int* valorPointer;
int valor = 0;


void setup() {


  Serial.begin(9600);
  valorPointer = &valor; //El pointer apunta al valor

  if (wireless_mode){
    //Set la ip
    IPAddress local_ip(192,168,1,1);
    IPAddress gateway(192,168,1,1);
    IPAddress subnet(255,255,255,0);

    WiFi.softAP("wifi", "12345678");
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address");
    Serial.println(IP);


    //Set webserver
    _serverPointer = new WebServer(80); 
    _serverPointer->enableCORS();

    //Vincular datos
    AppREST::linkServer(_serverPointer);
    AppREST::linkValor(valorPointer);

    //Configurar GET
    _serverPointer->on("/valor", HTTP_GET, AppREST::GETValor);

    //Configurar PUT
    _serverPointer->on("/valorput", HTTP_PUT, AppREST::PUTValor);

    //Inicializar
    _serverPointer->begin();
  }  
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() { 
  
  _serverPointer->handleClient();

  valor = valor + 1;

  Serial.println(*valorPointer);
  delay(2000);

}