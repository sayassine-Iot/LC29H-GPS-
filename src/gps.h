#ifndef _GPS_H_
#define _GPS_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

// Initialize device
extern void gps_init(void);
// Activate device
extern void gps_on(void);
// Turn off device (low-power consumption)
extern void gps_off(void);

void timestamp_to_datetime(uint32_t timestamp, uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute, uint8_t *second);

// convert deg to decimal deg latitude, (N/S), longitude, (W/E)
void gps_convert_deg_to_dec(double *, char, double *, char);
double gps_deg_dec(double);

#endif
