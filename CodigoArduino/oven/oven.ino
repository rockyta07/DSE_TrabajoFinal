#include <EasyNextionLibrary.h>
#include <trigger.h>
#include <Adafruit_MLX90614.h>
#include "TimeLib.h" // arduino-timer makes the arduino nano crash with no errors, and restarts infinitely. //#include <TimerOne.h> //#include <timer.h> #<timeLib.h>


// CONFIGURATION CONSTANTS
#define RELAY_PIN 2  // Signal pin to the relay. 5V = OFF = CUT, 0V = ON = ALLOW
#define BEEPER_RELAY 10
//#define HEATER_DELAY   // Delay in miliseconds (60000 means every minute)
#define HEART_LED 13
bool heart_led_on = false;
#define RELAY_OFF HIGH
#define RELAY_ON LOW // Hay que dar tierra

/// Thermocouple
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#define MAXDO   3
#define MAXCS   4
#define MAXCLK  5
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

/*
Update the display at least every second
Update the relay once every minute approx (to not wear it)
https://github.com/rockyta07/DSE_TrabajoFinal/tree/main

# TODO
- Think redundancy if sensor goes crazy
- Test curva enfriamiento


# DISPLAY (Nextion)
toggle.val -> ON/OFF TOGGLE
remaining.val -> Minutes Left
mode.val -> Mode


display.writeNum("{name}.val", 0);
where {name} is:
- mode switcher (ComboBox): mode
- Toggle (Switch, to turn on, starts the timer for the curve): toggle

Show:
- actual temp: sensor
- target temp: target
- elapsed time: elapsed
- remaining time: remaining
- total time: total

*/


////////////////////////////////// VARIABLES //////////////////////////////////
EasyNex display(Serial); // Display

struct point {
  long celsius; // Target temperature in Celsius
  long minutes; // Target time in seconds
};

#define MAX_POINTS 12

struct curve { // Name and Array of time/temperature points to follow
  String name; // Name of the curve
  int points; // Amount of points to follow
  point point[MAX_POINTS]; // Array with points to follow
};

int sensor_celsius;
int mode = 2; // 0: manual, 1: ciclo manual, 2: curvas
int programaSeleccionado; // TODO REMOVE, EACH PROGRAM IS A MODE. SIMPLER.
int target_celsius;

curve cur;

////////////////////////////////// FUNCTIONS //////////////////////////////////
void beep() {
  for (int i=0; i < 8; i++){
    for (int j=0; j < 20; j++){
      // 1/10ms = 200Hz sound
      digitalWrite(BEEPER_RELAY, HIGH);
      delay(5);
      digitalWrite(BEEPER_RELAY, LOW);
      delay(5);
    }
    delay(200);
  }
}

void set_heater(int target_celsius) {
  /*
  Turns the resistor ON or OFF depending on the temperature.

  Takes 'target_celsius' as target temperature in celsius,
  Reads the current temperature from the MLX90614 sensor
  If the current temperature is lower, turns the resistor ON.
  Otherwise, turns the resistor OFF.

  */
  int sensor_celsius = 25;
  if (sensor_celsius < target_celsius)
    digitalWrite(RELAY_PIN, RELAY_ON);
  else if (sensor_celsius >= target_celsius)
    digitalWrite(RELAY_PIN, RELAY_OFF);
}

int read_temperature() {
  /* Reads the temperature of the thermocouple and checks everything OK.
  */
  int sensor_celsius = thermocouple.readCelsius();
  
  if (isnan(sensor_celsius)) {
    // Serial.println("Thermocouple fault(s) detected!");
    // uint8_t e = thermocouple.readError();
    // Serial print error with more details
    // if (e & MAX31855_FAULT_OPEN) Serial.println("FAULT: Thermocouple is open - no connections.");
    // if (e & MAX31855_FAULT_SHORT_GND) Serial.println("FAULT: Thermocouple is short-circuited to GND.");
    // if (e & MAX31855_FAULT_SHORT_VCC) Serial.println("FAULT: Thermocouple is short-circuited to VCC.");
    // Show error in display and disconnect relay
    exception("Thermocouple error, can't read temperature.", false);
  } else {
    return sensor_celsius;
  }
}

void exception(String error_string, bool stop) {
  /*
  Prints error_string in the nextion display
  */
  digitalWrite(RELAY_PIN, RELAY_OFF);
  display.writeStr("page 3");
  display.writeStr("t1.txt", error_string);
  beep();
  delay(2000);
  if (stop) {
    while (1){
      beep();
    }
  }
}

void manual_mode() {
  /*
  Updates the time variable in code and the display
  TODO SEPARATE minutes_left--; from this function, so this one can go fast?

  
  */
  /*
  static long minutes_left = display.readNumber("remaining.val");
  minutes_left--;

  if (minutes_left == 0){
    // TIME ENDED.
    digitalWrite(RELAY_PIN, RELAY_OFF); // Heater OFF
    display.writeNum("toggle.val", 0); // Display toggle OFF
    display.writeNum("remaining.val", minutes_left);
  } else {
    // STILL WORK TO DO
    display.writeNum("remaining.val", minutes_left);
    target_celsius = display.readNumber("sensor.val");
    set_heater(target_celsius);
  }
  */
}

void curve_mode() {
  /*
  Starts reading the curve and going linearly from point to point as time goes on.
  TODO PASS A CURVE TO MAKE THIS FUNCTION GENERIC, instead of using cur directly

  Depending on the value of toggle.val, it will start or stop the process of following the curve.
  */
  static int toggle_on = 0;
  static int toggle_previous_on = 0;

  toggle_on = display.readNumber("toggle2.val"); // TODO READ THE RIGHT VALUE
  int now_minutes = int(now()/60);
  
  Serial.println("");
  Serial.print("Toggle: ");
  Serial.println(toggle_on);

  // Update time
  display.writeNum("elapsed2.val", now_minutes);
  display.writeNum("total2.val", cur.point[cur.points-1].minutes);
  display.writeNum("remaining2.val", cur.point[cur.points-1].minutes - now_minutes);

  // If just pressed the toggle
  if (toggle_on && !toggle_previous_on) {
    // Restart the curve
    define_curve(); // To update the time
  } else if (toggle_on) {
    // Continue with the curve

    // If time passed > than time of final point
    if (now() > cur.point[cur.points - 1].minutes) {
      // TIME ENDED.
      digitalWrite(RELAY_PIN, RELAY_OFF); // Heater OFF
      display.writeNum("toggle2.val", 0); // Turn OFF the toggle
      display.writeNum("target2.val", -1); // Display target of -1ºC
    } else {
      // Still work to do

      // Find the next point to follow
      int i = 0;
      while (cur.point[i].minutes < now_minutes) {
        // When the point is in the future, exit the while loop
        i++;
      }

      /*
      if (i == 0) {
        exception("curve_mode ERROR. Either the code or the curve are wrong.", true);
      }
      */

      // i is the next point
      int target_celsius = interpolate(now(), cur.point[i-1].minutes, cur.point[i-1].celsius, cur.point[i].minutes, cur.point[i].celsius);
      // TODO write minutes_passed or graph, GUI.
      set_heater(target_celsius);
      display.writeNum("target.val", target_celsius); // Display target of target_celsius
    }
  }

  toggle_previous_on = toggle_on;
}

int interpolate(int t, int t1, int T1, int t2, int T2) {
  /*
  t is the current time, t1 and T1 the time and temperature of the first point, t2 and T2 the same for the next point

  Returns the temperature that corresponds to the time t between those points.
  */
  return (int)(T1 + (t-t1)/(t2-t1) * (T2 - T1));
}

void define_curve(){
  /*
  Defines the declared curve with the time when it's been called

  */
  cur.name = "cur: 2ºC/min, 85ºC 1.5h, 120ºC 1h";
  cur.points = 6;
  // Punto inicial
  cur.point[0].celsius = sensor_celsius;
  cur.point[0].minutes = now();
  
  // Calentar a 2ºC/min hasta 85ºC
  cur.point[1].celsius = 85;
  cur.point[1].minutes = cur.point[0].minutes + (85 - cur.point[0].celsius) * (1/2); // Temp difference * (1 min / 2 ºC)
  // Mantener a 85ºC 1.5h
  cur.point[2].celsius = 85;
  cur.point[2].minutes = cur.point[1].minutes + 1.5 * 60;
  // Calentar 2ºC/min hasta 120ºC
  cur.point[3].celsius = 120;
  cur.point[3].minutes = cur.point[2].minutes + (cur.point[3].celsius - cur.point[2].celsius) * (1/2); // Temp difference * (1 min / 2 ºC)
  // Mantener 120ºC 1h
  cur.point[4].celsius = 120;
  cur.point[4].minutes = cur.point[3].minutes + 60;
  // Enfriar a 2ºC/min hasta 60ºC
  cur.point[5].celsius = 60;
  cur.point[5].minutes = cur.point[4].minutes + (cur.point[4].celsius - cur.point[5].celsius) * (1/2);
}

void setup() {
  display.begin(9600);
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(HEART_LED, OUTPUT);
  pinMode(BEEPER_RELAY, OUTPUT);

  // Startup check and leave OFF
  digitalWrite(RELAY_PIN, RELAY_ON);
  delay(50);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  delay(50);
  
  if (!thermocouple.begin()) {
    Serial.println("Thermocouple error.");
    exception("ERROR: Sensor de temperatura no se pudo inicializar",true);
  }
  
  sensor_celsius = read_temperature();  
  Serial.print("Temperature: ");
  Serial.println(sensor_celsius);


  setTime(0);
  ////////// CURVES ///////////
  define_curve();

  display.writeNum("sensor_temp2.val",sensor_celsius);
  display.writeNum("target2.val",-1);
  display.writeNum("remaining2.val",0);

}

void loop() {
  // Heart led toggle
  digitalWrite(13, heart_led_on);
  heart_led_on = !heart_led_on;

  // Nextion touch response
  display.NextionListen();
  
  // Read sensor temperature
  sensor_celsius = thermocouple.readCelsius();
  if ((sensor_celsius <= -20) || (sensor_celsius > 200) || (sensor_celsius == NAN)) {
    exception("Temperatura fuera de rango o NAN",false);
  }
  Serial.print("T [ºC]: ");
  Serial.println(sensor_celsius);

// DELETEME  display.writeNum("target.val", sensor_celsius);
  display.writeNum("sensor_temp2.val", sensor_celsius);
  //display.writeNum("elapsed2.val", now()/60);
  
  // Read the mode from the nextion display
  mode = display.readNumber("mode_val.val"); // TODO READ RELIABLY
  
  Serial.print("Mode: ");
  Serial.println(mode);
  switch (mode) {
    case 0:
      manual_mode();
      break;
    case 1:
      //curvas();
      break;
    case 2:
      curve_mode();
      break;
  }
  

}

// TODO SOMETHING BLINKING ON THE SCREEN TO KNOW IT'S NOT FROZEN
// TODO CHECK RIGHT POLARITY OF RELAY SIGNAL, IF DISCONNECTED -> OFF


