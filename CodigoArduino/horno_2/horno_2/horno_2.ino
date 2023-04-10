#include <TimeLib.h>

#include <EasyNextionLibrary.h>
#include <trigger.h>


#include <SPI.h>
#include "Adafruit_MAX31855.h"

#define MAXDO   3
#define MAXCS   4
#define MAXCLK  5

Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

#define pinRele 2


////////////////////////////////// VARIABLES //////////////////////////////////

EasyNex myNex(Serial);

struct Programa
{
  String nombre;   // Tipo de programa
  int tempObj; // En grados
  int tiempo;      // En minutos
  int progresionAsc;  // En grados/minuto
  int tempFinal;
  int progresionDesc;
};

int temperaturaSensor;
int temperaturaDeseada;
long tiempoFin;
long tiempoRestante;
bool haEmpezado = false;
int modoSeleccionado;
int statusCurvas = 0;

///////////////////////////////////////////////////////////////////////////////

////////////////////////////////// FUNCIONES //////////////////////////////////

// Funcion que actualiza el estado del elemento calefactor
void updateHeaterState(int temperaturaSel)
{
  temperaturaDeseada = temperaturaSel;
  temperaturaSensor = (int) thermocouple.readCelsius();
  if (temperaturaSensor < temperaturaSel - 4)
    digitalWrite(pinRele, LOW); // Encendemos la resistencia
  else if (temperaturaSensor >= temperaturaSel)
    digitalWrite(pinRele, HIGH); // Apgamos la resistencia

}

void safetyWatchdog() {
    if(temperaturaSensor > 350) {
      myNex.writeStr("page 2");
      digitalWrite(pinRele, HIGH);
      delay(100);
      while(1);
    }
  }

void exception(String mensajeExcepcion) {
    myNex.writeStr("page 3");
    myNex.writeStr("t1.txt", mensajeExcepcion);
    digitalWrite(pinRele, HIGH);
    delay(100);
    while(1);
}


///////////////////////////////////////////////////////////////////////////////

void setup()
{
  setTime(0);
  myNex.begin(9600);
  pinMode(pinRele, OUTPUT);    // Rele
  digitalWrite(pinRele, HIGH); // Abrimos el rele por seguridad

  Serial.begin(9600);

  if (!thermocouple.begin()) exception("No se ha podido inicializar el sensor de temperatura");


  modoSeleccionado = 0; // modo selecionado 0 - normal 1 - curvas
}

void loop()
{
  safetyWatchdog();
  temperaturaSensor = (int) thermocouple.readCelsius();
  if(temperaturaSensor == NAN) exception("Fallo en el sensor de temperatura");
  //temperaturaSensor = TEMP;
  modoSeleccionado = myNex.readNumber("cb0.val");
  switch (modoSeleccionado)
  {
    case 0:
      if (myNex.readNumber("sw0.val") == 1) {
        
        if(tiempoFin >= 0) {
          tiempoRestante = (now() - tiempoFin)/60;
          myNex.writeNum("n2.val", (tiempoFin - now())/60);
          temperaturaDeseada = myNex.readNumber("n0.val");
          if (now() < tiempoFin) updateHeaterState(temperaturaDeseada);
          else
          {
          //updateHeaterState(0); //Desactivamos el calefactor por seguridad
          myNex.writeNum("sw0.val", 0);
          digitalWrite(pinRele, HIGH); // Como el tiempo ha terminado reseteamos el switch a 0.
          } 
        }
        else {
          updateHeaterState(temperaturaDeseada);          
        }
        
      }
      else {
        if(myNex.readNumber("n2.val") > 0) {
            tiempoFin = myNex.readNumber("n2.val")*60 + now();          
        } 
        else tiempoFin = -1;
        digitalWrite(pinRele, HIGH); //Desactivamos el elemento calefactor por seguridad
      }
      break;
  case 1: // CURVAS
   struct Programa miPrograma;
    if (myNex.readNumber("sw0.val") == 1) {
      switch (statusCurvas) {
        case 0:       
          if(haEmpezado == false) {
            haEmpezado = true;
            tiempoFin = now() + 60;
            temperaturaDeseada = (int) thermocouple.readCelsius();
            
          }

          if(temperaturaSensor < miPrograma.tempObj) {
            if(tiempoFin < now()) {
              temperaturaDeseada = miPrograma.progresionAsc + temperaturaDeseada;
              tiempoFin = now() + 60;
            }
            updateHeaterState(temperaturaDeseada);
            
          }
          else {
            statusCurvas = 1;
            haEmpezado = false;
          }
          break;

        case 1:
            if(haEmpezado == false) {
              haEmpezado = true;
              tiempoFin = now() + miPrograma.tiempo*60;
              
            }
            tiempoRestante = (now() - tiempoFin)/60;
            myNex.writeNum("n0.val", (tiempoFin - now())/60);
            temperaturaDeseada = miPrograma.tempObj;
            if (now() < tiempoFin) updateHeaterState(temperaturaDeseada);
            else {
              statusCurvas = 2;
              haEmpezado = false;
            }
            break;
        case 2:
          if(haEmpezado == false) {
            tiempoFin = now() + 60;
            temperaturaDeseada = temperaturaSensor;
            haEmpezado = true;
          }

          if(temperaturaSensor > miPrograma.tempFinal) {
            if(tiempoFin < now()) {
              temperaturaDeseada = temperaturaDeseada - miPrograma.progresionDesc;
              tiempoFin = now() + 60;
            }
            updateHeaterState(temperaturaDeseada);
          }
          else {
            statusCurvas = 3;
            haEmpezado = false;
             myNex.writeNum("sw0.val", 0);
            digitalWrite(pinRele, HIGH);
            
          }
          break;
      }
    }
    else {
       miPrograma.tempObj = myNex.readNumber("n2.val");
       miPrograma.progresionAsc = myNex.readNumber("n3.val");
       miPrograma.tiempo = myNex.readNumber("n4.val");
       miPrograma.tempFinal = myNex.readNumber("n6.val");
       miPrograma.progresionDesc = myNex.readNumber("n7.val");
       statusCurvas = 0;
       tiempoFin = 0;
      digitalWrite(pinRele, HIGH);
      }
      myNex.writeNum("n1.val", (uint32_t) temperaturaDeseada);
      myNex.writeNum("n5.val", (uint32_t) temperaturaDeseada);


      break;
    }

    myNex.NextionListen();  
    myNex.writeNum("n1.val", (uint32_t) temperaturaSensor);
    
    delay(100);
    //myNex.writeNum("sw0.val", 0);
    // temperaturaDeseada = myNex.readNumber("n0.val"); // Solo lo hacemos en el setup se ajusta automaticamente a 90
  }
