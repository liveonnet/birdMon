#include <Arduino.h>

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