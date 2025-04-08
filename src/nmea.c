#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <math.h>
#include "nmea.h"

LOG_MODULE_REGISTER(nmea, CONFIG_LOG_DEFAULT_LEVEL);

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
    // Validate checksum first
    uint8_t checksum_status = nmea_valid_checksum(sentence);
    if (checksum_status == NMEA_CHECKSUM_ERROR) 
    {
        return NMEA_CHECKSUM_ERROR;
    }

    // Detect sentence type
    if (strstr(sentence, NMEA_GPGGA_WORD) != NULL) return NMEA_GPGGA;
    if (strstr(sentence, NMEA_GPRMC_WORD) != NULL) return NMEA_GPRMC;
    if (strstr(sentence, NMEA_GPVTG_WORD) != NULL) return NMEA_GPVTG;
    if (strstr(sentence, NMEA_GPGSA_WORD) != NULL) return NMEA_GPGSA;
    if (strstr(sentence, NMEA_GPGSV_WORD) != NULL) return NMEA_GPGSV;
    if (strstr(sentence, NMEA_GPGLL_WORD) != NULL) return NMEA_GPGLL;
    if (strstr(sentence, NMEA_GPZDA_WORD) != NULL) return NMEA_GPZDA;
    if (strstr(sentence, NMEA_GPGST_WORD) != NULL) return NMEA_GPGST;
    if (strstr(sentence, NMEA_GPGNS_WORD) != NULL) return NMEA_GPGNS;
    if (strstr(sentence, NMEA_PQVERNO_WORD) != NULL) return NMEA_PQVERNO;

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
    // Process complete NMEA message
    GNSS_Data new_data = {0};
     
    // Dispatch to appropriate handler
    msgtype = nmea_get_message_type(sentence);
    if (msgtype == NMEA_CHECKSUM_ERROR) 
    {
        return;
    }
    
    switch (msgtype) 
    {
        case NMEA_GPGGA:
            nmea_parse_gpgga(sentence,&new_data);
        break;
        case NMEA_GPRMC:
            nmea_parse_gprmc(sentence, &new_data);
        break;
        case NMEA_GPGLL:
            nmea_parse_gpgll(sentence, &new_data);
        break;
        case NMEA_GPGSV:
            nmea_parse_gpgsv(sentence, &new_data);
        break;
        case NMEA_GPGSA:
            nmea_parse_gpgsa(sentence, &new_data);
        break; 
        case NMEA_GPGST:
            //nmea_parse_gpgst(sentence, &new_data);
        break;
        case NMEA_GPZDA:
            //nmea_parse_gpzda(sentence, &new_data);
        break;
        case NMEA_GPGNS:
            // No direct parser in prototypes, could add if needed
        break;
        case NMEA_PQVERNO:
            //nmea_parse_pqverno(sentence, &new_data);
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
