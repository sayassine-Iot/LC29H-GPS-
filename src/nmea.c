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
#define RECEIVE_BUFF_SIZE     128

/* Timing Constants */
#define SLEEP_TIME_MS         1000
#define RECEIVE_BUFF_SIZE     128
#define RECEIVE_TIMEOUT       100
#define GPS_INIT_DELAY_MS     2000
#define GPS_CMD_DELAY_MS      300

/* NMEA Sentence Buffering */
#define NMEA_MAX_LEN 82
static char nmea_buffer[NMEA_MAX_LEN];
static int nmea_index = 0;

static uint8_t rx_buf[RECEIVE_BUFF_SIZE] = {0};

/* GPS Data Structure */
struct gps_data 
{
    double latitude;
    double longitude;
    float altitude;
    uint8_t satellites;
    char timestamp[10];
    char status;
};

static const struct device *uart = DEVICE_DT_GET(UART_DEVICE_NODE);

static void parse_gpgga(const char *nmea, struct gps_data *data)
{
    char *token;
    char copy[NMEA_MAX_LEN];
    strncpy(copy, nmea, sizeof(copy));

    token = strtok(copy, ",");
    int field = 0;
    
    while (token != NULL) {
        switch (field) {
            case 1:  // Timestamp (HHMMSS.SSS)
                strncpy(data->timestamp, token, sizeof(data->timestamp));
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

static void parse_gprmc(const char *nmea, struct gps_data *data)
{
    char *token;
    char copy[NMEA_MAX_LEN];
    strncpy(copy, nmea, sizeof(copy));

    token = strtok(copy, ",");
    int field = 0;
    
    while (token != NULL) {
        switch (field) {
            case 1:  // Timestamp (HHMMSS.SSS)
                strncpy(data->timestamp, token, sizeof(data->timestamp));
                break;
            case 2:  // Status (A=active, V=void)
                data->status = *token;
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

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    static struct gps_data current_data = {0};
    
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
                        uint8_t checksum = 0;
                        for (char *p = nmea_buffer + 1; p < asterisk; p++) 
                        {
                            checksum ^= *p;
                        }
                        uint8_t expected = strtoul(asterisk + 1, NULL, 16);
                        if (checksum != expected) 
                        {
                            printk("Checksum failed for: %s\n", nmea_buffer);
                            nmea_index = 0;
                            break;
                        }
                    }
                    
                    // Parse known sentence types
                    if (strncmp(nmea_buffer, "$GPGGA", 6) == 0) 
                    {
                        parse_gpgga(nmea_buffer, &current_data);
                        printk("GPS Fix: %.6f° %c, %.6f° %c, Alt: %.1fm, Sats: %d\n",
                              fabs(current_data.latitude), 
                              current_data.latitude >= 0 ? 'N' : 'S',
                              fabs(current_data.longitude),
                              current_data.longitude >= 0 ? 'E' : 'W',
                              (double)current_data.altitude,  // Explicit cast to double
                              current_data.satellites);
                    }
                    else if (strncmp(nmea_buffer, "$GPRMC", 6) == 0) 
                    {
                        parse_gprmc(nmea_buffer, &current_data);
                        printk("Position: %s, Status: %c\n", 
                              current_data.timestamp,
                              current_data.status);
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

uint8_t nmea_get_message_type(const char *message)
{
    uint8_t checksum = 0;

    checksum = nmea_valid_checksum(message);
    if (checksum != _EMPTY) 
    {
        return checksum;
    }

    if (strstr(message, NMEA_GPGGA_WORD) != NULL) 
    {
        return NMEA_GPGGA;
    }

    if (strstr(message, NMEA_GPRMC_WORD) != NULL) 
    {
        return NMEA_GPRMC;
    }

    if (strstr(message, NMEA_GPVTG_WORD) != NULL) 
    {
        return NMEA_GPVTG;
    }

    if (strstr(message, NMEA_GPGSA_WORD) != NULL) 
    {
        return NMEA_GPGSA;
    }

    if (strstr(message, NMEA_GPGSV_WORD) != NULL) 
    {
        return NMEA_GPGSV;
    }

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
