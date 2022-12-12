#include <EasyNextionLibrary.h>
#include <trigger.h>
#include <Adafruit_MLX90614.h>
#include <timer.h>

#define pinRele 4

///////// Para probar sin horno /////////
#define TEMP 60 // En grados
/////////////////////////////////////////

////////////////////////////////// VARIABLES //////////////////////////////////
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
EasyNex myNex(Serial);

struct Programa
{
  String nombre;   // Tipo de programa
  int tempObj; // En grados
  int tiempo;      // En minutos
  int progresion;  // En grados/minuto
};

Programa* programas = (Programa*) malloc(10 * sizeof(Programa)); // Array que almacena las curvas
// Variables globales
Timer timerMain;
Timer timerCurva;
int temperaturaSensor;
int temperaturaDeseada;
long tiempoRestante;
int modoSeleccionado;
int programaSeleccionado;
boolean falloSeguridad = false;

///////////////////////////////////////////////////////////////////////////////

////////////////////////////////// FUNCIONES //////////////////////////////////

// Funcion que actualiza el estado del elemento calefactor
void updateHeaterState(int temperaturaSel)
{
  temperaturaDeseada = temperaturaSel;
  temperaturaSensor = (int) mlx.readObjectTempC();
  if (temperaturaSensor < temperaturaSel)
    digitalWrite(pinRele,LOW); // Encendemos la resistencia
  else if (temperaturaSensor >= temperaturaSel + 2)
    digitalWrite(pinRele, HIGH); // Apgamos la resistencia

}

void safetyWatchdog() {
    if(temperaturaSensor > 150) myNex.writeStr("page page 3");
    digitalWrite(pinRele, HIGH);
  }

void updateCurveTemp() {
  temperaturaDeseada += programas[programaSeleccionado].progresion;  
}

void updateTime() {
 tiempoRestante--;
}
///////////////////////////////////////////////////////////////////////////////

void setup()
{
  myNex.begin(9600);
  pinMode(pinRele, OUTPUT);    // Rele
  digitalWrite(pinRele, HIGH); // Abrimos el rele por seguridad

  // TIMERS
  timerMain.setInterval(60000);
  timerCurva.setInterval(60000);
  timerMain.setCallback(updateTime);
  timerCurva.setCallback(updateCurveTemp);

  Serial.begin(9600);

  if (!mlx.begin()) falloSeguridad = true; 

  modoSeleccionado = 0; // modo selecionado 0 - normal 1 - curvas

  // Declaramos los programas en el array progrmas
  /****FIBRA****/
  programas[0].nombre = "fibra";
  programas[0].tempObj = 90;
  programas[0].progresion = 1;
   programas[0].tiempo = 90;
  /****OTRO****/
}

void loop()
{
  
  temperaturaSensor = (int) mlx.readObjectTempC();
  //temperaturaSensor = TEMP;
  modoSeleccionado = myNex.readNumber("cb0.val");
    switch (modoSeleccionado)
    {
    case 0:
      if (myNex.readNumber("sw0.val") == 1) {
        if (timerMain.isStopped() == true)
        {
          timerMain.start();
          tiempoRestante = myNex.readNumber("n2.val"); // Leemos pantalla y lo pasamos a minutos
        }
        timerMain.update();
        myNex.writeNum("n2.val", tiempoRestante);
        temperaturaDeseada = myNex.readNumber("n0.val");
        if (tiempoRestante > 0) updateHeaterState(temperaturaDeseada);
        
        else
        {
          //updateHeaterState(0); //Desactivamos el calefactor por seguridad
          myNex.writeNum("sw0.val", 0);
          digitalWrite(pinRele, HIGH); // Como el tiempo ha terminado reseteamos el switch a 0.
        } 
      }
      else {
        //updateHeaterState(0); //Desactivamos el calefactor por seguridad
        myNex.writeNum("sw0.val", 0);
        timerMain.stop(); //Reseteamos el temporizador
      }
      break;
  case 1: // CURVAS
   /*Indicar programa*/
    programaSeleccionado = myNex.readNumber("cb1.val");  
    
      myNex.writeNum("n2.val", programas[programaSeleccionado].tempObj);
      myNex.writeNum("n3.val", programas[programaSeleccionado].progresion);
      myNex.writeNum("n4.val", programas[programaSeleccionado].tiempo); 
    
  
    if (myNex.readNumber("sw0.val") == 1) {
      if (timerMain.isStopped() == true)
      {
        timerMain.start();
        timerCurva.start();
        tiempoRestante = programas[programaSeleccionado].tiempo;
      }
      tiempoRestante -= timerMain.getElapsedTime();
      myNex.writeNum("n0.val", tiempoRestante);
      if (tiempoRestante > 0) updateHeaterState(temperaturaDeseada);
      else {
        myNex.writeNum("sw0.val", 0);
        digitalWrite(pinRele, HIGH);
        
        
        }
    }
    else {
        //updateHeaterState(0); //Desactivamos el calefactor por seguridad
        timerMain.stop();
        timerCurva.stop();
        myNex.writeNum("sw0.val", 0);
      }
     break;
    
    }
    myNex.NextionListen();  
    myNex.writeNum("n1.val", (uint32_t) temperaturaSensor);
    delay(100);
    //myNex.writeNum("sw0.val", 0);
    // temperaturaDeseada = myNex.readNumber("n0.val"); // Solo lo hacemos en el setup se ajusta automaticamente a 90
  }
