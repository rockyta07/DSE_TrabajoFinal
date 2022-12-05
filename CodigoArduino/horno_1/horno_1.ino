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
  int tempInicial; // En grados
  int tiempo;      // En minutos
  int progresion;  // En grados/minuto
};

Programa *programas = malloc(10 * sizeof(Programa)); // Array que almacena las curvas
// Variables globales
Timer timerMain;
Timer timerCurva;
int temperaturaSensor;
int temperaturaDeseada;
int tiempoFuncionamiento;
int modoSeleccionado;
int programaSelecionado;

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

void updateCurveTemp() {
  temperaturaDeseada += programas[programaSelecionado].progresion;  
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

  if (!mlx.begin())
  {
    Serial.println("Error de conexion.");
    while (1)
      ;
  };

  modoSeleccionado = 0; // modo selecionado 0 - normal 1 - curvas

  // Declaramos los programas en el array progrmas
  /****FIBRA****/
  programas[0].nombre = "fibra";
  programas[0].tempInicial = 90;
  programas[0].progresion = 1;
  /****OTRO****/
}

void loop()
{
  myNex.NextionListen();
  // temperaturaSensor = (int) mlx.readObjectTempC();
  temperaturaSensor = TEMP;
  if (myNex.readNumber("sw0.val") == 1)
  { // Comprobamos el estado del interruptor
    switch (modoSeleccionado)
    {
    case 0:
      // Switch on-off pantalla
      if (timerMain.isStopped == true)
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
        timerMain.stop();
        myNex.writeNumber("sw0.val", 0); // Como el tiempo ha terminado reseteamos el switch a 0.
      }

      break;
    }
  case 1: // CURVAS
    if (timerMain.isStopped == true)
    {
      programaSeleccionado = 0; /*Indicar programa*/
      while (temperaturaSensor < programas[programaSeleccionado].tempInicial())
      { // Precalentado
        updateHeaterState(programas[programaSeleccionado].tempInicial());
        temperaturaSensor = TEMP;
      }

      timerMain.start();
      timerCurva.start();
    }
    if (tiempoFuncionamiento > timerMain.getElapsedTime())
    {
      temperaturaDeseada = myNex.readNumber("n0.val");
      updateHeaterState(temperaturaDeseada);
      break;
    }

    myNex.writeNum("n1.val", (uint32_t)temperaturaSensor);
    // temperaturaDeseada = myNex.readNumber("n0.val"); // Solo lo hacemos en el setup se ajusta automaticamente a 90
  }
}
