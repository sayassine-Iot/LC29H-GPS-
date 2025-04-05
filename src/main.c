#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <string.h>
#include <stdio.h>
#include "nmea.h"
 
/* Configuration */
#define NMEA_MAX_LENGTH 82      // Standard NMEA sentence max length
#define NMEA_QUEUE_SIZE 10      // Number of messages to buffer
#define STACK_SIZE 1024         // Processing thread stack size
#define PRIORITY 7              // Processing thread priority

#define RECEIVE_TIMEOUT_MS 100  // UART receive timeout
#define TX_TIMEOUT_MS 1000  // Added transmission timeout
#define RX_BUFFER_SIZE 128

#define RESET_PIN  23
#define WAKEUP_PIN 24
#define VCC_PIN    25

/* Message queue for NMEA sentences */
K_MSGQ_DEFINE(nmea_msgq, sizeof(char[NMEA_MAX_LENGTH]), NMEA_QUEUE_SIZE, 4);
 
/* UART device */
static const struct device *const uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
static uint8_t rx_buf[RX_BUFFER_SIZE];

const struct device *gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static volatile bool tx_done = false;
 
/* NMEA parsing state */
static bool receiving_nmea = false;
static char nmea_sentence[NMEA_MAX_LENGTH];
static size_t nmea_index;
 
/* Forward declarations */
static void handle_unknown(const char *sentence);

void initialize_gps_module(void)
{
    // Configure GPIOs
    if (!device_is_ready(gpio0_dev)) 
    {
        printk("GPIO device not ready\n");
    }
 
    // Configure all pins as outputs
    gpio_pin_configure(gpio0_dev, RESET_PIN, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(gpio0_dev, WAKEUP_PIN, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(gpio0_dev, VCC_PIN, GPIO_OUTPUT_INACTIVE);
 
    // Power sequence for GPS module
    // 1. Power ON GPS (if controlled via GPIO)
    gpio_pin_set(gpio0_dev, VCC_PIN, 1);
    k_msleep(100);  // Wait for power stabilization
 
    // 2. Wake up GPS (active high)
    gpio_pin_set(gpio0_dev, WAKEUP_PIN, 1);
    k_msleep(100);

    // 3. Reset sequence (active low)
    gpio_pin_set(gpio0_dev, RESET_PIN, 0);
    k_msleep(100);  // Hold reset low
    gpio_pin_set(gpio0_dev, RESET_PIN, 1);
    k_msleep(500);  // Wait for module to boot
 
    printk("GPS module initialized\n");
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
 
    printk("UNKNOWN: %s\n", buffer); // Print the extracted sentence type
}
 
/* UART Transmission Function */
int send_nmea_message(const char *sentence)
{
    if (!device_is_ready(uart_dev)) 
    {
        printk("UART device not ready\n");
        return -ENODEV;
    }

    size_t len = strlen(sentence);
    int ret;
    int attempts = 0;
    const int max_attempts = 3;

    while (attempts < max_attempts) 
    {
        // Convert timeout to milliseconds as expected by uart_tx
        ret = uart_tx(uart_dev, (const uint8_t *)sentence, len, TX_TIMEOUT_MS);
        
        if (ret == 0) 
        {
            printk("TX successful: %s", sentence);
            return 0;
        }
        
        printk("TX attempt %d failed: %d\n", attempts + 1, ret);
        k_sleep(K_MSEC(100));  // Short delay between attempts
        attempts++;
    }

    printk("Failed to send after %d attempts: %s", max_attempts, sentence);
    return -EIO;
}
 
/* UART Callback Handler */
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    switch (evt->type) 
    {
        case UART_TX_DONE:
            printk("UART TX completed successfully\n");
            tx_done = true;
        break;
        case UART_TX_ABORTED:
            printk("UART TX aborted!\n");
            tx_done = true;
        break;
        case UART_RX_RDY: 
            const uint8_t *data = &evt->data.rx.buf[evt->data.rx.offset];
            size_t len = evt->data.rx.len;
 
            for (size_t i = 0; i < len; ++i) 
            {
                char c = data[i];
                if (c == '$') 
                {
                    // Start of new sentence
                    receiving_nmea = true;
                    nmea_index = 0;
                    nmea_sentence[nmea_index++] = c;
                } 
                else if (receiving_nmea) 
                {
                    // Check buffer bounds (leave room for \n and \0)
                    if (nmea_index < NMEA_MAX_LENGTH - 2) 
                    {
                        nmea_sentence[nmea_index++] = c;
                        // End of sentence
                        if (c == '\n') 
                        {
                            nmea_sentence[nmea_index] = '\0';
                            receiving_nmea = false;
                         
                            // Add to queue (non-blocking)
                            if (k_msgq_put(&nmea_msgq, nmea_sentence, K_NO_WAIT) != 0) 
                            {
                                printk("Queue full, dropped: %s", nmea_sentence);
                            }
                        }
                    }
                    else 
                    {
                        // Sentence too long
                        receiving_nmea = false;
                        printk("NMEA too long, dropped\n");
                    }
                }
            }
        break;
        case UART_RX_DISABLED:
            uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT_MS);
        break;
        case UART_RX_STOPPED:
            printk("UART RX stopped. Reason: %d\n", evt->data.rx_stop.reason);
            uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT_MS);
        break;
        default:
            printk("Unhandled event: %d\n", evt->data.rx_stop.reason);
        break;
    }
}

/* UART Initialization */
static void uart_init(void)
{
    if (!device_is_ready(uart_dev)) 
    {
        printk("UART device not ready\n");
        return;
    }
 
    // Set callback
    int ret = uart_callback_set(uart_dev, uart_cb, NULL);
    if (ret < 0) 
    {
        printk("Cannot set UART callback (%d)\n", ret);
        return;
    }

    // Start receiving
    ret = uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT_MS);
    if (ret < 0) 
    {
        printk("Cannot enable UART RX (%d)\n", ret);
        return;
    }
 
    printk("UART initialized\n");
}

/* NMEA Processing Thread */
static void nmea_processing_thread(void *unused1, void *unused2, void *unused3)
{
    char sentence[NMEA_MAX_LENGTH];
    uint8_t msgtype;
    // Process complete NMEA message
    GNSS_Data new_data = {0};
     
    while (1) 
    {
        // Wait for message (blocking)
        if (k_msgq_get(&nmea_msgq, &sentence, K_FOREVER) == 0) 
        {
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
                    // Print position information
                    printk("Position: %.6f,%.6f ", new_data.latitude, new_data.longitude);
                    printk("Alt: %.1f m Sats: %d/%d\n", (double)new_data.altitude, new_data.satellites, new_data.total_sats_in_view);
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
    }
}
 
K_THREAD_DEFINE(nmea_thread, STACK_SIZE, nmea_processing_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
 
 /* Main Application */
int main(void)
{
    initialize_gps_module();
    uart_init();
    
    send_nmea_message("$PQTMVER*58\r\n");
    /*send_nmea_message("$PAIR062,1,1*3F\r\n");
    send_nmea_message("$PAIR062,2,1*3C\r\n");
    send_nmea_message("$PAIR062,3,1*3D\r\n");
    send_nmea_message("$PAIR062,4,1*3A\r\n");
    send_nmea_message("$PAIR062,5,1*3B\r\n");*/
    
    if(tx_done == true)
    {
        send_nmea_message("$PQTMRESTOREPAR*13\r\n");
        tx_done = false;
    }
    
    printk("NMEA Parser started\n");
    printk("Use 'send_pair' shell command to transmit $PAIR062 message\n");
}
 