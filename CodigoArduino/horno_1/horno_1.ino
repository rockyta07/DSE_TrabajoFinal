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
int tiempoFuncionamiento;
int modoSeleccionado;
int programaSeleccionado;
boolean falloSeguridad = false;

///////////////////////////////////////////////////////////////////////////////

////////////////////////////////// FUNCIONES //////////////////////////////////

// Funcion que actualiza el estado del elemento calefactor
void updateHeaterState(int temperaturaSel)
{
  temperaturaDeseada = temperaturaSel;
  // temperaturaSensor = (int) mlx.readObjectTempC();
  temperaturaSensor = TEMP;
  if (temperaturaSensor < temperaturaSel)
    digitalWrite(pinRele, LOW); // Apagamos la resistencia
  else if (temperaturaSensor >= temperaturaSel - 5)
    digitalWrite(pinRele, HIGH); // Encendemos la resistencia

  Serial.print("HORNO EN FUNCIONAMIENTO, temperatura objetivo: ");
  Serial.print(temperaturaSel);
  Serial.print(" Temperatura actual: ");
  Serial.print(temperaturaSensor);
  Serial.println();
}

void safetyWatchdog() {
    if(temperaturaSensor > 150) 
  }

void updateCurveTemp() {
  temperaturaDeseada += programas[programaSeleccionado].progresion;  
}
///////////////////////////////////////////////////////////////////////////////

void setup()
{
  myNex.begin(9600);
  pinMode(pinRele, OUTPUT);    // Rele
  digitalWrite(pinRele, HIGH); // Abrimos el rele por seguridad

  // TIMERS
  timerMain.clearInterval();
  timerCurva.setInterval(60000);
  timerCurva.setCallback(updateCurveTemp);

  Serial.begin(9600);

  if (!mlx.begin()) falloSeguridad = true; 

  modoSeleccionado = 0; // modo selecionado 0 - normal 1 - curvas

  // Declaramos los programas en el array progrmas
  /****FIBRA****/
  programas[0].nombre = "fibra";
  programas[0].tempObj = 90;
  programas[0].progresion = 1;
  /****OTRO****/
}

void loop()
{
  myNex.NextionListen();
  // temperaturaSensor = (int) mlx.readObjectTempC();
  temperaturaSensor = TEMP;
  modoSeleccionado = myNex.readNumber("cb0.val");
    switch (modoSeleccionado)
    {
    case 0:
      if (myNex.readNumber("sw0.val") == 1) {
        if (timerMain.isStopped() == true)
        {
          timerMain.start();
          tiempoFuncionamiento = myNex.readNumber("") * 60000; // Leemos pantalla y lo pasamos a minutos
        }
        timerMain.update();
        if (tiempoFuncionamiento > timerMain.getElapsedTime())
        {
          temperaturaDeseada = myNex.readNumber("n0.val");
          updateHeaterState(temperaturaDeseada);
        }
        else
        {
          updateHeaterState(0); //Desactivamos el calefactor por seguridad
          myNex.writeNum("sw0.val", 0); // Como el tiempo ha terminado reseteamos el switch a 0.
        } 
      }
      else {
        updateHeaterState(0); //Desactivamos el calefactor por seguridad
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
        tiempoFuncionamiento = programas[programaSeleccionado].tiempo * 60000;
      }
      tiempoFuncionamiento -= timerMain.getElapsedTime();
      myNex.writeNum("n0.val", (int) (tiempoFuncionamiento/60000));
      if (tiempoFuncionamiento > 0) updateHeaterState(temperaturaDeseada);
      else {
        myNex.writeNum("sw0.val", 0);
        
        
        }
    }
    else {
        updateHeaterState(0); //Desactivamos el calefactor por seguridad
        timerMain.stop();
        timerCurva.stop();
      }
     break;
    
    }

    myNex.writeNum("n1.val", (uint32_t)temperaturaSensor);
    // temperaturaDeseada = myNex.readNumber("n0.val"); // Solo lo hacemos en el setup se ajusta automaticamente a 90
  }
