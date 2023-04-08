#include <EasyNextionLibrary.h>
#include <trigger.h>
#include <Adafruit_MLX90614.h>
#include <timer.h>

#define pinRele 4


////////////////////////////////// VARIABLES //////////////////////////////////
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
EasyNex myNex(Serial);

struct Programa
{
  String nombre;   // Tipo de programa
  int tempObj[10]; // En grados
  int tiempo[10];      // En minutos
  int progresion[10];  // En grados/minuto
};

Programa* programas = (Programa*) malloc(10 * sizeof(Programa)); // Array que almacena las curvas
// Variables globales
Timer timerMain;
int temperaturaSensor;
int temperaturaDeseada;
long tiempoActual = 0;
int posicionPrograma = 0;
int modoSeleccionado;
int programaSeleccionado;

///////////////////////////////////////////////////////////////////////////////

////////////////////////////////// FUNCIONES //////////////////////////////////

// Funcion que actualiza el estado del elemento calefactor
void updateHeaterState(int temperaturaSel)
{
  temperaturaDeseada = temperaturaSel;
  temperaturaSensor = (int) mlx.readObjectTempC();
  if (temperaturaSensor < temperaturaSel)
    digitalWrite(pinRele,LOW); // Encendemos la resistencia
  else if (temperaturaSensor >= temperaturaSel + 1)
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


void updateTime() {
 tiempoActual++;
 if(modoSeleccionado == 1) {
    if(temperaturaDeseada < programas[programaSeleccionado].tempObj[posicionPrograma])
    {
    temperaturaDeseada += programas[programaSeleccionado].progresion[posicionPrograma];  //Actualizamos la curva
    }
  if(programas[programaSeleccionado].tiempo < tiempoActual) {
    posicionPrograma++;
    timerMain.stop();
    }
 }  
}
///////////////////////////////////////////////////////////////////////////////

void setup()
{
  myNex.begin(9600);
  pinMode(pinRele, OUTPUT);    // Rele
  digitalWrite(pinRele, HIGH); // Abrimos el rele por seguridad

  // TIMERS
  timerMain.setInterval(60000);
  timerMain.setCallback(updateTime);

  Serial.begin(9600);

  if (!mlx.begin()) exception("No se ha podido inicializar el sensor de temperatura");


  modoSeleccionado = 0; // modo selecionado 0 - normal 1 - curvas

  // Declaramos los programas en el array progrmas
  /****FIBRA****/
  programas[0].nombre = "Programa";
  programas[0].tempObj[0] = 30;
  programas[0].tempObj[1] = 60;
  programas[0].progresion[0] = 10;
  programas[0].progresion[1] = 10;
  programas[0].tiempo[0] = 5;
  programas[0].tiempo[1] = 5;
  /****BIAXIAL***
  programas[1].nombre = "Gurit";
  programas[1].tempObj = 135;
  programas[1].progresion = 2;
  programas[1].tiempo = 60;
 /****FIBRA DE VIDIRIO***
  programas[2].nombre = "F Vidrio";
  programas[2].tempObj = 125;
  programas[2].progresion = 2;
  programas[2].tiempo = 60;
  /**CURVA SOFISTICADA*
  programas[3].curva = {{85,1,120},{160,1, 120}}; //(Grados, progresion, tiempo) */
   
}

void loop()
{
  safetyWatchdog();
  temperaturaSensor = (int) mlx.readObjectTempC();
  if(temperaturaSensor == NAN) exception("Fallo en el sensor de temperatura");
  //temperaturaSensor = TEMP;
  modoSeleccionado = myNex.readNumber("cb0.val");
  switch (modoSeleccionado)
  {
    case 0:
    int tiempoTotal;
      if (myNex.readNumber("sw0.val") == 1) {
        if (timerMain.isStopped() == true) 
        {
          timerMain.start();
          tiempoTotal = myNex.readNumber("n2.val"); // Leemos pantalla y lo pasamos a minutos
        }
        timerMain.update();
        myNex.writeNum("n2.val", tiempoActual);
        temperaturaDeseada = myNex.readNumber("n0.val");
        if (tiempoTotal < tiempoActual) updateHeaterState(temperaturaDeseada);
        else
        {
          //updateHeaterState(0); //Desactivamos el calefactor por seguridad
          myNex.writeNum("sw0.val", 0);
          tiempoActual = 0;
          digitalWrite(pinRele, HIGH); // Como el tiempo ha terminado reseteamos el switch a 0.
        } 
      }
      else {
        timerMain.stop(); //Reseteamos el temporizador
        digitalWrite(pinRele, HIGH); //Desactivamos el elemento calefactor por seguridad
      }
      break;
  case 1: // CURVAS
   /*Indicar programa*/
    programaSeleccionado = myNex.readNumber("cb1.val");  

    boolean empiezaTimer = false;
      myNex.writeNum("n2.val", programas[programaSeleccionado].tempObj[posicionPrograma]);
      myNex.writeNum("n3.val", programas[programaSeleccionado].progresion[posicionPrograma]);
      myNex.writeNum("n4.val", programas[programaSeleccionado].tiempo[posicionPrograma]); 
    
  
    if (myNex.readNumber("sw0.val") == 1) {
     
      if (timerMain.isStopped() == true && empiezaTimer == true)
      {
        timerMain.start();
        tiempoActual = programas[programaSeleccionado].tiempo[posicionPrograma];
        temperaturaDeseada = temperaturaSensor;
      }
      if(empiezaTimer = true) timerMain.update();
      if(temperaturaSensor >= programas[programaSeleccionado].tempObj[posicionPrograma] - 1 )empiezaTimer = true;
      
      myNex.writeNum("n0.val", tiempoActual);
      myNex.writeNum("n5.val", temperaturaDeseada);
      if (tiempoActual < programas[programaSeleccionado].tiempo[posicionPrograma]) updateHeaterState(temperaturaDeseada);
      else if(empiezaTimer != false) {
        myNex.writeNum("sw0.val", 0);
        digitalWrite(pinRele, HIGH);
        }
    }
    else {
        //updateHeaterState(0); //Desactivamos el calefactor por seguridad
        timerMain.stop();
        digitalWrite(pinRele, HIGH); //Desactivamos el elemento calefactor por seguridad
      }
      
     break;
    
    }
    myNex.NextionListen();  
    myNex.writeNum("n1.val", (uint32_t) temperaturaSensor);
    
    delay(100);
    //myNex.writeNum("sw0.val", 0);
    // temperaturaDeseada = myNex.readNumber("n0.val"); // Solo lo hacemos en el setup se ajusta automaticamente a 90
  }
