/**
 * @file o_sntp.c
 * @author Manel AISSAOUI 
 * @brief Time module
 * 
 * RTC module using TG0 LAC timer of the ESP32.
 * 
 * This timer is a 64-bit up-counting high resolution timer with a programmable compare value
 * when the timer reaches this value, an alarm is fired.
 * This timer uses the APB clock source (typically 80 MHz) which is derived from CPU_CLOCK,Time will be measured at 1us resolution.
 * In this implementation the timer is used for two purposes.
 * 1.	Perform timekeeping once time is synchronized
 * 
 * 2.	Generate periodic or single shot alarms.
 * 
 * Please Note that this timer is not available during any reset and sleep modes
 * As an alternative you can use RTC to keep time measurement during sleep modes
 * Setting the time takes place in the xO_RTC_SyncTime , time synchronization will use the adjtime function
 * to smoothly update the time, this function will speed up or slow down the system clock in order to make a
 * gradual adjustement.This will ensure that the calendar time reported by the system clock is always monotonically 
 * increasing.
 * If the time error is greater than 35 minutes, adjtime will fail and you need to update the time using settimeofday().
 * To receive notification of time synchronization you can use the callback function or get the synchronization status by calling the function xO_RTC_SyncJob ().
 * After the synchronization is completed, the synchronization status will be eSYNC_STATUS_COMPLETED then it will be updated to eSYNC_STATUS_RESET.
 * Current time can be obtained by calling the function xO_RTC_ReadTime()
 * this implementation sets up the timer interrupt to fire when the timestamp set by the user is reached, see xO_RTC_SetAlarm 
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html
 *
 * @version 0.1
 * @date 2021-07-05
 * 
 * @copyright Copyright (c) Manel Aissaoui
 * 
 */


#include "o_sntp.h"

static const char *TAG = "O_SNTP";
static eO_SNTP_SyncStatus_TypeDef xO_SNTP_Sync(void);
stO_SNTP_ContextTypeDef stO_SNTP_Context;
static volatile eO_SNTP_SyncStatus_TypeDef time_sync_status = eSYNC_STATUS_RESET;


esp_err_t xO_SNTP_InitTimer(void)
{  
  ESP_LOGI(TAG, "Initializing Timer");

  time_sync_status = eSYNC_STATUS_RESET; 
   
  esp_err_t ret = esp_timer_init();

  if(ret == ESP_OK)
  {
    ESP_LOGI(TAG, "timer is initialized and Time will be synchronized from custom code");
  }
  else 
  {
    ESP_LOGI(TAG, "timer is already initialized");
  }

  ret = xO_SNTP_SetTime(0, 0, 0, 1, 1, 0);
  return ret ;
}

esp_err_t xO_SNTP_SetTime(uint8_t second, uint8_t minute, uint8_t hour, uint8_t day, uint8_t month, uint8_t year)
{
  /* don't foreget to assert params */ 
  if((second>=60)&&(minute>=60)&&(hour>=24)&&(day>31)&&(month>12))
  {
    ESP_LOGE(TAG, "xO_SNTP_SetTime Failed when assert params");
    return ESP_FAIL;
  }

  struct tm time_now = {
  .tm_hour = hour,
  .tm_min  = minute,
  .tm_sec  = second,
  .tm_mday = day,
  .tm_mon  = month-1,              // from 0 to 11  (month-1)
  .tm_year = 100 + year,  //2000 + year - 1900,   //current-1900
  }; 

  time_t t = mktime (&time_now);
  struct timeval tv = { .tv_sec = t , .tv_usec = 0 };
  if(settimeofday(&tv, NULL) < 0)
  {
    ESP_LOGE(TAG, "xO_SNTP_SetTime Failed when setting timeofday");
    return ESP_FAIL;
  } 

  return ESP_OK;
}

void xO_SNTP_ReadTime()
{   
  char strftime_buf[64];
  struct tm  current_time ;
  time_t now = time(NULL);          // now is date&time in seconds
  /*set time zone*/
  setenv("TZ", "UTC", 1);
  tzset();
  localtime_r(&now, &current_time);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &current_time);
  ESP_LOGI(TAG, "%s", strftime_buf);  

  stO_SNTP_Context.second   = current_time.tm_sec;
  stO_SNTP_Context.minute   = current_time.tm_min;
  stO_SNTP_Context.hour     = current_time.tm_hour;
  stO_SNTP_Context.day      = current_time.tm_mday;
  stO_SNTP_Context.month    = current_time.tm_mon+1;
  stO_SNTP_Context.year     = current_time.tm_year-100;     // +1900-2000
}


/* lazem CREATE timer 9bal ma taamel SET */
void xO_SNTP_CreateTimer( void * callback, esp_timer_handle_t* timer )
{
  const esp_timer_create_args_t timer_args = 
  {
    .callback = callback,
    //.name = "oneshot-timer"
  };
    
  ESP_ERROR_CHECK(esp_timer_create(&timer_args,timer));
}

void xO_SNTP_SetPeriodicAlarm( esp_timer_handle_t periodic_timer, uint64_t timestamp )
{
  xO_SNTP_StopTimer(periodic_timer);    // Maher: timer should not be running when setting alarm (documentation)
  esp_timer_start_periodic( periodic_timer, timestamp );
}

void  xO_SNTP_SetSingleShotAlarm( esp_timer_handle_t oneshot_timer, uint64_t timestamp )
{
  xO_SNTP_StopTimer(oneshot_timer);    // Maher: timer should not be running when setting alarm (documentation)
  esp_timer_start_once( oneshot_timer,timestamp );
}

void xO_SNTP_StopTimer( esp_timer_handle_t timer)
{
  esp_timer_stop( timer );
}

void xO_SNTP_DeleteTimer( esp_timer_handle_t timer)
{
  xO_SNTP_StopTimer( timer );
  esp_timer_delete( timer );
}

static eO_SNTP_SyncStatus_TypeDef xO_SNTP_Sync(void)
{
  eO_SNTP_SyncStatus_TypeDef eO_RTC_SyncStatus = time_sync_status;

  if (eO_RTC_SyncStatus == eSYNC_STATUS_IN_PROGRESS) 
  {
    struct timeval outdelta;
    adjtime(NULL, &outdelta);
    int64_t out = (int64_t)outdelta.tv_sec * 1000000L + (int64_t)outdelta.tv_usec;
    ESP_LOGI(TAG, "out is :%lld", out);
    if (outdelta.tv_sec == 0 && outdelta.tv_usec == 0) 
    {
      time_sync_status = eSYNC_STATUS_COMPLETED;           
      ESP_LOGI(TAG, "TIME IS SYNCHRONIZED");
    } 
    else 
    {
      ESP_LOGI(TAG, "ADJUSTEMENT IN PROGRESS");
    }
  }
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  return time_sync_status;
  
}

eO_SNTP_SyncStatus_TypeDef xO_SNTP_SyncTime( struct timeval *tv ) 
{ 
  struct timeval tv_now;     //, outdelta;
  gettimeofday(&tv_now,NULL); 
  int64_t system_time = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
  int64_t current_time = (int64_t)tv->tv_sec * 1000000L + (int64_t)tv->tv_usec;
  int64_t delta = current_time - system_time ;
  ESP_LOGI(TAG, "delta is :%lld", delta);
  if (abs(delta) <= ERROR_TOLERANCE)
  {
    ESP_LOGI(TAG, "Time is already synchronized");
  }
  else
  {
    struct timeval tv_delta = { .tv_sec = delta / 1000000L, .tv_usec = delta % 1000000L };
    if ( adjtime(&tv_delta, NULL) == -1 )
    {
      ESP_LOGI(TAG, "Function adjtime can not update time because the error is very big (greater than 35 minutes");
      settimeofday(tv,NULL);
      ESP_LOGI(TAG, "time was synchronized through settimeofday() ");
      time_sync_status = eSYNC_STATUS_COMPLETED;  
    }
    else 
    {  
      time_sync_status = eSYNC_STATUS_IN_PROGRESS; 
    }
  
    while (time_sync_status == eSYNC_STATUS_IN_PROGRESS)
    {
      xO_SNTP_Sync();
    }
  }
  return time_sync_status;
}