/* 
*
*
*
*/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "o_sntp.h"
#include "esp_task_wdt.h"
#include "sdkconfig.h"


static const char *TAG = "main ";


void app_main(void)
{  
   struct timeval tv_now ;



   while (1)
   { 

       vTaskDelay( 20 / portTICK_PERIOD_MS);
      
   }
}



