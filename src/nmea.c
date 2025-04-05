#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include "nmea.h"

void nmea_parse_gpgga(const char *nmea, GNSS_Data *data)
{
    char *token; 
    char copy[NMEA_MAX_LEN];
    SAFE_STRNCPY(copy, nmea, sizeof(copy));

    token = strtok(copy, ",");
    int field = 0;
    
    while (token != NULL) 
    {
        switch (field) 
        {
            case 1:  // Timestamp (HHMMSS.SSS)
                SAFE_STRNCPY(data->timestamp, token, sizeof(data->timestamp));
                break;
            case 2:  // Latitude (DDMM.MMMM)
                data->latitude = atof(token) / 100.0;
                break;
            case 3:  // Latitude direction (N/S)
                if (*token == 'S') data->latitude *= -1;
                break;
            case 4:  // Longitude (DDDMM.MMMM)
                data->longitude = atof(token) / 100.0;
                break;
            case 5:  // Longitude direction (E/W)
                if (*token == 'W') data->longitude *= -1;
                break;
            case 7:  // Satellites in use
                data->satellites = atoi(token);
                break;
            case 9:  // Altitude (meters)
                data->altitude = atof(token);
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}

void nmea_parse_gprmc(const char *nmea, GNSS_Data *data)
{
    char *token;
    char copy[NMEA_MAX_LEN];
    SAFE_STRNCPY(copy, nmea, sizeof(copy));

    token = strtok(copy, ",");
    int field = 0;
    
    while (token != NULL) 
    {
        switch (field) 
        {
            case 1:  // Timestamp (HHMMSS.SSS)
                SAFE_STRNCPY(data->timestamp, token, sizeof(data->timestamp));
                break;
            case 2:  // Status (A=active, V=void)
                //data->status = *token;
                break;
            case 3:  // Latitude (DDMM.MMMM)
                data->latitude = atof(token) / 100.0;
                break;
            case 4:  // Latitude direction (N/S)
                if (*token == 'S') data->latitude *= -1;
                break;
            case 5:  // Longitude (DDDMM.MMMM)
                data->longitude = atof(token) / 100.0;
                break;
            case 6:  // Longitude direction (E/W)
                if (*token == 'W') data->longitude *= -1;
                break;
            case 7:  // Speed (knots)
                // Can be parsed if needed
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}

void nmea_parse_gpgsa(const char *nmea, GNSS_Data *data) 
{
    char *token;
    char copy[NMEA_MAX_LEN];
    SAFE_STRNCPY(copy, nmea, sizeof(copy));

    token = strtok(copy, ",");
    int field = 0;
    
    while (token != NULL) 
    {
        switch (field) 
        {
            case 2:  // Fix Type (1=No fix, 2=2D, 3=3D)
                data->fix_quality = atoi(token);
                break;
            case 15: // PDOP
                data->pdop = atof(token);
                break;
            case 16: // HDOP
                data->hdop = atof(token);
                break;
            case 17: // VDOP
                data->vdop = atof(token);
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}

void nmea_parse_gpgsv(const char *nmea, GNSS_Data *data) 
{
    char *token;
    char copy[NMEA_MAX_LEN];
    SAFE_STRNCPY(copy, nmea, sizeof(copy));

    token = strtok(copy, ",");
    int field = 0;
    int sat_index = 0;
    
    while (token != NULL) 
    {
        switch (field) 
        {
            case 3:  // Total Satellites in View
                data->total_sats_in_view = atoi(token);
                break;
            case 4:  // Satellite PRN (first of up to 4 per message)
            case 8:  // Second satellite
            case 12: // Third satellite
            case 16: // Fourth satellite
                if (sat_index < 24) 
                {
                    data->sat_info[sat_index].prn = atoi(token);
                    sat_index++;
                }
                break;
            case 5:  // Elevation (first satellite)
            case 9:  // Second satellite
            case 13: // Third satellite
            case 17: // Fourth satellite
                if (sat_index > 0) 
                {
                    data->sat_info[sat_index-1].elevation = atoi(token);
                }
                break;
            case 6:  // Azimuth (first satellite)
            case 10: // Second satellite
            case 14: // Third satellite
            case 18: // Fourth satellite
                if (sat_index > 0) 
                {
                    data->sat_info[sat_index-1].azimuth = atoi(token);
                }
                break;
            case 7:  // SNR (first satellite)
            case 11: // Second satellite
            case 15: // Third satellite
            case 19: // Fourth satellite
                if (sat_index > 0) 
                {
                    data->sat_info[sat_index-1].snr = atoi(token);
                }
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}

void nmea_parse_gpvtg(const char *nmea, GNSS_Data *data) 
{
    char *token;
    char copy[NMEA_MAX_LEN];
    SAFE_STRNCPY(copy, nmea, sizeof(copy));

    token = strtok(copy, ",");
    int field = 0;
    
    while (token != NULL) 
    {
        switch (field) 
        {
            case 1:  // True Course (degrees)
                data->course = atof(token);
                break;
            case 7:  // Speed (km/h)
                data->speed = atof(token);
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}

void nmea_parse_gpgll(const char *nmea, GNSS_Data *data) 
{
    char *token;
    char copy[NMEA_MAX_LEN];
    SAFE_STRNCPY(copy, nmea, sizeof(copy));

    token = strtok(copy, ",");
    int field = 0;
    
    while (token != NULL) 
    {
        switch (field) 
        {
            case 1:  // Latitude (DDMM.MMMM)
                data->latitude = atof(token) / 100.0;
                break;
            case 2:  // N/S
                if (*token == 'S') data->latitude *= -1;
                break;
            case 3:  // Longitude (DDDMM.MMMM)
                data->longitude = atof(token) / 100.0;
                break;
            case 4:  // E/W
                if (*token == 'W') data->longitude *= -1;
                break;
            case 5:  // UTC Time (HHMMSS.SSS)
                SAFE_STRNCPY(data->timestamp, token, sizeof(data->timestamp));
                break;
            case 6:  // Status (A=Valid, V=Invalid)
                data->fix_quality = (*token == 'A') ? 1 : 0;
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}

NMEA_MessageType nmea_get_message_type(const char *message) 
{
    // Validate checksum first
    uint8_t checksum_status = nmea_valid_checksum(message);
    if (checksum_status == NMEA_CHECKSUM_ERROR) 
    {
        return NMEA_CHECKSUM_ERROR;
    }

    // Detect message type
    if (strstr(message, NMEA_GPGGA_WORD) != NULL) return NMEA_GPGGA;
    if (strstr(message, NMEA_GPRMC_WORD) != NULL) return NMEA_GPRMC;
    if (strstr(message, NMEA_GPVTG_WORD) != NULL) return NMEA_GPVTG;
    if (strstr(message, NMEA_GPGSA_WORD) != NULL) return NMEA_GPGSA;
    if (strstr(message, NMEA_GPGSV_WORD) != NULL) return NMEA_GPGSV;
    if (strstr(message, NMEA_GPGLL_WORD) != NULL) return NMEA_GPGLL;
    if (strstr(message, NMEA_GPZDA_WORD) != NULL) return NMEA_GPZDA;
    if (strstr(message, NMEA_GPGST_WORD) != NULL) return NMEA_GPGST;
    if (strstr(message, NMEA_GPGNS_WORD) != NULL) return NMEA_GPGNS;
    if (strstr(message, NMEA_PQVERNO_WORD) != NULL) return NMEA_PQVERNO;

    return NMEA_UNKNOWN;
}

uint8_t nmea_valid_checksum(const char *message) 
{
    uint8_t checksum= (uint8_t)strtol(strchr(message, '*')+1, NULL, 16);

    char p;
    uint8_t sum = 0;
    ++message;
    while ((p = *message++) != '*') 
    {
        sum ^= p;
    }

    if (sum != checksum) 
    {
        return NMEA_CHECKSUM_ERROR;
    }

    return _EMPTY;
}

void nmea_enable_pps_sync(void)
{
	//uint8_t buffer[] = NMEA_ENABLE_PPS_SYNC;
	//send_nmea_message(buffer);
}

void nmea_hot_restart(void)
{
	//uint8_t buffer[] = NMEA_HOT_RST_CMD;
	//send_nmea_message(buffer);
}

void nmea_factory_reset(void)
{
	//uint8_t buffer[] = NMEA_FCOLD_RST_CMD;
	//send_nmea_message(buffer);
}

void nmea_standby(void)
{
	//uint8_t buffer[] = NMEA_SET_STDBY_CMD;
	//send_nmea_message(buffer);
}