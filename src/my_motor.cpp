#include "my_motor.h"

int initMotorPWM()
{
  const int motor1Pin = GPIO_NUM_12;
  const int motor2Pin = GPIO_NUM_13;
  int ret = 0;
  Serial.printf("pin %d - channel %d freq %d res %d\n", motor1Pin, motor1Channel, motorFreq, motorRes);
  ledcAttachPin(motor1Pin, motor1Channel);       // 将通道与对应的引脚连接
  ledcSetup(motor1Channel, motorFreq, motorRes); // 设置通道

  Serial.printf("pin %d - channel %d freq %d res %d\n", motor2Pin, motor2Channel, motorFreq, motorRes);
  ledcAttachPin(motor2Pin, motor2Channel);       // 将通道与对应的引脚连接
  ledcSetup(motor2Channel, motorFreq, motorRes); // 设置通道
  Serial.println("Motor PWM inited.");
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
    ledcWrite(motor1Channel,calculatePWM(degree));// 输出PWM
    delay(20);
    return ret;
}

int set_motor2(int degree)
{
    int ret = 0;
    Serial.printf("motor2 set to degree %d\n", degree);
    ledcWrite(motor2Channel,calculatePWM(degree));// 输出PWM
    delay(20);
    return ret;
}