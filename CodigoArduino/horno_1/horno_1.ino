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
  int tempObj; // En grados
  int tiempo;      // En minutos
  int progresion;  // En grados/minuto
};

Programa* programas = (Programa*) malloc(10 * sizeof(Programa)); // Array que almacena las curvas
// Variables globales
Timer timerMain;
int temperaturaSensor;
int temperaturaDeseada;
long tiempoRestante;
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
  else if (temperaturaSensor >= temperaturaSel + 2)
    digitalWrite(pinRele, HIGH); // Apgamos la resistencia

}

void safetyWatchdog() {
    if(temperaturaSensor > 150) {
      myNex.writeStr("page page 3");
      digitalWrite(pinRele, HIGH);
      delay(100);
    }
  }

void exception(String mensajeExcepcion) {
  while(true) {
    myNex.writeStr("page page 4");
    myNex.writeStr("t1.txt", mensajeExcepcion);
    digitalWrite(pinRele, HIGH);
    delay(100);
    
  }
}


void updateTime() {
 tiempoRestante--;
 if(programaSeleccionado == 1) temperaturaDeseada += programas[programaSeleccionado].progresion;  //Actualizamos la curva
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
  programas[0].nombre = "fibra";
  programas[0].tempObj = 90;
  programas[0].progresion = 1;
   programas[0].tiempo = 90;
  /****BIAXIAL****/
  programas[1].nombre = "biaxial";
  programas[1].tempObj = 135;
  programas[1].progresion = 1;
  programas[1].tiempo = 60;
 /****FIBRA DE VIDIRIO****/  
  programas[1].nombre = "F Vidrio";
  programas[1].tempObj = 125;
  programas[1].progresion = 1;
  programas[1].tiempo = 60;
   
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
        digitalWrite(pinRele, HIGH); //Desactivamos el elemento calefactor por seguridad
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
        tiempoRestante = programas[programaSeleccionado].tiempo;
      }
      myNex.writeNum("n0.val", tiempoRestante);
      if (tiempoRestante > 0) updateHeaterState(temperaturaDeseada);
      else {
        myNex.writeNum("sw0.val", 0);
        //digitalWrite(pinRele, HIGH);
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
