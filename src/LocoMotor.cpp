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

  if (stdbypin != NOSTDBY_PIN) {
    handle_stdby = true;
    pinMode(motstdbypin, OUTPUT);
    digitalWrite(motstdbypin, LOW);
  }

  pinMode(motinpin1, OUTPUT);
  pinMode(motinpin2, OUTPUT);

  ledcSetup(motchannel, motfrequency, motbits); //set PWM frequency and resolution for motor
  ledcAttachPin(motpwmpin, motchannel); //attach channel to gpio pin
  ledcWrite(motchannel, 0);
  change_direction(forward);

  Log.traceln("Motor %d - Motor initialized on channel %d, pwmpin %d, stdbypin %d, in1/2 %d/%d",
    motchannel,
    motchannel,
    motpwmpin,
    motstdbypin,
    motinpin1, motinpin2);

  Log.traceln("Motor %d - Max speed value for motor is %d", motchannel, max_speed);
}

void LocoMotor::immediate_stop(){
  Log.traceln("Motor %d - in LocoMotor::immediate_stop()", motchannel);
  ledcWrite(motchannel, 0);
  actual_speed = 0;
  target_speed = 0;
}

void LocoMotor::start_operation() {
  Log.traceln("Motor %d - in LocoMotor::start_operation()", motchannel);
  coaster_enabled = true;
  set_motor_polarity(motion_direction);
  direct_set_speed(0);
  if(handle_stdby)
    digitalWrite(motstdbypin, HIGH);
  Log.traceln("Motor %d - LocoMotor::start_operation() done", motchannel);
}

void LocoMotor::set_motor_polarity(motordirenum dir){
  	bool dirb = (dir==forward) ? false : true;
    Log.traceln("Motor %d - Setting motor polarity to (1/2) %d/%d", motchannel, dirb, !dirb);
    digitalWrite(motinpin1, dirb);
    digitalWrite(motinpin2, !dirb);
}

/// @brief set speed an direction
/// @param speed new speed value
/// @param newdir new direction
void LocoMotor::set_speed(unsigned int speed, motordirenum newdir) {
  Log.traceln("Motor %d - in LocoMotor::set_speed(speed=%d, dir=%d)", motchannel, speed, newdir);

  if (speed <= max_speed)
    target_speed = speed;
  else
    target_speed = max_speed;
  
  bool dochange = false;

  if (newdir != motion_direction && actual_speed > 100)
    target_speed = 0;
  else
    dochange = true;

  if(dochange)
    change_direction(newdir);
}

const char* LocoMotor::get_direction_str(){
  return (motion_direction==forward) ? "forward" : "backward";
}

void LocoMotor::set_speed(unsigned int speed) {
  Log.traceln("Motor %d - in LocoMotor::set_speed(speed=%d)", motchannel, speed);
  if (speed <= max_speed)
    target_speed = speed;
  else
    target_speed = max_speed;
}

void LocoMotor::change_direction(motordirenum nd){
  Log.traceln("Motor %d - in LocoMotor::change_direction(dir=%d)", motchannel, nd);

  if (motion_direction != nd) {
    Log.traceln("Changing direction to <%s>", get_direction_str());
    set_motor_polarity(nd);
    motion_direction = nd;
  }
}

void LocoMotor::motion_control(){
  if (actual_speed==target_speed)
    return;

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

  int ms = get_max_speed();
  if (actual_speed > ms)
    actual_speed = ms;

  direct_set_speed(actual_speed);
}

void LocoMotor::direct_set_speed(int speedval){
  Log.traceln("Motor %d - in LocoMotor::direct_set_speed(speedval=%d)", motchannel, speedval);
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