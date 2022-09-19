#ifndef _COMM_H_
#define _COMM_H_
#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <driver/rtc_io.h>
#include <sys/time.h>
#include <FS.h>
#include <SD_MMC.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include <driver/rtc_io.h>
#include "lwip/apps/sntp.h"
#include <time.h>
#include <EEPROM.h>
#include "private_info.h"

// define the number of bytes you want to access
#define EEPROM_SIZE 1
// led flash on broad
#define LED_GPIO 33

// 休眠唤醒相关
#define TIME_TO_SLEEP 60         // Time ESP32 will go to sleep (in seconds)
#define uS_TO_S_FACTOR 1000000   // Conversion factor for micro seconds to seconds
const int ext_wakeup_pin_1 = 12; // 唤醒外部中断1
const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
const int ext_wakeup_pin_2 = 13; // 唤醒外部中断2
const uint64_t ext_wakeup_pin_2_mask = 1ULL << ext_wakeup_pin_2;


int connectWiFi(const char *ssid, const char *password);
void disconnectWiFi();
void syncTime(void);
void initSd();
void setSensor(sensor_t *s);
void pic2tf(int imageNum);
int printWakeupReason();
int calcDuration();
int initWakeup();
void startSleep();
void IRAM_ATTR isrRestart();

#endif