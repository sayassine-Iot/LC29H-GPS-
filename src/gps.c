#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gps.h"
#include "nmea.h"

// Compute the GPS location using decimal scale
void gps_location(loc_t *coord) 
{
    
    gpgga_t gpgga;
    gprmc_t gprmc;
    char buffer[256];
    uint8_t msgtype;
        
    // Clear buffer before reading new data
    memset(buffer, 0, sizeof(buffer));  
        
    // Read data from the GPS module
    nmea_read_response(buffer, sizeof(buffer));

    // Process the received NMEA message
    msgtype = nmea_get_message_type(buffer);
        
    switch (msgtype) 
    {
        case NMEA_GPGGA:
        case NMEA_GNGGA:  // Handle GNSS GGA as well
            nmea_parse_gpgga(buffer, &gpgga);

            gps_convert_deg_to_dec(&(gpgga.latitude), gpgga.lat, &(gpgga.longitude), gpgga.lon);

            coord->latitude = gpgga.latitude;
            coord->longitude = gpgga.longitude;
            coord->altitude = gpgga.altitude;
            break;
        case NMEA_GPRMC:
        case NMEA_GNRMC:  // Handle GNSS RMC as well
            nmea_parse_gprmc(buffer, &gprmc);

            coord->speed = gprmc.speed;
            coord->course = gprmc.course;
            break;
    
        default:
           break;    
    }
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

