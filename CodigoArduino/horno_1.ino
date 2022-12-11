#include <EasyNextionLibrary.h> //importamos librerias nextion
#include <trigger.h> //Una biblioteca Arduino para activar un interruptor externo basado en una variable medida que supera un umbral
#include <Adafruit_MLX90614.h> //Libreria del MLX90614 sensor de temperatura
#include <timer.h> //temporizador de inicio/parada simple que puede decirle cuánto tiempo ha transcurrido desde que se inició
//////////////////CURVA///////////////////

NexWaveform curva = NexWaveform(0,1,"curva");//se crea curva que es el waveform creado para mostrar las curvas en el nextion
///////////////////////////////////////////////

#define pinRele 4 //definimos rele

///////// Para probar sin horno /////////
#define TEMP 60 // En grados
/////////////////////////////////////////

////////////////////////////////// VARIABLES //////////////////////////////////
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); //creamos mlx que será el sensor de temperatura
EasyNex myNex(Serial);

struct Programa //creamos una estructura
{
  String nombre;   // Tipo de programa
  int tempInicial; // En grados
  int tiempo;      // En minutos
  int progresion;  // En grados/minuto
};

Programa *programas = malloc(10 * sizeof(Programa)); // Array que almacena las curvas, se hace con memoria dinámica
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
void updateHeaterState(int temperaturaSel)//se mete como parámetro la temperatura seleccionada
{
  temperaturaDeseada = temperaturaSel; //la igualamos a la deseada
  // temperaturaSensor = (int) mlx.readObjectTempC();
  temperaturaSensor = TEMP; //los 60 grados lo guardamos en temperatura del sensor
  if (temperaturaSensor < temperaturaSel)//si la selecionada es mayor que la del sensor
    digitalWrite(pinRele, LOW); // Apagamos la resistencia
  else if (temperaturaSensor >= temperaturaSel - 5) //sino, si es mayor
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
  myNex.begin(9600);//lanzamos el nextion
  pinMode(pinRele, OUTPUT);    // Rele
  digitalWrite(pinRele, HIGH); // Abrimos el rele por seguridad

  // TIMERS
  timerMain.clearInterval();
  timerCurva.setInterval(60000);
  timerCurva.setCallback(updateCurveTemp);

  Serial.begin(9600);//monitor serie

  if (!mlx.begin()) //si el sensor de temperatura no empieza
  {
    Serial.println("Error de conexion.");//printeamos un error de conexion
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
  temperaturaSensor = TEMP;//los 60 grados a la temperaturaSensor
  if (myNex.readNumber("sw0.val") == 1)//si pulsamos en el boton que se encuentra en la interfaz del horno en el nextion
  { // Comprobamos el estado del interruptor
    switch (modoSeleccionado) //comprobamos el estado
    {
    case 0:
      // Switch on-off pantalla
      if (timerMain.isStopped == true) //si se ha pulsado
      {
        timerMain.start(); //el tiempo empieza a contar
        tiempoFuncionamiento = myNex.readNumber("") * 60000; // Leemos pantalla y lo pasamos a minutos
      }
      timerMain.update();//actualizamos el tiempo
      if (tiempoFuncionamiento > timerMain.getElapsedTime()) //??
      {
        temperaturaDeseada = myNex.readNumber("n0.val");//n0 es la temperatura objetivo marcada en nextion
        updateHeaterState(temperaturaDeseada); // Funcion que actualiza el estado del elemento calefactor
      }
      else
      {
        timerMain.stop();//se para el tiempo
        myNex.writeNumber("sw0.val", 0); // Como el tiempo ha terminado reseteamos el switch a 0.
      }

      break;
    }
  case 1: // CURVAS
    if (timerMain.isStopped == true)//si se ha pulsado?
    {
      programaSeleccionado = 0; /*Indicar programa*/ //nombre fibra, 90 tem ini, 1 progresion
      while (temperaturaSensor < programas[programaSeleccionado].tempInicial())//se va precalentando hasta que la temperatura llega a la incial que es 90
      { // Precalentado
        updateHeaterState(programas[programaSeleccionado].tempInicial());// Funcion que actualiza el estado del elemento calefactor
        temperaturaSensor = TEMP;//60
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
    curva.add(0,temperaturaDeseada);
    curva.add(0,temperaturaSensor);
    myNex.writeNum("n1.val", (uint32_t)temperaturaSensor);
    // temperaturaDeseada = myNex.readNumber("n0.val"); // Solo lo hacemos en el setup se ajusta automaticamente a 90
  }
}
