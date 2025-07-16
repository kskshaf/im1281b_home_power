#ifndef IM1281B_HOME_POWER_H
#define IM1281B_HOME_POWER_H

#include <libserialport.h>
#include "log.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <locale.h>

#define Timeout   1000
#define BaudRate  4800

#define DATA_FILE  "/tmp/im1281b_data"
#define KEEP_FILE  "/tmp/im1281b_keep"
#define LOG_FILE   "/var/log/im1281b/im1281b.log" // 需要 mkdir
#define WEB_FILE   "/usr/share/test-web/index.html"

#define Voltage_POF   120
#define Voltage_LOW   190
#define Voltage_HIGH  260

#define Power_HIGH    2000
#define Temp_HIGH     45

#endif
