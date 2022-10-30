#ifndef LOCOMOTOR_h
#define LOCOMOTOR_h

#include <Arduino.h>

enum motordirenum {forward=0, backward=1};

class LocoMotor {
  private:
    int motchannel;
    int motpwmpin,
      motstdbypin,
      motinpin1,
      motinpin2;
    double motfrequency;
    int motbits; //resolution in bits
    int target_speed,
      actual_speed,
      acceleration_step,
      max_speed;
    motordirenum motion_direction;
    bool coaster_enabled;
    unsigned long int timer_delta,
      last_time; 

    void motion_control();
    void set_motor_polarity(motordirenum dir);
    void change_direction(motordirenum newdirection);
    bool time_over();
    void direct_set_speed(int speedval);

  public:
    LocoMotor(int channel, int inpin1, int inpin2, int pwmpin, int stdbypin, double frequency, int bits, int accelstep=5);
    
    /// @brief start motor operations
    void start_operation();
    
    /// @brief set motor speed to given speedval. Speed will slowly advance or decrease to the given value
    /// @param speedval Value of the speed
    /// @param direction Direction of the motor (values forward and backward)
    void set_speed(unsigned int speedval, motordirenum direction);
    
    /// @brief Set new speed but keep current direction of rotation
    /// @param speedval new value for rotational speed
    void set_speed(unsigned int speedval);

    /// @brief Immediately stop the motor, no coasting!
    void immediate_stop();

    /// @brief stop the motor, but allow to coast to standstill
    void stop();

    /// @brief execute on operational step
    void operate();

    int get_actual_speed() {return actual_speed;}
    int get_acceleration_step() {return acceleration_step;}
    int get_max_speed() {return max_speed;}
    motordirenum get_direction() {return motion_direction;}
};

#endif