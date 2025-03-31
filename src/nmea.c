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

#define UART_DEVICE_NODE DT_NODELABEL(uart0)
#define UART_BUF_SIZE 256
#undef RECEIVE_BUFF_SIZE
#define RECEIVE_BUFF_SIZE     128

/* Timing Constants */
#define SLEEP_TIME_MS         1000
#define RECEIVE_TIMEOUT       100
#define GPS_INIT_DELAY_MS     2000
#define GPS_CMD_DELAY_MS      300

/* NMEA Sentence Buffering */
static char nmea_buffer[NMEA_MAX_LEN];
static int nmea_index = 0;

static uint8_t rx_buf[RECEIVE_BUFF_SIZE] = {0};

static const struct device *uart = DEVICE_DT_GET(UART_DEVICE_NODE);

#define SAFE_STRNCPY(dest, src, size) \
    do { \
        strncpy(dest, src, size - 1); \
        dest[size - 1] = '\0'; \
    } while (0)

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

// Declare gps_location with the correct signature before usage
void gps_location(const char *nmea, GNSS_Data *data);

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    GNSS_Data current_data = {0};
    
    switch (evt->type) 
    {
        case UART_RX_RDY:
        for (int i = 0; i < evt->data.rx.len; i++) 
        {
            char c = rx_buf[evt->data.rx.offset + i];
            
            // Start of new NMEA sentence
            if (c == '$') 
            {
                nmea_index = 0;
                nmea_buffer[nmea_index++] = c;
            } 
            // End of NMEA sentence
            else if (c == '\n' && nmea_index > 0) 
            {
                nmea_buffer[nmea_index] = '\0';
                
                // Validate checksum if present
                char *asterisk = strchr(nmea_buffer, '*');
                if (asterisk) 
                {
                    uint8_t checksum = nmea_valid_checksum(nmea_buffer);
                    uint8_t expected = strtoul(asterisk + 1, NULL, 16);
                    if (checksum != expected) 
                    {
                        printk("Checksum failed for: %s\n", nmea_buffer);
                        nmea_index = 0;
                        break;
                    }
                }
                
                // Process complete NMEA message
                GNSS_Data new_data = {0};
                gps_location(nmea_buffer, &new_data); // Correct declaration ensures no warning
                
                // Update current data if we got valid information
                if (new_data.fix_quality > 0) 
                {
                    current_data = new_data;
                    
                    // Print position information
                    printk("Position: %.6f,%.6f ", 
                          current_data.latitude, 
                          current_data.longitude);
                    printk("Alt: %.1f m Sats: %d/%d\n",
                          (double)current_data.altitude, // Explicit cast to double
                          current_data.satellites,
                          current_data.total_sats_in_view);
                    
                    // Print satellite info (first 4)
                    for (int i = 0; i < 4 && i < current_data.total_sats_in_view; i++) 
                    {
                        printk("PRN%d: %ddB@%d° ", 
                              current_data.sat_info[i].prn,
                              current_data.sat_info[i].snr,
                              current_data.sat_info[i].elevation);
                    }
                    printk("\n");
                }
                
                nmea_index = 0;
            } 
            // Accumulate NMEA data
            else if (nmea_index > 0 && nmea_index < NMEA_MAX_LEN - 1) 
            {
                nmea_buffer[nmea_index++] = c;
            }
        }
        break;
         
        case UART_RX_DISABLED:
            uart_rx_enable(dev, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT);
            break;
            
        case UART_TX_DONE:
            printk("Command sent successfully\n");
            break;
            
        default:
            printk("UART event: %d\n", evt->type);
    }
}

// Update gps_location to match its usage
void gps_location(const char *nmea, GNSS_Data *data) 
{
    if (!data || !nmea) return;
    
    uint8_t msgtype;
        
    // Verify checksum and determine message type
    msgtype = nmea_get_message_type(nmea);
    if (msgtype == NMEA_CHECKSUM_ERROR) 
    {
        return;
    }
        
    switch (msgtype) 
    {
        case NMEA_GPGGA:
            nmea_parse_gpgga(nmea, data);
            break;
            
        case NMEA_GPRMC:
            nmea_parse_gprmc(nmea, data);
            break;
            
        case NMEA_GPGLL:
            nmea_parse_gpgll(nmea, data);
            break;
            
        case NMEA_GPGSV:
            nmea_parse_gpgsv(nmea, data);
            break;
            
        case NMEA_GPGSA:
            nmea_parse_gpgsa(nmea, data);
            break;
            
        case NMEA_GPGST:
            //nmea_parse_gpgst(nmea, data);
            break;
            
        case NMEA_GPZDA:
            //nmea_parse_gpzda(nmea, data);
            break;
            
        case NMEA_GPGNS:
            // No direct parser in prototypes, could add if needed
            break;
            
        case NMEA_PQVERNO:
            //nmea_parse_pqverno(nmea, data);
            break;
    
        default:
            // Unhandled message type
            break;
    }
}

void nmea_init(void)
{

    printk("=== GPS Module Initialization ===\n");
    
    if (!device_is_ready(uart))
    {
        printk("UART device not ready\n");
    }

    if (uart_callback_set(uart, uart_cb, NULL) != 0) 
    {
        printk("Failed to set UART callback\n");
    }

    if (uart_rx_enable(uart, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT) != 0) 
    {
        printk("Failed to enable UART RX\n");
    }

    // Initial delay for module power-up
    k_msleep(GPS_INIT_DELAY_MS);

    // Send configuration commands
    printk("Configuring GPS module...\n");
    nmea_send_cmd(LC29H_RESET_CMD);
    k_msleep(GPS_CMD_DELAY_MS);
    
    nmea_send_cmd(LC29H_ENABLE_NMEA_CMD);
    k_msleep(GPS_CMD_DELAY_MS);
    
    nmea_send_cmd(LC29H_UPDATE_RATE_CMD);
    k_msleep(GPS_CMD_DELAY_MS);
    
    nmea_send_cmd(LC29H_ENABLE_SBAS_CMD);
    k_msleep(GPS_CMD_DELAY_MS);

    printk("Waiting for GPS data...\n");
    
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
        return NMEA_CHECKSUM_ERR;
    }

    return _EMPTY;
}

void nmea_send_cmd(const char *cmd) 
{
    printk("Sending command: %s", cmd);
    int ret = uart_tx(uart, (const uint8_t *)cmd, strlen(cmd), SYS_FOREVER_MS);
    if (ret) 
    {
        printk("Failed to send command (err %d)\n", ret);
    }
}

void nmea_read_response(char *buffer, size_t buf_size) 
{
    size_t count = 0;
    int64_t timeout = k_uptime_get() + 500;

    while (count < buf_size - 1) 
    {
        
        if (buffer[count-1] == '\n') 
        {
            break;
        }
        
        if (k_uptime_get() > timeout) 
        {
            break;
        }
        k_sleep(K_MSEC(10));
        count++;
    }

    if (count > 0) {
        buffer[count] = '\0';
        printk("Response: %s", buffer);
    }
}

void nmea_enable_pps_sync(void)
{
	uint8_t buffer[] = NMEA_ENABLE_PPS_SYNC;
	nmea_send_cmd(buffer);
}

void nmea_hot_restart(void)
{
	uint8_t buffer[] = NMEA_HOT_RST_CMD;
	nmea_send_cmd(buffer);
}

void nmea_factory_reset(void)
{
	uint8_t buffer[] = NMEA_FCOLD_RST_CMD;
	nmea_send_cmd(buffer);
}

void nmea_standby(void)
{
	uint8_t buffer[] = NMEA_SET_STDBY_CMD;
	nmea_send_cmd(buffer);
}