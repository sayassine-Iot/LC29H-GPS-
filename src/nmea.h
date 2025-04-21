#ifndef _NMEA_H_
#define _NMEA_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

/* NMEA Command Macros */
#define _EMPTY 0x00
#define _COMPLETED 0x03
#define NMEA_MESSAGE_ERR 0xC0
#define NMEA_MAX_LEN 82

/* LC29H-Specific Commands */
#define LC29H_SAVE_CFG         "$PQTMSAVEPAR*5A\r\n" 
#define LC29H_VERNO_CMD        "$PQTMVERNO*58\r\n"
#define LC29H_SET_BAUD         "$PAIR864,0,0,115200*\r\n"
#define LC29H_UPDATE_RATE_CMD  "$PQTXT,W,UPDATE,100*2F\r\n"
#define LC29H_DEFAULT8CFG      "$PQTMRESTOREPAR*13\r\n" 
#define LC29H_ENABLE_GGA       "$PAIR062,0,1*3B\r\n"
#define LC29H_ENABLE_GLL       "$PAIR062,1,1*3A\r\n"
#define LC29H_ENABLE_GSA       "$PAIR062,2,1*3F\r\n"
#define LC29H_ENABLE_GSV       "$PAIR062,3,1*3E\r\n"
#define LC29H_ENABLE_RMC       "$PAIR062,4,1*3D\r\n"
#define LC29H_ENABLE_VTG       "$PAIR062,5,1*38\r\n"
#define LC29H_DISABLE_GGA      "$PAIR062,0,0*3A\r\n"
#define LC29H_DISABLE_GLL      "$PAIR062,1,0*3B\r\n"  
#define LC29H_DISABLE_GSA      "$PAIR062,2,0*3C\r\n"  
#define LC29H_DISABLE_GSV      "$PAIR062,3,0*3D\r\n"  
#define LC29H_DISABLE_RMC      "$PAIR062,4,0*3E\r\n"  
#define LC29H_DISABLE_VTG      "$PAIR062,5,0*3B\r\n"

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

// NMEA Sentence Identifiers
#define NMEA_GPGGA_WORD "$GPGGA"
#define NMEA_GPRMC_WORD "$GPRMC"
#define NMEA_GPVTG_WORD "$GPVTG"
#define NMEA_GPGSA_WORD "$GPGSA"
#define NMEA_GPGSV_WORD "$GPGSV"
#define NMEA_GPGLL_WORD "$GPGLL"
#define NMEA_GPZDA_WORD "$GPZDA"
#define NMEA_GPGST_WORD "$GPGST"
#define NMEA_GPGNS_WORD "$GPGNS"
#define NMEA_GNGGA_WORD "$GNGGA"
#define NMEA_PQVERNO_WORD "$PQTMVERNO"

#define SAFE_STRNCPY(dest, src, size) \
    do { \
        strncpy(dest, src, size - 1); \
        dest[size - 1] = '\0'; \
    } while (0)

    typedef struct 
{
    uint8_t hours;      // 0-23
    uint8_t minutes;    // 0-59
    uint8_t seconds;    // 0-59
    uint16_t millis;    // 0-999
    bool valid;         // true if time was parsed successfully
} TimeStruct;

extern TimeStruct UTC_time;

/* GNSS Data Structure */
typedef struct 
{
    // Common Fields (from GGA/RMC)
    double latitude;
    double longitude;
    float altitude;
    float speed;         // in km/h (from RMC/VTG)
    float course;        // in degrees (from RMC/VTG)
    uint8_t fix_quality;     // 0=invalid, 1=GPS, 2=DGPS, etc. (from GGA/GSA)
    uint8_t satellites;      // Number of satellites in use (from GGA/GSA)
    uint32_t timestamp;  // UTC time (HHMMSS.SS)
    char date[10];       // UTC date (DDMMYY)
#ifdef GSA
    // Additional Fields (from other messages)
    float hdop;          // Horizontal Dilution of Precision (from GSA)
    float vdop;          // Vertical DOP (from GSA)
    float pdop;          // Position DOP (from GSA)
    float mag_var;       // Magnetic variation (from RMC)
    char mode_indicator; // NMEA 4.1+ mode (A=Autonomous, D=DGPS, etc.) (from GNS/RMC)
    char nav_status[20]; // Navigation status (from GNS)
#endif
#ifdef GSV
    // GSV (Satellites in View)
    int total_sats_in_view;
    struct {
        int prn;         // Satellite PRN number
        int elevation;  // Elevation in degrees
        int azimuth;    // Azimuth in degrees
        int snr;        // Signal-to-noise ratio (dB)
    } sat_info[24];     // Max 24 satellites (4 per GSV sentence)
#endif
#ifdef GST
    // GST (GNSS Pseudorange Errors)
    float std_latitude;  // Standard deviation of latitude error (m)
    float std_longitude; // Standard deviation of longitude error (m)
    float std_altitude;  // Standard deviation of altitude error (m)
#endif
#ifdef ZDA
    // ZDA (UTC Time & Date)
    int utc_year;        // Full year (e.g., 2025)
    int utc_month;       // Month (1-12)
    int utc_day;         // Day (1-31)
    int utc_hour;        // Hour (0-23)
    int utc_min;         // Minute (0-59)
    int utc_sec;         // Second (0-59)
#endif
#ifdef GRS
    // GRS (GNSS Range Residuals)
    float range_residuals[12]; // Range residuals for each satellite (m)
#endif
    // Firmware Info
    char firmware_version[32];
} GNSS_Data;

extern GNSS_Data *gnss_data;

// NMEA Message Types
typedef enum 
{
    NMEA_UNKNOWN = 0,
    NMEA_GPGGA,
    NMEA_GPRMC,
    NMEA_GPVTG,
    NMEA_GPGSA,
    NMEA_GPGSV,
    NMEA_GPGLL,
    NMEA_GPZDA,
    NMEA_GPGST,
    NMEA_GPGNS,
    NMEA_GNGGA,
    NMEA_PQVERNO,
    NMEA_CHECKSUM_ERROR
} NMEA_MessageType;

typedef struct 
{
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} NMEA_Time;

typedef struct 
{
    uint8_t day;
    uint8_t month;
    uint16_t year;  // 2-digit or 4-digit year depending on needs
} NMEA_Date;

void nmea_processing(const char *message);
int send_nmea_message(const char *sentence);
void nmea_init(void);
#endif

