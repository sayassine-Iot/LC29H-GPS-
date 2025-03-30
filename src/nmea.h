#ifndef _NMEA_H_
#define _NMEA_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/* NMEA Command Macros */
#define _EMPTY 0x00
#define _COMPLETED 0x03
#define NMEA_GPRMC 0x01
#define NMEA_GPGGA 0x02
#define NMEA_GPVTG 0x03
#define NMEA_GPGSA 0x04
#define NMEA_GPGSV 0x05
#define NMEA_GNGGA 0x06  
#define NMEA_GNRMC 0x07  
#define NMEA_UNKNOWN 0x00
#define NMEA_CHECKSUM_ERR 0x80
#define NMEA_MESSAGE_ERR 0xC0

/* LC29H-Specific Commands */
#define LC29H_VERNO_CMD        "$PQTMVERNO*\r\n"
#define LC29H_MSM7_ENABLE      "$PAIR432,1*\r\n"
#define LC29H_REFMSG_ENABLE    "$PAIR434,1*\r\n"
#define LC29H_EPHEMERIS_ENABLE "$PAIR436,1*\r\n"
#define LC29H_SET_BAUD         "$PAIR864,0,0,115200*\r\n"
#define LC29H_SAVE_PARAMS      "$PQTMSAVEPAR*\r\n"
#define LC29H_SURVEY_IN        "$PQTMCFGSVIN,W,1,86400,15,0,0,0*\r\n"
#define LC29H_SURVEY_STATUS    "$PQTMSVINSTATUS*\r\n"
#define LC29H_ENABLE_GGA       "$PAIR062,0,1*\r\n"
#define LC29H_ENABLE_GLL       "$PAIR062,1,1*\r\n"
#define LC29H_ENABLE_GSA       "$PAIR062,2,1*\r\n"
#define LC29H_ENABLE_GSV       "$PAIR062,3,1*\r\n"
#define LC29H_ENABLE_RMC       "$PAIR062,4,1*\r\n"
#define LC29H_ENABLE_VTG       "$PAIR062,5,1*\r\n"
#define LC29H_ENABLE_ZDA       "$PAIR062,6,1*\r\n"
#define LC29H_ENABLE_GRS       "$PAIR062,7,1*\r\n"
#define LC29H_ENABLE_GST       "$PAIR062,8,1*\r\n"
#define LC29H_GET_FW_VER       "$PQVERNO*48\r\n"
#define LC29H_RESET_CMD        "$PQTXT,RST*3B"
#define LC29H_ENABLE_NMEA_CMD  "$PQTXT,W,VER,1,0,0,1,1,1,1,1,1,1*2D"
#define LC29H_UPDATE_RATE_CMD  "$PQTXT,W,UPDATE,100*2F"
#define LC29H_ENABLE_SBAS_CMD  "$PQTXT,W,SBAS,1*3D"

/* NMEA Sentence Enable Commands */
#define NMEA_SET_STDBY_CMD     "$PMTK161,0*28\r\n"     // Enter standby mode
#define NMEA_HOT_RST_CMD       "$PMTK101*32\r\n"       // Hot reset (fastest, keeps ephemeris)
#define NMEA_WARM_RST_CMD      "$PMTK102*31\r\n"       // Warm reset (keeps almanac)
#define NMEA_COLD_RST_CMD      "$PMTK103*30\r\n"       // Cold reset (clears ephemeris, keeps almanac)
#define NMEA_FCOLD_RST_CMD     "$PMTK104*37\r\n"       // Factory cold reset (full reset)
#define NMEA_CLR_FLASH_CMD     "$PMTK120*31\r\n"       // Clear flash data
#define NMEA_CLEAR_ORBIT_CMD   "$PMTK127*36\r\n"       // Clear assisted ephemeris (AGPS)
#define NMEA_FIXINT_CMD        "$PMTK220,1000*1F\r\n"  // Fix interval (1000 ms = 1 Hz)
#define NMEA_FIXINT_5HZ_CMD    "$PMTK220,200*2C\r\n"   // Fix interval (200 ms = 5 Hz)
#define NMEA_FIXINT_10HZ_CMD   "$PMTK220,100*2F\r\n"   // Fix interval (100 ms = 10 Hz)
#define NMEA_ENABLE_PPS_SYNC   "$PMTK255,1*2D\r\n"     // Enable PPS Sync
#define NMEA_DISABLE_PPS_SYNC  "$PMTK255,0*2C\r\n"     // Disable PPS Sync
#define NMEA_ENABLE_GGA_RMC    "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n" // Enable GGA & RMC
#define NMEA_ENABLE_ALL_NMEA   "$PMTK314,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1*2C\r\n" // Enable all messages
#define NMEA_DISABLE_ALL_NMEA  "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n" // Disable all messages
#define NMEA_SAVE_CONFIG       "$PMTK397*26\r\n"       // Save current settings to flash
#define NMEA_QUERY_FW_VER      "$PMTK605*31\r\n"       // Query firmware version
#define NMEA_GPRMC_WORD        "GPRMC"
#define NMEA_GPGGA_WORD        "GPGGA"
#define NMEA_GPVTG_WORD        "GPVTG"  // Velocity information
#define NMEA_GPGSA_WORD        "GPGSA"  // Satellite status
#define NMEA_GPGSV_WORD        "GPGSV"  // Satellites in view

struct gpgga {
    // Latitude eg: 4124.8963 (XXYY.ZZKK.. DEG, MIN, SEC.SS)
    double latitude;
    // Latitude eg: N
    char lat;
    // Longitude eg: 08151.6838 (XXXYY.ZZKK.. DEG, MIN, SEC.SS)
    double longitude;
    // Longitude eg: W
    char lon;
    // Quality 0, 1, 2
    uint8_t quality;
    // Number of satellites: 1,2,3,4,5...
    uint8_t satellites;
    // Altitude eg: 280.2 (Meters above mean sea level)
    double altitude;
};
typedef struct gpgga gpgga_t;

struct gprmc {
    double latitude;
    char lat;
    double longitude;
    char lon;
    double speed;
    double course;
};
typedef struct gprmc gprmc_t;

uint8_t nmea_get_message_type(const char *);
uint8_t nmea_valid_checksum(const char *);
void nmea_parse_gpgga(char *, gpgga_t *);
void nmea_parse_gprmc(char *, gprmc_t *);
void nmea_send_cmd(const char *cmd);
void nmea_read_response(char *buffer, size_t buf_size);
void nmea_init(void);

#endif

