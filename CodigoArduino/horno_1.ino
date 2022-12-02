#include <EasyNextionLibrary.h>
#include <trigger.h>
#include <Adafruit_MLX90614.h>
#include <timer.h>

#define pinRele 4

///////// Para probar sin horno /////////
#define TEMP  60 //En grados
/////////////////////////////////////////

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
EasyNex myNex(Serial);

struct Programa 
{
  String nombre; //Tipo de programa
  int tempInicial; //En grados
  int tiempo; //En minutos
  int progresion; //En grados/minuto
};

//Variables globales
Timer timer;
int temperaturaSensor;
int temperaturaDeseada;
int tiempoIncialms;


void hornear(int temperaturaSel) {
  temperaturaDeseada = temperaturaSel;
  //temperaturaSensor = (int) mlx.readObjectTempC();
  temperaturaSensor = TEMP;
  if (temperaturaSensor < temperaturaSel) digitalWrite(pinRele, LOW); //Apagamos la resistencia
  else if (temperaturaSensor >= temperaturaSel - 5) digitalWrite(pinRele, HIGH); //Encendemos la resistencia 

  Serial.print("HORNO EN FUNCIONAMIENTO, temperatura objetivo: ");
  Serial.print(temperaturaSel);
  Serial.print(" Temperatura actual: ");
  Serial.print(temperaturaSensor);
  Serial.println();
}


void setup() {
  myNex.begin(9600);
  pinMode(pinRele, OUTPUT); //Rele
  digitalWrite(pinRele, HIGH); //Abrimos el rele por seguridad
  timer.clearInterval();


  Serial.begin(9600);

  if (!mlx.begin()) {
    Serial.println("Error de conexion.");
    while (1);
  };

  Programa fibra;

  // Declaramos los valores de la curva de FIBRA //

  fibra.programa = "fibra";
  fibra.tempInicial = 90;
  fibra.tiempo = 90;
  fibra.progresion = 1;
}

void loop() {
  //temperaturaSensor = (int) mlx.readObjectTempC();
  temperaturaSensor = TEMP;
  int modo = 0 //modo selecionado 0 - normal 1 - curvas
  switch (modo) {
    case 0:
      if (/*switch de la pantalla*/ true) {
        if(timer.isStopped == true) {
          timer.start();
          tiempoIncialms = 0 /*Leer de pantalla*/
          }
        while (tiempoIncialms > timer.getElapsedTime() && /*switch de la pantalla true*/) {
          temperaturaDeseada = myNex.readNumber("n0.val");
          hornear(temperaturaDeseada); 
        }
        break;
      }
    case 1:
      /*Codigo curvas*/

  }
  myNex.NextionListen();
  myNex.writeNum("n1.val", (uint32_t) temperaturaSensor);
  temperaturaDeseada = myNex.readNumber("n0.val");
  hornear(temperaturaDeseada);



}
