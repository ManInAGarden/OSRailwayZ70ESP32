#include <ArduinoLog.h>
#include "LocoMotor.h"


LocoMotor::LocoMotor(int channel, int inpin1, int inpin2,  int pwmpin, int stdbypin, double frequency, int bits, int accelstep) {
  motchannel = channel;
  motpwmpin = pwmpin;
  motinpin1 = inpin1;
  motinpin2 = inpin2;
  motstdbypin = stdbypin;
  motfrequency = frequency;
  motbits = bits;
  target_speed = 0;
  actual_speed = 0;
  motion_direction = forward;
  acceleration_step = accelstep;
  max_speed = pow(2, bits) - 1;
  coaster_enabled = false;
  timer_delta = 10; //timer ticks every 10ms or later whenever operate() is called
  last_time = 0;

  pinMode(motstdbypin, OUTPUT);
  pinMode(motstdbypin, OUTPUT);
  digitalWrite(motstdbypin, LOW);
  pinMode(motinpin1, OUTPUT);
  pinMode(motinpin2, OUTPUT);

  ledcSetup(motchannel, motfrequency, motbits); //set PWM frequency and resolution for motor
  ledcAttachPin(motpwmpin, motchannel); //attach channel to gpio pin
  ledcWrite(motchannel, 0);

  Log.traceln("Motor initialized on channel %d, pwmpin %d, stdbypin %d, in1/2 %d/%d",
    motchannel,
    motpwmpin,
    motstdbypin,
    motinpin1, motinpin2);

  Log.traceln("Max speed value for motor is %d", max_speed);
}

void LocoMotor::immediate_stop(){
  Log.traceln("in LocoMotor::immediate_stop()");
  ledcWrite(motchannel, 0);
  actual_speed = 0;
  target_speed = 0;
  //digitalWrite(motinpin1, HIGH);
  //digitalWrite(motinpin2, HIGH);
}

void LocoMotor::start_operation() {
  Log.traceln("in LocoMotor::start_operation()");
  coaster_enabled = true;
  set_motor_polarity(motion_direction);
  direct_set_speed(0);
  digitalWrite(motstdbypin, HIGH);
  Log.traceln("LocoMotor::start_operation() done");
}

void LocoMotor::set_motor_polarity(motordirenum dir){
  	bool dirb = (dir==forward) ? false : true;
    Log.traceln("Setting motor polarity to (1/2) %d/%d", dirb, !dirb);
    digitalWrite(motinpin1, dirb);
    digitalWrite(motinpin2, !dirb);
}

void LocoMotor::set_speed(unsigned int speed, motordirenum newdir) {
  Log.traceln("in LocoMotor::set_speed(speed=%d, dir=%d)", speed, newdir);

  target_speed = speed;
  bool dochange = false;

  if(newdir == backward){
    if(motion_direction != backward && actual_speed > 100){
      target_speed = 0;
    } else {
      dochange = true;
    }
  } else {
    if(motion_direction != forward && actual_speed > 100){
      target_speed = 0;
    } else {
      dochange = true;
    }
  }

  if(dochange)
    change_direction(newdir);
}

void LocoMotor::set_speed(unsigned int speed) {
  Log.traceln("in LocoMotor::set_speed(speed=%d)", speed);

  target_speed = speed;
}

void LocoMotor::change_direction(motordirenum nd){
  Log.traceln("in LocoMotor::change_direction(dir=%d)", nd);

  if (motion_direction != nd) {
    if (nd==forward){
      Log.traceln("Changing direction to <forward>");
      digitalWrite(motinpin1, HIGH);
      digitalWrite(motinpin2, LOW);  
    }
    else {
      Log.traceln("Changing direction to <backward>");
      digitalWrite(motinpin1, LOW);
      digitalWrite(motinpin2, HIGH);  
    }
    motion_direction = nd;
  }
}

void LocoMotor::motion_control(){
    if(actual_speed < target_speed){
      actual_speed = actual_speed + acceleration_step;
    } else if(actual_speed > target_speed){
      actual_speed = actual_speed - acceleration_step;
    }    
    if(actual_speed > max_speed){
      actual_speed = max_speed;
    }
    if(actual_speed < 0){
      actual_speed = 0;
    }

    if (actual_speed != target_speed)
      direct_set_speed(actual_speed);
}

void LocoMotor::direct_set_speed(int speedval){
  Log.traceln("in LocoMotor::direct_set_speed(speedval=%d)", speedval);
  ledcWrite(motchannel, speedval);
}

bool LocoMotor::time_over(){
  if(!coaster_enabled)
    return false;
    
  if(last_time > millis()*2)//millis restarted
		last_time = 0;

  if ((unsigned long int)(millis() - last_time) >= timer_delta) {
		last_time = millis();
    return true;
  }

  return false;
}

void LocoMotor::operate() {
  if(time_over())
		motion_control();
}