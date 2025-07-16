#ifndef EXIT_H
#define EXIT_H

#include "im1281b_home_power.h"

extern volatile sig_atomic_t keep_running;

void signal_handler(int signum);
int check_keep_file();

#endif