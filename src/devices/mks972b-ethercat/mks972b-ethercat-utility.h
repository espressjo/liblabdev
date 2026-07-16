#ifndef MKS972B_UTILITY_H
#define MKS972B_UTILITY_H

#include <soem/soem.h>

#include <chrono>
#include <csignal>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

std::string decode_exception(ecx_contextt *ctx, uint8_t status);
uint8_t read_exception_status(ecx_contextt *ctx, int slave_idx);
bool read_reading_valid(ecx_contextt *ctx, int slave_idx);
uint16_t read_sensor_number(ecx_contextt *ctx, int slave_idx);
float e_read_pressure(ecx_contextt *ctx, int slave_idx);

#endif
