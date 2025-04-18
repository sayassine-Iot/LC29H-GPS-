#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <math.h>
#include "nmea.h"

GNSS_Data *gnss_data = NULL;
TimeStruct UTC_time = {0};

LOG_MODULE_REGISTER(nmea, CONFIG_LOG_DEFAULT_LEVEL);

void nmea_init(void)
{
    gnss_data = malloc(sizeof(GNSS_Data));
    if (gnss_data != NULL) 
    {
        memset(gnss_data, 0, sizeof(GNSS_Data));
    }
}

// Helper function: Convert NMEA (DDMM.MMMM or DDDMM.MMMM) to decimal degrees
static double nmea_to_decimal(double nmea_coord) 
{
    int degrees = (int)(nmea_coord / 100);
    double minutes = nmea_coord - (degrees * 100);
    return degrees + (minutes / 60.0);
}

TimeStruct nmea_parse_time(const char* time_str)
{
    TimeStruct time = {0};
    time.valid = false;

    if ((time_str == NULL) || (strlen(time_str) < 6))
    {
        return time;  // Invalid time format
    }

    // Example time format: "123456.78" (hhmmss.sss)
    char buffer[3] = {0};
    
    // Extract hours (first 2 digits)
    strncpy(buffer, time_str, 2);
    time.hours = atoi(buffer);
    
    // Extract minutes (next 2 digits)
    strncpy(buffer, time_str + 2, 2);
    time.minutes = atoi(buffer);
    
    // Extract seconds (next 2 digits)
    strncpy(buffer, time_str + 4, 2);
    time.seconds = atoi(buffer);
    
    // Find decimal point for hundredths
    const char* decimal_ptr = strchr(time_str, '.');
    if ((decimal_ptr != NULL) && (strlen(decimal_ptr) > 1))
    {
        // Take up to 2 digits after decimal
        strncpy(buffer, decimal_ptr + 1, 2);
        time.millis = atoi(buffer);
    }
    
    // Validate ranges
    if (time.hours < 24 && time.minutes < 60 && time.seconds < 60) 
    {
        time.valid = true;
    }

    // Combine into uint32_t timestamp (hhmmsshh format)
    return time;
}

void nmea_parse_gpgga(const char *nmea)
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
                UTC_time = nmea_parse_time(token);
                break;
            case 2:  // Latitude (DDMM.MMMM)
                gnss_data->latitude = nmea_to_decimal(atof(token));
                break;
            case 3:  // Latitude direction (N/S)
                if (*token == 'S') gnss_data->latitude *= -1;
                break;
            case 4:  // Longitude (DDDMM.MMMM)
                gnss_data->longitude = nmea_to_decimal(atof(token));
                break;
            case 5:  // Longitude direction (E/W)
                if (*token == 'W') gnss_data->longitude *= -1;
                break;
            case 7:  // Satellites in use
                gnss_data->satellites = atoi(token);
                break;
            case 9:  // Altitude (meters)
                gnss_data->altitude = atof(token);
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}

void nmea_parse_gprmc(const char *nmea)
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
                UTC_time = nmea_parse_time(token);
                break;
            case 2:  // Status (A=active, V=void)
                //gnss_data->status = *token;
                break;
            case 3:  // Latitude (DDMM.MMMM)
                gnss_data->latitude = nmea_to_decimal(atof(token));
                break;
            case 4:  // Latitude direction (N/S)
                if (*token == 'S') gnss_data->latitude *= -1;
                break;
            case 5:  // Longitude (DDDMM.MMMM)
                gnss_data->longitude = nmea_to_decimal(atof(token));
                break;
            case 6:  // Longitude direction (E/W)
                if (*token == 'W') gnss_data->longitude *= -1;
                break;
            case 7:  // Speed (knots)
                // Can be parsed if needed
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}

void nmea_parse_gngga(const char *nmea)
{
    char copy[NMEA_MAX_LEN];
    SAFE_STRNCPY(copy, nmea, sizeof(copy));

    char *token = strtok(copy, ",");
    int field = 0;

    while (token != NULL) 
    {
        switch (field) 
        {
            case 1: // Time
                UTC_time = nmea_parse_time(token);
                break;
            case 2: // Latitude
                gnss_data->latitude = nmea_to_decimal(atof(token));
                break;
            case 3: // N/S Indicator
                //gnss_data->ns_indicator = *token;
                //if (*token == 'S') gnss_data->latitude *= -1;
                break;
            case 4: // Longitude
                gnss_data->longitude = nmea_to_decimal(atof(token));
                break;
            case 5: // E/W Indicator
                //gnss_data->ew_indicator = *token;
                //if (*token == 'W') gnss_data->longitude *= -1;
                break;
            case 6: // Fix Quality
                gnss_data->fix_quality = atoi(token);
                break;
            case 7: // Number of Satellites
                //gnss_data->num_satellites = atoi(token);
                break;
            case 8: // HDOP
                //gnss_data->hdop = atof(token);
                break;
            case 9: // Altitude
                gnss_data->altitude = atof(token);
                break;
            case 10: // Altitude Units
                //gnss_data->altitude_units = *token;
                break;
            case 11: // Geoid Separation
                //gnss_data->geoid_separation = atof(token);
                break;
            case 12: // Geoid Units
                //gnss_data->geoid_units = *token;
                break;
            case 13: // Age of Differential Correction
                //SAFE_STRNCPY(gnss_data->age_of_diff_corr, token, sizeof(gnss_data->age_of_diff_corr));
                break;
            case 14: // Diff Ref Station ID
                //SAFE_STRNCPY(gnss_data->diff_ref_station_id, token, sizeof(gnss_data->diff_ref_station_id));
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}

#ifdef GSA
void nmea_parse_gpgsa(const char *nmea) 
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
                gnss_data->fix_quality = atoi(token);
                break;
            case 15: // PDOP
                gnss_data->pdop = atof(token);
                break;
            case 16: // HDOP
                gnss_data->hdop = atof(token);
                break;
            case 17: // VDOP
                gnss_data->vdop = atof(token);
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}
#endif

#ifdef GSV
void nmea_parse_gpgsv(const char *nmea) 
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
                gnss_data->total_sats_in_view = atoi(token);
                break;
            case 4:  // Satellite PRN (first of up to 4 per message)
            case 8:  // Second satellite
            case 12: // Third satellite
            case 16: // Fourth satellite
                if (sat_index < 24) 
                {
                    gnss_data->sat_info[sat_index].prn = atoi(token);
                    sat_index++;
                }
                break;
            case 5:  // Elevation (first satellite)
            case 9:  // Second satellite
            case 13: // Third satellite
            case 17: // Fourth satellite
                if (sat_index > 0) 
                {
                    gnss_data->sat_info[sat_index-1].elevation = atoi(token);
                }
                break;
            case 6:  // Azimuth (first satellite)
            case 10: // Second satellite
            case 14: // Third satellite
            case 18: // Fourth satellite
                if (sat_index > 0) 
                {
                    gnss_data->sat_info[sat_index-1].azimuth = atoi(token);
                }
                break;
            case 7:  // SNR (first satellite)
            case 11: // Second satellite
            case 15: // Third satellite
            case 19: // Fourth satellite
                if (sat_index > 0) 
                {
                    gnss_data->sat_info[sat_index-1].snr = atoi(token);
                }
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}
#endif

#ifdef VTG
void nmea_parse_gpvtg(const char *nmea) 
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
                gnss_data->course = atof(token);
                break;
            case 7:  // Speed (km/h)
                gnss_data->speed = atof(token);
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}
#endif

#ifdef GLL
void nmea_parse_gpgll(const char *nmea) 
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
                gnss_data->latitude = atof(token) / 100.0;
                break;
            case 2:  // N/S
                if (*token == 'S') gnss_data->latitude *= -1;
                break;
            case 3:  // Longitude (DDDMM.MMMM)
                gnss_data->longitude = atof(token) / 100.0;
                break;
            case 4:  // E/W
                if (*token == 'W') gnss_data->longitude *= -1;
                break;
            case 5:  // UTC Time (HHMMSS.SSS)
                SAFE_STRNCPY(gnss_data->timestamp, token, sizeof(gnss_data->timestamp));
                break;
            case 6:  // Status (A=Valid, V=Invalid)
                gnss_data->fix_quality = (*token == 'A') ? 1 : 0;
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
}
#endif
void parse_pqverno(const char *sentence) 
{
    char copy[NMEA_MAX_LEN];
    SAFE_STRNCPY(copy, sentence, sizeof(copy));
    copy[sizeof(copy)-1] = '\0';

    char *token = strtok(copy, ",");
    int index = 0;
    while (token != NULL) 
    {
        if (index == 2) 
        {  // SW Version is usually the 3rd token
            strncpy(gnss_data->firmware_version, token, sizeof(gnss_data->firmware_version));
            gnss_data->firmware_version[sizeof(gnss_data->firmware_version)-1] = '\0';
            break;
        }
        token = strtok(NULL, ",");
        index++;
    }
}

uint8_t nmea_valid_checksum(const char *sentence) 
{
    uint8_t checksum= (uint8_t)strtol(strchr(sentence, '*')+1, NULL, 16);

    char p;
    uint8_t sum = 0;
    ++sentence;
    while ((p = *sentence++) != '*') 
    {
        sum ^= p;
    }

    if (sum != checksum) 
    {
        return NMEA_CHECKSUM_ERROR;
    }

    return _EMPTY;
}

NMEA_MessageType nmea_get_message_type(const char *sentence) 
{
    char *token;
    char copy[NMEA_MAX_LEN];
    SAFE_STRNCPY(copy, sentence, sizeof(copy));
    // Validate checksum first
    uint8_t checksum_status = nmea_valid_checksum(sentence);
    if (checksum_status == NMEA_CHECKSUM_ERROR) 
    {
        return NMEA_CHECKSUM_ERROR;
    }
    
    token = strtok(copy, ",");

    // Detect token type
    if (strstr(token, NMEA_GPGGA_WORD) != NULL) return NMEA_GPGGA;
    if (strstr(token, NMEA_GPRMC_WORD) != NULL) return NMEA_GPRMC;
    if (strstr(token, NMEA_GNGGA_WORD) != NULL) return NMEA_GNGGA;
    //if (strstr(token, NMEA_GPVTG_WORD) != NULL) return NMEA_GPVTG;
    //if (strstr(token, NMEA_GPGSA_WORD) != NULL) return NMEA_GPGSA;
    //if (strstr(token, NMEA_GPGSV_WORD) != NULL) return NMEA_GPGSV;
    //if (strstr(token, NMEA_GPGLL_WORD) != NULL) return NMEA_GPGLL;
    //if (strstr(token, NMEA_GPZDA_WORD) != NULL) return NMEA_GPZDA;
    //if (strstr(token, NMEA_GPGST_WORD) != NULL) return NMEA_GPGST;
    //if (strstr(token, NMEA_GPGNS_WORD) != NULL) return NMEA_GPGNS;
    if (strstr(token, NMEA_PQVERNO_WORD) != NULL) return NMEA_PQVERNO;

    return NMEA_UNKNOWN;
}

static void handle_unknown(const char *sentence) 
{
    char buffer[16]; // Local buffer to store the sentence type
 
    // Extract the first part of the sentence (e.g., "$GPGGA")
    const char *delimiter = strchr(sentence, ','); // Find the first comma
    if (delimiter != NULL)
    {
        size_t length = delimiter - sentence; // Calculate the length of the sentence type
        if (length >= sizeof(buffer))
        {
            length = sizeof(buffer) - 1; // Ensure we don't overflow the buffer
        }
        strncpy(buffer, sentence, length); // Copy the sentence type to buffer
        buffer[length] = '\0'; // Null-terminate the string
    } 
    else 
    {
        strncpy(buffer, sentence, sizeof(buffer) - 1); // Handle case with no comma
        buffer[sizeof(buffer) - 1] = '\0'; // Null-terminate the string
    }
 
    LOG_DBG("UNKNOWN: %s\n", buffer); // Print the extracted sentence type
}

/* NMEA Processing */
void nmea_processing(const char *sentence)
{
    uint8_t msgtype;
     
    // Dispatch to appropriate handler
    msgtype = nmea_get_message_type(sentence);
    if (msgtype == NMEA_CHECKSUM_ERROR) 
    {
        return;
    }
    
    switch (msgtype) 
    {
        case NMEA_GPGGA:
            nmea_parse_gpgga(sentence);
        break;
        case NMEA_GPRMC:
            nmea_parse_gprmc(sentence);
        break;
        case NMEA_GNGGA:
            nmea_parse_gngga(sentence);
        break;
        case NMEA_GPGLL:
        case NMEA_GPGSV:
        case NMEA_GPGSA:
        case NMEA_GPGST:
        case NMEA_GPZDA:
        case NMEA_GPGNS:
            handle_unknown(sentence);
        break;
        case NMEA_PQVERNO:
            parse_pqverno(sentence);
        break;
        default:
            // Unhandled message type
            handle_unknown(sentence);
        break;
    }
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
