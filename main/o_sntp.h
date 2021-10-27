/**
 * @file o_sntp.h
 * @author Manel AISSAOUI 
 * @version 0.1
 * @date 2021-07-05
 * 
 * @copyright Copyright (c) 2021 Manel Aissaoui
 * 
 */

#ifndef _O_SNTP_H_
#define _O_SNTP_H_

#include <string.h>
#include <sys/types.h>
#include <sys/timespec.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define ERROR_TOLERANCE         500000  // in  microseconds


typedef enum {
    eSYNC_STATUS_RESET,         // Reset status.
    eSYNC_STATUS_COMPLETED,     // Time is synchronized.
    eSYNC_STATUS_IN_PROGRESS,   // Smooth time sync in progress.
} eO_SNTP_SyncStatus_TypeDef;


/**
 * @brief O_SNTP context struct
 * 
 */
typedef struct{

    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;           // after 2000

    //module states  

}stO_SNTP_ContextTypeDef;

extern stO_SNTP_ContextTypeDef stO_SNTP_Context;

/**
 * @brief Initialize SNTP
 * 
 * @return esp_err_t 
 */
esp_err_t xO_SNTP_InitTimer(void);

/**
 * @brief This function updates the system time.
 *          SET the time, if date is incoherent it sets the future closest coherent date 
 * 
 * @param second 
 * @param minute 
 * @param hour 
 * @param day 
 * @param month 
 * @param year 
 * @return esp_err_t 
 */
esp_err_t xO_SNTP_SetTime(uint8_t second, uint8_t minute, uint8_t hour, uint8_t day, uint8_t month, uint8_t year);

/**
 * @brief Updates the SNTP time context
 * 
 */
void xO_SNTP_ReadTime();

/**
 * @brief Create Timer and assign a callback to it
 * 
 * @param callback 
 * @param timer 
 */
void xO_SNTP_CreateTimer( void * callback, esp_timer_handle_t* timer );

/**
 * @brief Start periodic alarm
 * 
 * @param periodic_timer        Timer handler
 * @param timestamp             in microsecond
 */
void xO_SNTP_SetPeriodicAlarm( esp_timer_handle_t periodic_timer, uint64_t timestamp );

/**
 * @brief Start one shot alarm
 * 
 * @param oneshot_timer         Timer handler
 * @param timestamp             in microsecond
 */
void xO_SNTP_SetSingleShotAlarm( esp_timer_handle_t oneshot_timer, uint64_t timestamp );

/**
 * @brief Stop Timer
 * 
 * @param timer                 Timer handler
 */
void xO_SNTP_StopTimer( esp_timer_handle_t timer);

/**
 * @brief Delete Timer
 * 
 * @param timer                 Timer handler
 */
void xO_SNTP_DeleteTimer( esp_timer_handle_t timer);




#endif /* _O_SNTP_H_ */

