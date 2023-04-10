#include <EasyNextionLibrary.h>
#include <trigger.h>
#include <Adafruit_MLX90614.h>
#include <arduino-timer.h> //#include <TimerOne.h> //#include <timer.h> #<timeLib.h>


// CONFIGURATION CONSTANTS
#define RELAY_PIN 13  // Signal pin to the relay. 5V = OFF = CUT, 0V = ON = ALLOW
//#define HEATER_DELAY   // Delay in miliseconds (60000 means every minute)
#define RELAY_OFF HIGH
#define RELAY_ON LOW // Hay que dar tierra

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
- actual temp: temp
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

#define MAX_POINTS

struct curve { // Name and Array of time/temperature points to follow
  String name; // Name of the curve
  int points; // Amount of points to follow
  point point[MAX_POINTS]; // Array with points to follow
};

// Create timer with ms resolution
auto timer = timer_create_default();  // auto means deduce variable type

int sensor_celsius;
int mode = 1; // 0: manual,
int programaSeleccionado; // TODO REMOVE, EACH PROGRAM IS A MODE. SIMPLER.
int target_celsius;

curve cur;

////////////////////////////////// FUNCTIONS //////////////////////////////////
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

void temp_watchdog(int celsius) {
  /*
  IF VALUE OUT OF NORMAL RANGE, CUT RELAY, ERROR IN DISPLAY, sleep 1 second
  */

  if ((celsius <= 0) || (celsius > 200)) {
    display.writeStr("page 2");
    digitalWrite(RELAY_PIN, RELAY_OFF);
    delay(1000);
  } else if (celsius == NAN){
    exception("Fallo en el sensor de temperatura",false);
  }
}

void exception(String error_string, bool stop) {
  /*
  Prints error_string in the nextion display
  */
  display.writeStr("page 3");
  display.writeStr("t1.txt", error_string);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  delay(1000);
  if (stop) {
    while (1);
  }
  
}

void manual_mode() {
  /*
  Updates the time variable in code and the display
  TODO SEPARATE minutes_left--; from this function, so this one can go fast?
  */
  static long minutes_left = display.readNumber("remaining.val");
  minutes_left--;

  if (minutes_left == 0){
    // TIME ENDED.
    digitalWrite(RELAY_PIN, RELAY_OFF); // Heater OFF
    display.writeNum("toggle.val", 0); // Display toggle OFF
    timer.cancel(); // End all the tasks of the timer
    display.writeNum("remaining.val", minutes_left);
  } else {
    // STILL WORK TO DO
    display.writeNum("remaining.val", minutes_left);
    target_celsius = display.readNumber("temp.val");
    set_heater(target_celsius);
  }
}

void curve_mode() {
  /*
  Starts reading the curve and going linearly from point to point as time goes on.
  TODO PASS A CURVE TO MAKE THIS FUNCTION GENERIC, instead of using cur directly
  */
  static long minutes_passed = 0;

  // If time passed > than time of final point
  if (minutes_passed > cur.point[cur.points - 1].minutes) {
    // TIME ENDED.
    digitalWrite(RELAY_PIN, RELAY_OFF); // Heater OFF
    display.writeNum("toggle.val", 0); // Display toggle OFF
    display.writeNum("temp.val", target_celsius); // Display the temperature
    display.writeNum("target.val", 0); // Display target of 0ºC
    timer.cancel(); // End all the tasks of the timer
  } else {
    // Still work to do

    // Find the next point to follow
    int i = 0;
    while (cur.point[i].minutes < minutes_passed) {
      // When the point is in the future, exit the while loop
      i++;
    }

    if (i == 0) {
      exception("curve_mode ERROR. Either the code or the curve are wrong.", true);
    }

    // Now i is the next point
    int target_celsius = interpolate(minutes_passed, cur.point[i-1].minutes, cur.point[i-1].celsius, cur.point[i].minutes, cur.point[i].celsius);
    // TODO write minutes_passed or graph, GUI.
    set_heater(target_celsius);
    display.writeNum("temp.val", target_celsius); // Display the temperature
    display.writeNum("target.val", target_celsius); // Display target of target_celsius
  }

  // Update time
  minutes_passed++;
  display.writeNum("elapsed.val", minutes_passed);
  display.writeNum("total.val", cur.point[cur.points-1]);
  display.writeNum("remaining.val", cur.point[cur.points-1] - minutes_passed);
  
}

void setup() {
  display.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  Serial.begin(9600);

  ////////// CURVES ///////////
  cur.name = "cur: 2ºC/min, 85ºC 1.5h, 120ºC 1h";
  cur.points = 6;
  // Punto inicial
  cur.point[0].celsius = sensor_celsius;
  cur.point[0].minutes = 0;
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

void loop() {
  digitalWrite(RELAY_PIN, RELAY_OFF);
  // Nextion touch response
  display.NextionListen();
  // Update timer status and check whether arrived at setpoint
  timer.tick();
  // Read sensor temperature
  sensor_celsius = 25; // TODO THERMOCOUPLE
  display.writeNum("n1.val", (uint32_t)sensor_celsius);
  // If crazy, error
  temp_watchdog(sensor_celsius);
  // Read the mode from the nextion display
  mode = display.readNumber("mode.val");

  display.writeNum("toggle.val", 0);

  // IF TOGGLE ON BUT TIMER OFF -> SET THE TIMER -> STARTS A TASK
  Serial.println(display.readNumber("toggle.val") == 1);
  Serial.println(timer.empty());
  if ((display.readNumber("toggle.val") == 1) && timer.empty()) {
    switch (mode) {
      case 0:  // MANUAL MODE - SET TARGET TEMPERATURE AND TIME UNTIL OFF.
        auto manual_mode_task = timer.every(60000, manual_mode); // Update the time every DELAY in miliseconds, then the function to call periodically
        break;
      case 1:  // cur CURVE
        auto curve_mode_task = timer.every(60000, curve_mode); // Update the time every DELAY in miliseconds, then the function to call periodically
        break;
    }
  } else if (display.readNumber("toggle.val") == 0) {
    // If toggle OFF
    timer.cancel(); // End all the tasks of the timer. Interrupt any works in progress.
  }
}

int interpolate(int t, int t1, int T1, int t2, int T2) {
  /*
  t is the current time, t1 and T1 the time and temperature of the first point, t2 and T2 the same for the next point

  Returns the temperature that corresponds to the time t between those points.
  */
  return (int)(T1 + (t-t1)/(t2-t1) * (T2 - T1));
}

// TODO SOMETHING BLINKING ON THE SCREEN TO KNOW IT'S NOT FROZEN
// TODO CHECK RIGHT POLARITY OF RELAY SIGNAL, IF DISCONNECTED -> OFF


