#include "comm.h"
#include "camera_web_server.h"
#include "my_motor.h"
#include "private_info.h"

// 深度休眠唤醒次数计数, 硬件重启会清零
RTC_DATA_ATTR int bootCount = 0;

int imageNum = 0;

void startCameraServer();

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // pinMode(GPIO_NUM_4, OUTPUT);
  // digitalWrite(GPIO_NUM_4, LOW);
  // rtc_gpio_hold_en(GPIO_NUM_4);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // 中断处理
  // pinMode(GPIO_NUM_16, INPUT_PULLUP);
  // attachInterrupt(GPIO_NUM_16, isrRestart, FALLING);

  bootCount++;
  if (bootCount == 1)
  {
    /*
    首次启动
    连接wifi校正时间
    初始化相机，舵机pwm
    */
    Serial.println("first time to boot");
    connectWiFi(ssid, password);
    syncTime();

    Serial.printf("Camera Ready! Use 'http://%s' to connect\n", WiFi.localIP().toString().c_str());
    initCamera();
    initSd();
    initMotorPWM();
    startCameraServer();
  }
  else
  {
    /*
    非首次启动
    计算睡眠时间
    判断被唤醒原因
      1 PIR触发
          初始化相机
          初始化tf卡
          保存截图到tf卡
          继续睡眠
      2 超时
          初始化相机
          初始化tf卡
          保存截图到tf卡
          继续睡眠
      3 其他外部gpio触发
          重启
    其他原因
      继续睡眠
    */
    // pinMode(GPIO_NUM_4, OUTPUT);
    // digitalWrite(GPIO_NUM_4, LOW);
    // rtc_gpio_hold_en(GPIO_NUM_4);

    Serial.printf("boot count %d\n", bootCount);
    calcDuration();
    int io_num = printWakeupReason();
    switch (io_num)
    {
    case 0: // 超时和触发 ext_wakeup_pin_1 一个效果，都是拍照保存
      Serial.println("timer wake up");

      initCamera();
      initSd();

      // pinMode(GPIO_NUM_4, OUTPUT);
      // digitalWrite(GPIO_NUM_4, LOW);
      // rtc_gpio_hold_en(GPIO_NUM_4);

      // rtc_gpio_hold_dis(GPIO_NUM_4);
      // pinMode(GPIO_NUM_4, INPUT);
      // digitalWrite(GPIO_NUM_4, LOW);

      pinMode(LED_GPIO, OUTPUT);
      digitalWrite(LED_GPIO, LOW); // turn the red led next to the reset button ON
      pic2tf(imageNum);
      digitalWrite(LED_GPIO, HIGH); // turn the red led next to the reset button OFF
      delay(1000);
      break;
    case ext_wakeup_pin_1:
      Serial.printf("io %d wake up\n", ext_wakeup_pin_1);
      initCamera();
      initSd();
      pinMode(LED_GPIO, OUTPUT);
      digitalWrite(LED_GPIO, LOW); // turn the red led next to the reset button ON
      pic2tf(imageNum);
      delay(1000);
      digitalWrite(LED_GPIO, HIGH); // turn the red led next to the reset button OFF
      break;
    case ext_wakeup_pin_2: // 重启
      Serial.printf("io %d wake up, to restart ...\n", ext_wakeup_pin_1);
      esp_restart(); // bootCount将重新改为0
      break;
    default:
      Serial.println("unknown wake up cause?\n");
      break;
    }
    initWakeup();
    startSleep();
  }

  // 不会执行到这里
  // pinMode(LED_GPIO, OUTPUT);
}

void loop()
{
  // put your main code here, to run repeatedly:
  // delay(10000);
}