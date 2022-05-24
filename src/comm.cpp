#include "comm.h"

int connectWiFi(const char *ssid, const char *password)
{
  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);
  Serial.printf("connecting to WIFI %s\n", ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(800);
  }
  Serial.print("WIFI connected. MAC: ");
  Serial.printf("%s", WiFi.macAddress().c_str());
  Serial.printf(" IP: ");
  Serial.println(WiFi.localIP());
  return 0;
}

void disconnectWiFi()
{
  Serial.println("disconnect WiFi");
  WiFi.disconnect();
}

void syncTime(void)
{
  Serial.println("Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "ntp1.aliyun.com");
  sntp_setservername(1, "1.cn.pool.ntp.org");
  sntp_init();

  // wait for time to be set
  time_t now = 0;
  struct tm timeinfo = {0};
  int retry = 0;
  setenv("TZ", "CST-8", 1);
  tzset();
  Serial.printf("waiting for system time to be set...\n");
  while (timeinfo.tm_year < (2019 - 1900))
  {
//    Serial.printf("Waiting for system time to be set... (%d)\n", ++retry);
    vTaskDelay(300 / portTICK_PERIOD_MS);
    time(&now);
    localtime_r(&now, &timeinfo);
  }
  Serial.printf("system time set.\n");

  time_t t;
  t = time(NULL);
  // Serial.printf("seconds since January 1, 1970 is %ld\n", t);

  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  Serial.printf("current date/time in Beijing is: %s\n", strftime_buf);
}


void setSensor(sensor_t *s)
{
  s->set_brightness(s, 1);
  s->set_contrast(s, 1);
  s->set_saturation(s, 2);
}

void initSd()
{
  // if(!SD_MMC.begin()){
  if (!SD_MMC.begin("/sdcard", true))
  { // set mode1bit=true, to prevent gpio4 flash light from glowing
    // in mode1bit, only GPIO 2(HS_DATA0) is used, GPIO 4(HS_DATA1)/12(HS_DATA2)/13(HS_DATA3) is free for other use
    Serial.println("SD Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD Card attached");
    return;
  }

  Serial.println("init sd done.");

  /*
  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  }
  else if(cardType == CARD_SD){  Serial.println("SDSC");  }
  else if(cardType == CARD_SDHC){  Serial.println("SDHC");  }
  else {  Serial.println("UNKNOWN");  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD cap: %lluMB\n", cardSize);
  */
}

void pic2tf(int imageNum)
{
  camera_fb_t *fb = NULL;

  // Capture picture
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera Failed to Capture");
    return;
  }
  // initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  imageNum = EEPROM.read(0) + 1;

  // get time
  time_t now = 0;
  struct tm timeinfo = {0};
  setenv("TZ", "CST-8", 1);
  tzset();
  time(&now);
  localtime_r(&now, &timeinfo);

  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%Y%m%d_%H%M%S_", &timeinfo);
  Serial.printf("strftime_buf: %s\n", strftime_buf);
  String path = "/p" + String(strftime_buf) + String(imageNum) + ".jpg";
  Serial.printf("Picture file name: %s\n", path.c_str());

  fs::FS &fs = SD_MMC;

  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file in writing mode");
  }
  else
  {
    file.write(fb->buf, fb->len);
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.write(0, imageNum);
    EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb);
}

// =0:timer wake up, >0:pin_num wake up
int printWakeupReason()
{
  int ret = -1;
  esp_sleep_wakeup_cause_t wakeup_case = esp_sleep_get_wakeup_cause();
  switch (wakeup_case) // 判断唤醒原因
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("wakeup via RTC_IO signal");
    break;
  case ESP_SLEEP_WAKEUP_EXT1: // 外部中断1 唤醒
  {
    uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_pin_mask != 0)
    {
      Serial.println((log(wakeup_pin_mask)) / log(2), 0);  // debug only
      ret = __builtin_ffsll(wakeup_pin_mask) - 1;
      Serial.printf("wake up from RTC_CNTL GPIO %d\n", ret);
    }
    else
    {
      Serial.printf("wake up from other source.\n");
    }
    break;
  }
  case ESP_SLEEP_WAKEUP_TIMER: // 定时器唤醒
  {
    // printf("Wake up from timer. Time spent in deep sleep: %dmsn", sleep_time_ms);
    Serial.println("wake up from timer.");
    ret = 0;
    break;
  }
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("wakeup via TOUCHPAD signal");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("wakeup via ULP signal");
    break;
  case ESP_SLEEP_WAKEUP_UNDEFINED: // 不是唤醒 正常执行
    Serial.println("wakeup via undefined reason");
    break;
  default:
    Serial.printf("wakeup via other: %d\n", wakeup_case);
    break;
  }

  return ret;
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
      // Serial.printf("read ok\n");
      // 当前时间减去进入睡眠时间就是睡眠所消耗的时间，乘以1000转换成ms
      duration_time_ms = (now.tv_sec - sleep_time.tv_sec) * 1000 + (now.tv_usec - sleep_time.tv_usec) / 1000;
      Serial.printf("time elapsed %d ms\n", duration_time_ms);
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      Serial.printf("not found\n");
      break;
    default:
      Serial.printf("Error reading. %s\n", esp_err_to_name(err));
    }
  }

  return duration_time_ms;
}


int initWakeup()
{
  int ret = 0;
  Serial.printf("enable timer wakeup, %ds\n", TIME_TO_SLEEP);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // pinMode(GPIO_NUM_4, OUTPUT);
  // digitalWrite(GPIO_NUM_4, LOW);
  // rtc_gpio_hold_en(GPIO_NUM_4);

/*
  // 使能外部中断唤醒
  pinMode(ext_wakeup_pin_1, INPUT);
  pinMode(ext_wakeup_pin_2, INPUT);
  Serial.printf("enabling EXT1 wakeup on pins GPIO%d, GPIO%d\n", ext_wakeup_pin_1, ext_wakeup_pin_2);
  esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask | ext_wakeup_pin_2_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
*/

  // pinMode(GPIO_NUM_13, INPUT);
  // Serial.printf("enabling EXT0 wakeup on pins %d\n", GPIO_NUM_13);
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 1);

  // Isolate GPIO12 pin from external circuits. This is needed for modules
  // which have an external pull-up resistor on GPIO12 (such as ESP32-WROVER)
  // to minimize current consumption.
  // rtc_gpio_isolate(GPIO_NUM_12); // 将12脚隔离

  delay(1000);
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
  Serial.println("entering deep sleep");
  Serial.flush();
  esp_deep_sleep_start();
  Serial.println("This will never be printed !!!!");
}
