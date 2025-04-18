#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <string.h>
#include "shellnmea.h"
#include "nmea.h"
#include "gps.h"

LOG_MODULE_REGISTER(shellnmea, LOG_LEVEL_INF);

/* Shell command handler: Request software version from LH29C */
static int cmd_swversion(const struct shell *shell, size_t argc, char **argv) 
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    
    if (send_nmea_message(LC29H_VERNO_CMD) != 0) 
	{
        shell_error(shell, "Failed to send version request");
        return -EIO;
    }
    
    shell_print(shell, "Requested software version from LH29C");
    return 0;
}

static int cmd_show_swversion(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    if (gnss_data == NULL || strlen(gnss_data->firmware_version) == 0) 
    {
        shell_warn(shell, "Software version not available.");
    }
    else 
    {
        shell_print(shell, "Firmware version: %s", gnss_data->firmware_version);
    }
    return 0;
}

static uint8_t shell_checksum(const char *sentence)
{
    uint8_t checksum = 0;
    sentence++; 
    while (*sentence && *sentence != '*') {
        checksum ^= *sentence++;
    }
    return checksum;
}

/* Shell command handler: Custom NMEA command forwarding */
static int cmd_send_nmea(const struct shell *shell, size_t argc, char **argv)
{
    if (argc != 2) 
    {
        shell_error(shell, "Usage: send_nmea <NMEA_CMD_NO_CHECKSUM>");
        shell_error(shell, "Example: send_nmea $PUBX,00");
        return -EINVAL;
    }

    const char *base = argv[1];

    // Validate base sentence
    if (strlen(base) < 6 || base[0] != '$') 
    {
        shell_error(shell, "Invalid NMEA command format");
        return -EINVAL;
    }

    uint8_t checksum = shell_checksum(base);

    // Format full sentence with checksum and termination
    char cmd_buf[128];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s*%02X\r\n", base, checksum);

    if (send_nmea_message(cmd_buf) != 0) 
    {
        shell_error(shell, "Failed to send NMEA command");
        return -EIO;
    }

    shell_print(shell, "Sent to LH29C: %s", cmd_buf);
    return 0;
}

static int cmd_read_nmea(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if (!UTC_time.valid) 
    {
        shell_error(shell,"Invalid time\n");
    }
    
    shell_print(shell,"%-25s: %02u:%02u:%02u.%03u\n", "UTC Time", UTC_time.hours, UTC_time.minutes, UTC_time.seconds, UTC_time.millis);
    shell_print(shell,"%-25s: %.06lf\n", "The Latitude is", gnss_data->latitude);
    shell_print(shell,"%-25s: %.06lf\n", "The Longitude is", gnss_data->longitude);
    shell_print(shell,"%-25s: %.01lf\n", "The Altitude is", (double)gnss_data->altitude);
    shell_print(shell,"https://www.google.com/maps?q=%.06lf,%.06lf&z=18\n", gnss_data->latitude, gnss_data->longitude);
    
    return 0;
}

/* Shell command registration */
SHELL_CMD_REGISTER(swversion, NULL, "Request software version from LH29C", cmd_swversion);
SHELL_CMD_REGISTER(show_swversion, NULL, "Software version is", cmd_show_swversion);
SHELL_CMD_ARG_REGISTER(send_nmea, NULL, "Send custom NMEA command to LH29C (include $ and *CRC)", cmd_send_nmea, 2, 0);
SHELL_CMD_REGISTER(read_nmea, NULL, "Request the GPS data from LH29C", cmd_read_nmea);

void print_banner_char(char ch, int row) 
{
    // Define 5x5 style patterns for each letter (row-wise)
    const char *X[5] = {
        "*   *", " * * ", "  *  ", " * * ", "*   *"
    };
    const char *T[5] = {
        "*****", "  *  ", "  *  ", "  *  ", "  *  "
    };
    const char *R[5] = {
        "**** ", "*   *", "**** ", "*  * ", "*   *"
    };
    const char *A[5] = {
        " *** ", "*   *", "*****", "*   *", "*   *"
    };
    const char *C[5] = {
        " ****", "*    ", "*    ", "*    ", " ****"
    };
    const char *K[5] = {
        "*   *", "*  * ", "***  ", "*  * ", "*   *"
    };
    const char *E[5] = {
        "*****", "*    ", "**** ", "*    ", "*****"
    };

    const char **pattern = NULL;
    switch (ch) {
        case 'X': case 'x': pattern = X; break;
        case 'T': case 't': pattern = T; break;
        case 'R': case 'r': pattern = R; break;
        case 'A': case 'a': pattern = A; break;
        case 'C': case 'c': pattern = C; break;
        case 'K': case 'k': pattern = K; break;
        case 'E': case 'e': pattern = E; break;
        default: pattern = T; break; // fallback
    }

    printf("%s ", pattern[row]);
}

void print_xtracker(void) 
{
    char str[] = "xtracker";

    for (int row = 0; row < 5; row++) 
    {
        for (int i = 0; i < strlen(str); i++) 
        {
            print_banner_char(str[i], row);
        }
        printf("\n");
    }
}



