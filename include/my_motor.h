
// PWM设置相关
const int motorFreq = 50;   // 频率(20ms周期)
const int motorChannel = 8; // 通道(高速通道（0 ~ 7）由80MHz时钟驱动，低速通道（8 ~ 15）由 1MHz 时钟驱动。)
const uint8_t motorRes = 8; // 分辨率

int initMotorPWM();

int calculatePWM(int degree);

int set_motor1(int degree);
int set_motor2(int degree);