#include <Arduino.h>
#include "my_motor.h"

int initMotorPWM()
{
  const int motor1Pin = GPIO_NUM_13;
  const int motor2Pin = GPIO_NUM_12;
  int ret = 0;
  ledcAttachPin(motor1Pin, motorChannel);       // 将通道与对应的引脚连接
  ledcSetup(motorChannel, motorFreq, motorRes); // 设置通道

  ledcAttachPin(motor2Pin, motorChannel);       // 将通道与对应的引脚连接
  ledcSetup(motorChannel, motorFreq, motorRes); // 设置通道
  return ret;
}


int calculatePWM(int degree)
{
  // 20ms周期，高电平0.5-2.5ms，对应0-180度
  const float deadZone = 6.4; // 对应0.5ms（0.5ms/(20ms/256）)
  const float max = 32;       // 对应2.5ms

  if (degree < 0)
    degree = 0;
  if (degree > 180)
    degree = 180;

  return (int)(((max - deadZone) / 180) * degree + deadZone);
}


int set_motor1(int degree)
{
    int ret = 0;
    Serial.printf("motor1 set to degree %d\n", degree);
    return ret;
}

int set_motor2(int degree)
{
    int ret = 0;
    Serial.printf("motor2 set to degree %d\n", degree);
    return ret;
}