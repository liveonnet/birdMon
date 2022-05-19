#include <Arduino.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <driver/rtc_io.h>
#include <sys/time.h>
#include "camera_web_server.h"

#define LED_GPIO 33

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "**********";
const char *password = "**********";
// 深度休眠唤醒次数计数, 硬件重启会清零
RTC_DATA_ATTR int bootCount = 0;
// 休眠唤醒相关
#define TIME_TO_SLEEP 20         // Time ESP32 will go to sleep (in seconds)
#define uS_TO_S_FACTOR 1000000   // Conversion factor for micro seconds to seconds
const int ext_wakeup_pin_1 = 25; // 25脚为唤醒外部中断1
const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
const int ext_wakeup_pin_2 = 26; // 26脚为唤醒外部中断2
const uint64_t ext_wakeup_pin_2_mask = 1ULL << ext_wakeup_pin_2;

void startCameraServer();

void printWakeupReason()
{
  esp_sleep_wakeup_cause_t wakeup_case = esp_sleep_get_wakeup_cause();
  // 判断唤醒原因
  switch (wakeup_case)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("wakeup via RTC_IO signal");
    break;
  case ESP_SLEEP_WAKEUP_EXT1: // 外部中断1 唤醒
  {
    uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_pin_mask != 0)
    {
      int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
      Serial.printf("Wake up from GPIO %dn\n", pin);
    }
    else
    {
      Serial.printf("Wake up from GPIOn\n");
    }
    break;
  }
  case ESP_SLEEP_WAKEUP_TIMER: // 定时器唤醒
  {
    // printf("Wake up from timer. Time spent in deep sleep: %dmsn", sleep_time_ms);
    Serial.println("Wake up from timer.");
    break;
  }
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("wakeup via TOUCHPAD signal");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("wakeup via ULP signal");
    break;
  case ESP_SLEEP_WAKEUP_UNDEFINED: // 不是唤醒 正常执行
  default:
    Serial.printf("wakeup via other: %d\n", wakeup_case);
  }
}

int initWakeup()
{
  int ret = 0;
  Serial.printf("enable timer wakeup, %ds\n", TIME_TO_SLEEP);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  // 使能外部中断唤醒
  Serial.printf("Enabling EXT1 wakeup on pins GPIO%d, GPIO%d\n", ext_wakeup_pin_1, ext_wakeup_pin_2);
  esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask | ext_wakeup_pin_2_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
  // Isolate GPIO12 pin from external circuits. This is needed for modules
  // which have an external pull-up resistor on GPIO12 (such as ESP32-WROVER)
  // to minimize current consumption.
  rtc_gpio_isolate(GPIO_NUM_12); // 将12脚隔离
  return ret;
}

void startSleep()
{
  int ret = 0;
  struct timeval sleep_time;
  // 获取进入睡眠的时间并保存
  gettimeofday(&sleep_time, NULL);

  nvs_handle_t my_handler;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handler);
  if (err == ESP_OK)
  {
    err = nvs_set_u64(my_handler, "sec", sleep_time.tv_sec);
    err = nvs_set_u64(my_handler, "usec", sleep_time.tv_usec);
    if (err != ESP_OK)
    {
      Serial.printf("Error %s Writing.\n", esp_err_to_name(err));
    }
    else
    {
      nvs_commit(my_handler);
    }
    nvs_close(my_handler);
  }

  // 开始深度休眠
  Serial.println("Entering deep sleep");
  esp_deep_sleep_start();
}

int calcDuration()
{
  struct timeval now, sleep_time;
  int duration_time_ms;
  gettimeofday(&now, NULL); //获取当前时间，格式为s和us

  // 取之前的时间，计算休眠时长
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  nvs_handle_t my_handler;
  err = nvs_open("storage", NVS_READONLY, &my_handler);
  if (err != ESP_OK)
  {
    Serial.printf("Error %s open NVS handler\n", esp_err_to_name(err));
  }
  else
  {
    err = nvs_get_u64(my_handler, "sec", (uint64_t *)(&sleep_time.tv_sec));
    err = nvs_get_u64(my_handler, "usec", (uint64_t *)(&sleep_time.tv_usec));
    nvs_close(my_handler);
    switch (err)
    {
    case ESP_OK:
      Serial.printf("read ok\n");
      // 当前时间减去进入睡眠时间就是睡眠所消耗的时间，乘以1000转换成ms
      duration_time_ms = (now.tv_sec - sleep_time.tv_sec) * 1000 + (now.tv_usec - sleep_time.tv_usec) / 1000;
      Serial.printf("time elapsed %ms\n", duration_time_ms);
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      Serial.printf("not found\n");
      break;
    default:
      Serial.printf("Error %s reading.\n", esp_err_to_name(err));
    }
  }

  return duration_time_ms;
}

void syncTime()
{
  // TODO
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  if (bootCount == 0)
  {
    Serial.println("first time to boot");
    syncTime();
  }
  else
  {
    calcDuration();
    Serial.printf("boot count %d\n", bootCount);
    printWakeupReason();
  }

  bootCount++;

  if (bootCount == 1)
  {
    initCameraServer();

    Serial.print("WiFi Connecting ...\n");
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
    startCameraServer();
  }
  else
  {
    initWakeup();
    startSleep();
  }

  // 不会执行到这里
  pinMode(LED_GPIO, OUTPUT);
}

void loop()
{
  // put your main code here, to run repeatedly:
}