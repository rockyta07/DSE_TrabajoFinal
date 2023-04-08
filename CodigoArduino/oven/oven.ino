#include <EasyNextionLibrary.h>
#include <trigger.h>
#include <Adafruit_MLX90614.h>
#include <arduino-timer.h> //#include <TimerOne.h> //#include <timer.h>


// CONFIGURATION CONSTANTS
#define RELAY_PIN 4  // Signal pin to the relay. 5V = OFF = CUT, 0V = ON = ALLOW
//#define HEATER_DELAY   // Delay in miliseconds (60000 means every minute)
#define RELAY_OFF HIGH
#define RELAY_ON LOW

/*
Update the display at least every second
Update the relay once every minute approx (to not wear it)


*/


////////////////////////////////// VARIABLES //////////////////////////////////
Adafruit_MLX90614 temp_sensor = Adafruit_MLX90614(); // Temperature sensor
EasyNex display(Serial); // Display
/*
sw0.val -> ON/OFF TOGGLE
n2.val -> Minutes Left

*/

struct point {
  long temp_C; // Target temperature in Celsius
  long time_s; // Target time in seconds
};

#define MAX_POINTS

struct curve { // Name and Array of time/temperature points to follow
  String name; // Name of the curve
  point points[MAX_POINTS]; // Array with points to follow
};

// Create timer with ms resolution
auto timer = timer_create_default();  // auto means deduce variable type

int sensor_celsius;
int target_celsius;
long minutes_left;
int mode = 0; // 0: manual,
int programaSeleccionado; // TODO REMOVE, EACH PROGRAM IS A MODE. SIMPLER.

////////////////////////////////// FUNCTIONS //////////////////////////////////
void set_heater(int target_celsius) {
  /*
  Turns the resistor ON or OFF depending on the temperature.

  Takes 'target_celsius' as target temperature in celsius,
  Reads the current temperature from the MLX90614 sensor
  If the current temperature is lower, turns the resistor ON.
  Otherwise, turns the resistor OFF.

  */
  int sensor_celsius = (int)temp_sensor.readObjectTempC();
  if (sensor_celsius < target_celsius)
    digitalWrite(RELAY_PIN, RELAY_ON);
  else if (sensor_celsius >= target_celsius)
    digitalWrite(RELAY_PIN, RELAY_OFF);
}

void temp_watchdog(int temp_C) {
  /*
  IF VALUE OUT OF NORMAL RANGE, CUT RELAY, ERROR IN DISPLAY, sleep 1 second
  */

  if ((temp_C <= 0) || (temp_C > 150)) {
    display.writeStr("page 2");
    digitalWrite(RELAY_PIN, RELAY_OFF);
    delay(1000);
  } else if (temp_C == NAN){
    exception("Fallo en el sensor de temperatura");
  }
}

void exception(String error_string) {
  /*
  Prints error_string in the nextion display
  */
  display.writeStr("page 3");
  display.writeStr("t1.txt", error_string);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  delay(1000);
}


void manual_mode() {
  /*
  Updates the time variable in code and the display
  TODO SEPARATE minutes_left--; from this function?
  */
  minutes_left = display.readNumber("n2.val");
  minutes_left--;

  if (minutes_left == 0){
    // TIME ENDED.
    digitalWrite(RELAY_PIN, RELAY_OFF); // Heater OFF
    display.writeNum("sw0.val", 0); // Display toggle OFF
    timer.cancel(); // End all the tasks of the timer
    display.writeNum("n2.val", minutes_left);
  } else {
    // STILL WORK TO DO
    display.writeNum("n2.val", minutes_left);
    target_celsius = display.readNumber("n0.val");
    set_heater(target_celsius);
  }
}

void setup() {
  display.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  Serial.begin(9600);

  if (!temp_sensor.begin()) exception("No se ha podido inicializar el sensor de temperatura");
}

void loop() {
  // Nextion touch response
  display.NextionListen();
  // Update timer status and check whether arrived at setpoint
  timer.tick();
  // Read sensor temperature
  sensor_celsius = (int)temp_sensor.readObjectTempC();
  display.writeNum("n1.val", (uint32_t)sensor_celsius);
  // If crazy, error
  temp_watchdog(sensor_celsius);
  // Read the mode from the nextion display
  mode = display.readNumber("cb0.val");

  target_celsius = display.readNumber("n0.val");  // Solo lo hacemos en el setup se ajusta automaticamente a 90
  display.writeNum("sw0.val", 0);

  // IF TOGGLE ON BUT TIMER OFF -> SET THE TIMER -> STARTS A TASK
  if ((display.readNumber("sw0.val") == 1) && timer.empty()) {
    switch (mode) {
      case 0:  // MANUAL MODE - SET TARGET TEMPERATURE AND TIME UNTIL OFF.
        auto manual_mode_task = timer.every(60000, manual_mode); // Update the time every DELAY in miliseconds, then the function to call periodically
        break;
      case 1:  // CURVAS
        break;
    }
  }
}

// TODO SOMETHING BLINKING ON THE SCREEN TO KNOW IT'S NOT FROZEN



//////////////
// /*Indicar programa*/
// programaSeleccionado = display.readNumber("cb1.val");

// display.writeNum("n2.val", programas[programaSeleccionado].tempObj);
// display.writeNum("n3.val", programas[programaSeleccionado].progresion);
// display.writeNum("n4.val", programas[programaSeleccionado].tiempo);


// if (display.readNumber("sw0.val") == 1) {

//   if (timer.isStopped() == true)
//   {
//     timer.start();
//     minutes_left = programas[programaSeleccionado].tiempo;
//     target_celsius = sensor_celsius;
//   }
//   timer.update();
//   display.writeNum("n0.val", minutes_left);
//   display.writeNum("n5.val", target_celsius);
//   if (minutes_left > 0) set_heater(target_celsius);
//   else {
//     display.writeNum("sw0.val", 0);
//     digitalWrite(RELAY_PIN, RELAY_OFF);
//   }
// }
// else {
//     //set_heater(0); //Desactivamos el calefactor por seguridad
//     timer.stop();
//     digitalWrite(RELAY_PIN, RELAY_OFF);
//   }

// break;

