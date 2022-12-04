#include <EasyNextionLibrary.h>
#include <trigger.h>
#include <Adafruit_MLX90614.h>

#define pinRele 4

///////// Para probar sin horno /////////
#define TEMP  60 //En grados
/////////////////////////////////////////

////////////////////////////////// VARIABLES //////////////////////////////////
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
EasyNex myNex(Serial);

struct Programa 
{
  String nombre; //Tipo de programa
  int tempInicial; //En grados
  int tiempo; //En minutos
};


int temperaturaSensor;
int temperaturaDeseada;
bool precalentado = false;
String modoSeleccionado;

///////////////////////////////////////////////////////////////////////////////

////////////////////////////////// FUNCIONES //////////////////////////////////

// Para seleccionar la temperatura menual = modo normal
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


///////////////////////////////////////////////////////////////////////////////



void setup() {
  myNex.begin(9600);
  pinMode(pinRele, OUTPUT); //Rele
  digitalWrite(pinRele, HIGH); //Abrimos el rele por seguridad

  Serial.begin(9600);

  if (!mlx.begin()) {
    Serial.println("Error de conexion.");
    while (1);
  };


  // DEFINIMOS LOS STRUCT //
  // Programa FIBRA //
  Programa fibra; // Creamos un struct tipo programa 
  
  // Declaramos los valores de la curva de FIBRA //
  fibra.nombre = "fibra";
  fibra.tempInicial = 90;
  fibra.tiempo = 90;

  // Programa NORMAL //
  Programa normal; // Creamos un struct tipo normal

  // Declaramos los valores de la curva de NORMAL //
  normal.nombre = "normal";
  normal.tempInicial = 90;
  normal.tiempo = 90;

  // SELECCIONAMOS EL MODO //
  // ...
  // modoSeleccionado = Programa.nombre;
  modoSeleccionado = fibra.nombre; //Por ejemplo

  if (modoSeleccionado.compareTo(fibra.nombre)){
      temperaturaDeseada = fibra.tempInicial;
  } else { //Por defecto el modo es el normal
      temperaturaDeseada = myNex.readNumber("n0.val");
  }

  
}

void loop() {
  //temperaturaSensor = (int) mlx.readObjectTempC();
  temperaturaSensor = TEMP;
  myNex.NextionListen();
  myNex.writeNum("n1.val", (uint32_t) temperaturaSensor);
  //temperaturaDeseada = myNex.readNumber("n0.val"); // Solo lo hacemos en el setup se ajusta automaticamente a 90

  
  // Precalentamos el horno 
  hornear(temperaturaDeseada); //Si es modo normal se selecciona la temperatura, si no lo es
}
