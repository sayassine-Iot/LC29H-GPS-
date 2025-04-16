#ifndef _GPS_H_
#define _GPS_H_

// Initialize device
extern void gps_init(void);
// Activate device
extern void gps_on(void);
// Turn off device (low-power consumption)
extern void gps_off(void);

// -------------------------------------------------------------------------
// Internal functions
// -------------------------------------------------------------------------
void timestamp_to_datetime(int timestamp, int *year, int *month, int *day, int *hour, int *minute, int *second);

// convert deg to decimal deg latitude, (N/S), longitude, (W/E)
void gps_convert_deg_to_dec(double *, char, double *, char);
double gps_deg_dec(double);

#endif
