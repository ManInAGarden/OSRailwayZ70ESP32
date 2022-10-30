/* Originally programmed Olle Sköld in 2017
 * adjusted for an ESP32WROOM DevBoard by me in 2022
 * See Olle's excellent OSRailway project on thingiverse
 *
 * History since forked from the original:
 * - added Logging to the serial port
 * - changed analogWrite for the motor and headlights to ledcWrite methods and complete channel definition
 *   because default channel definitions seem to have changed
 * 
 * Wifi accesspoint using NodeMCU and ESP32 WROOM.
 * 
 * This software is provided free and you may use, change and distribute as you like.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h> 
#include <WebServer.h>
#include <ArduinoLog.h>
#include <ESPmDNS.h>
#include "LocoMotor.h"


#define MOT_CHAN 1 //PWN channel for motor
#define MOT_CHAN_FREQ 5000 //PWN frequence ist 1000 Hz
#define MOT_CHAN_RES 10 //channel resultion is 10 bit, whcih gives values 0..1023
#define MOT_STDBY 27 //GIO 27 for motor driver standby
#define MOT_PWM 0 //GPIO0 for motor speed PWM signal

/* Set these to your desired credentials. */
const char *ssid = "OSRZ70";
const char *password = "osrailway";


bool highBeamActive = false;
bool motionActive = false;


int LED_BL2 = 15; // Red light on side 2 GPIO15
int LED_BL1 = 13; // Red light on side 1 GPIO13
int LED_HB2 = 14; // High beam side 2 GPIO12
int LED_HB1 = 12; // High beam side 1 GPIO14

int LED_HDL = 2;  //PWM for headlights controlled by motor board

int motor_AIN1 = 5; //Motor in pin 1 GPIO5
int motor_AIN2 = 4; //Motor in pin2 GPIO4

LocoMotor* motor;

WebServer server(80);

/************************************************************************** 
 *  
 *  Server IP address is: http://192.168.4.1
 *  
 *  When you have connected your device to the wifi network, open the browser and enter the IP address above.
 *  
 */
void handleRoot() {
  /************************************************************************ 
  * This is the web page sent to the connected device. There are lots of memory available so this page can be made much more complex. 
  * It uses Ajax to send commands which means the page doesn't need to be reloaded each time a button is pressed. In this way, 
  * the page can be made much more graphically complex without suffering from long reloading delays.
  */
  Log.traceln("In handleRoot");
  String html ="<html> <header> <style type=\"text/css\"> body { background: #595B82; } #header { height: 30px; margin-left: 2px; margin-right: 2px; background: #d5dfe2; border-top: 1px solid #FFF; border-left: 1px solid #FFF; border-right: 1px solid #333; border-bottom: 1px solid #333; border-radius: 5px; font-family: \"arial-verdana\", arial; padding-top: 10px; padding-left: 20px; } #speed_setting { float: right; margin-right: 10px; } #control { height: 200px; margin-top: 4px; margin-left: 2px; margin-right: 2px; background: #d5dfe2; border-top: 1px solid #FFF; border-left: 1px solid #FFF; border-right: 1px solid #333; border-bottom: 1px solid #333; border-radius: 5px; font-family: \"arial-verdana\", arial; } .button { border-top: 1px solid #FFF; border-left: 1px solid #FFF; border-right: 1px solid #333; border-bottom: 1px solid #333; border-radius: 5px; margin: 5px; text-align: center; float: left; padding-top: 20px; height: 50px; background: #FFF} .long { width: 30%; } .short { width: 100px; } #message_box { float: left; margin-bottom: 0px; width: 100%; } #speed_slider {width: 300px;}</style> <script type=\"text/javascript\"> function updateSpeed(speed) { sendData(\"updateSpeed?speed=\"+speed); } function runForward(){ var speed = document.getElementById(\"speed_slider\").value; sendData(\"run?dir=forward&speed=\"+speed); } function runBackward(){ var speed = document.getElementById(\"speed_slider\").value; sendData(\"run?dir=backward&speed=\"+speed); } function sendData(uri){ var messageDiv = document.getElementById(\"message_box\"); messageDiv.innerHTML = \"192.168.4.1/\"+uri; var xhr = new XMLHttpRequest(); xhr.open('GET', 'http://192.168.4.1/'+uri); xhr.withCredentials = true; xhr.setRequestHeader('Content-Type', 'text/plain'); xhr.send(); } </script> </header> <body> <div id=\"header\"> OS-Railway Wifi Z70 <div id=\"speed_setting\"> Speed <input id=\"speed_slider\" type=\"range\" min=\"0\" max=\"1023\" step=\"10\" value=\"512\" onchange=\"updateSpeed(this.value)\"> </div> </div> <div id=\"control\"> <div id=\"button_backward\" class=\"button long\" onclick=\"runBackward()\"> Run backward </div> <div id=\"button_stop\" class=\"button long\" onclick=\"updateSpeed(0)\"> Stop </div> <div id=\"button_forward\" class=\"button long\" onclick=\"runForward()\"> Run forward </div> <div id=\"button_light_off\" class=\"button long\" onclick=\"sendData('lightoff')\"> Headlight off </div> <div id=\"button_light_on\" class=\"button long\" onclick=\"sendData('lighton')\"> Headlight on </div> <div id=\"button_emergency\" class=\"button long\" onclick=\"sendData('stop')\"> Emergency stop </div> <div id=\"message_box\"> </div> </div> </body> </html>";
  server.send(200, "text/html", html);
}


/**************************************************
 *  Motor operation
 */

void motorOperation(){

  Log.traceln("In motorOperation()");
  motionActive = true;  
  String move_dir = server.arg("dir");
  String motion_speed = server.arg("speed");
  int target_speed = motion_speed.toInt();  
  motordirenum direct;

  if (move_dir=="backward") {
    direct = backward;
  }
  else {
    direct = forward;
  }

  motor->set_speed(target_speed, direct);

  motordirenum md = motor->get_direction();
  bool dirb = (md==forward) ? true : false;
  digitalWrite(LED_BL2, dirb);
  digitalWrite(LED_BL1, !dirb);
    
  if(highBeamActive){
    digitalWrite(LED_HB2, dirb);
    digitalWrite(LED_HB1, !dirb);
  } else {
    digitalWrite(LED_HB2, 0);
    digitalWrite(LED_HB1, 0);
  }
}

void updateSpeed(){
  Log.traceln("In updateSpeed");
  String motion_speed = server.arg("speed");
  unsigned int target_speed = motion_speed.toInt();
  motor->set_speed(target_speed);  
}

void switchLightOn(){
    Log.traceln("In switchLightOn");
    highBeamActive = true;
    motordirenum md = motor->get_direction();
    bool dirb = (md==forward) ? true : false;
    digitalWrite(LED_HB2, dirb);
    digitalWrite(LED_HB1, !dirb);
}

void switchLightOff(){
    Log.traceln("In switchLightOff");
    highBeamActive = false;
    digitalWrite(LED_HB2, 0);
    digitalWrite(LED_HB1, 0);  
}

void motionStop(){
  Log.traceln("In motionStop");
  motionActive = false;
  motor->immediate_stop();
}




void flashBacklights() {
  digitalWrite(LED_BL2, 0);
  digitalWrite(LED_BL1, 0);
  delay(1000);
  digitalWrite(LED_BL1, 1);
  digitalWrite(LED_BL2, 1);
  delay(1000);
  digitalWrite(LED_BL2, 0);
  digitalWrite(LED_BL1, 0);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


/*********************************************************************
 * SETUP
 */

void setup() {
	delay(1000);

  Serial.begin(19200);
  while(!Serial && !Serial.available()){}

  Log.begin(LOG_LEVEL_VERBOSE, &Serial);

  Serial.println();
  Serial.println();
  Log.traceln("Creating Access Point: %s", ssid);
  
  motor = new LocoMotor(MOT_CHAN, motor_AIN1, motor_AIN2, MOT_PWM, MOT_STDBY, MOT_CHAN_FREQ, MOT_CHAN_RES);
  pinMode(LED_BL2, OUTPUT);
  pinMode(LED_BL1, OUTPUT);
  pinMode(LED_HB2, OUTPUT);
  pinMode(LED_HB1, OUTPUT);
  
  pinMode(LED_HDL, OUTPUT);
  Log.traceln("%s", "Declaration of pin modes successful");

  flashBacklights(); //flash red lights to signal that most of initialisation has been done
  
  digitalWrite(LED_HB2, 0);
  digitalWrite(LED_HB1, 0);

  analogWrite(LED_HDL, 1023);

  Log.traceln("Enabling motor operations from now on");
  motor->start_operation();
	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();
  Log.traceln("Accesspoint ready at IP: %p", myIP);

  if (MDNS.begin("esp32")) {
    Log.traceln("MDNS responder started");
  }

 /****************************************************************************
  * Here you find the functions that are called when the browser sends commands. 
  */
	server.on("/", handleRoot);
  server.on("/run", motorOperation);
  server.on("/stop", motionStop);
  server.on("/lighton", switchLightOn);
  server.on("/lightoff", switchLightOff);
  server.on("/updateSpeed", updateSpeed);
  server.onNotFound(handleNotFound);

  Log.traceln("Setup completed, starting loop now");
	server.begin();
}

void loop() {
	server.handleClient();
  motor->operate(); //one step for motor decellaration/acceleration
  delay(2); //allow cpu to do other things
}

