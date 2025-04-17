#include <string.h>
#include <math.h>
#include "gps.h"


// Number of days in each month for a non-leap year
const int days_in_month[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// Check if a year is a leap year
int is_leap_year(int year) 
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

void timestamp_to_datetime(uint32_t timestamp, uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute, uint8_t *second) 
{
    // Unix time starts at 1970-01-01 00:00:00 UTC
    int y = 1970;
    int days;

    // Extract time components from timestamp
    *second = timestamp % 60;
    timestamp /= 60;
    *minute = timestamp % 60;
    timestamp /= 60;
    *hour = timestamp % 24;
    timestamp /= 24;
    days = timestamp;  // total days since 1970-01-01

    // Calculate year
    while (1) 
    {
        int days_in_year = is_leap_year(y) ? 366 : 365;
        if (days >= days_in_year) 
        {
            days -= days_in_year;
            y++;
        } 
        else 
        {
            break;
        }
    }
    *year = y;

    // Calculate month
    int m = 0;
    while (1) 
    {
        int dim = days_in_month[m];
        if (m == 1 && is_leap_year(y)) 
        { // February in leap year
            dim = 29;
        }

        if (days >= dim) 
        {
            days -= dim;
            m++;
        } 
        else 
        {
            break;
        }
    }
    *month = m + 1;  // month is 1-based

    // Remaining days is the day of the month
    *day = days + 1; // day is 1-based
}

// Convert lat e lon to decimals (from deg)
void gps_convert_deg_to_dec(double *latitude, char ns,  double *longitude, char we)
{
    double lat = (ns == 'N') ? *latitude : -1 * (*latitude);
    double lon = (we == 'E') ? *longitude : -1 * (*longitude);

    *latitude = gps_deg_dec(lat);
    *longitude = gps_deg_dec(lon);
}

double gps_deg_dec(double deg_point)
{
    double ddeg;
    double sec = modf(deg_point, &ddeg)*60;
    int deg = (int)(ddeg/100);
    int min = (int)(deg_point-(deg*100));

    double absdlat = round(deg * 1000000.);
    double absmlat = round(min * 1000000.);
    double absslat = round(sec * 1000000.);

    return round(absdlat + (absmlat/60) + (absslat/3600)) /1000000;
}

