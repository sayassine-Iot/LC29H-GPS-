#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include "shellnmea.h"
#include "nmea.h"

#define RESET_PIN  23
#define WAKEUP_PIN 24
#define VCC_PIN    25
#define SENTENCE_MAX_LEN 128
#define TX_TIMEOUT_MS 1000 

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

const struct device *gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static const struct device *const uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
static char sentence[SENTENCE_MAX_LEN];
static uint16_t sentence_idx = 0;
static volatile bool tx_done = false;

// Work queue and work item
K_THREAD_STACK_DEFINE(gnss_work_q_stack, 1024);
struct k_work_q gnss_work_q;
struct k_work gnss_work;

// Ring buffer for thread-safe data transfer
RING_BUF_DECLARE(gnss_ring_buf, 256);

#ifdef NMEA_TEST 
// Checksum calculation
static bool verify_nmea_checksum(const char *sentence)
{
    const char *asterisk = strchr(sentence, '*');
    if (!asterisk || asterisk - sentence > SENTENCE_MAX_LEN - 3) 
    {
        return false;
    }

    uint8_t checksum = 0;
    for (const char *p = sentence + 1; p < asterisk; p++) 
    {
        checksum ^= *p;
    }

    char checksum_str[3];
    checksum_str[0] = *(asterisk + 1);
    checksum_str[1] = *(asterisk + 2);
    checksum_str[2] = '\0';

    return checksum == strtoul(checksum_str, NULL, 16);
}

static void parse_nmea_sentence(const char *sentence)
{
    if (!verify_nmea_checksum(sentence)) 
    {
        LOG_WRN("Invalid checksum for: %s", sentence);
        return;
    }

    // Check sentence type
    if (strstr(sentence, "GNRMC") || strstr(sentence, "GPRMC")) {
        LOG_DBG("Unhandled NMEA sentence: %s", sentence);
    }
    else if (strstr(sentence, "GNGGA") || strstr(sentence, "GPGGA")) {
        LOG_DBG("Unhandled NMEA sentence: %s", sentence);
    }
    else {
        LOG_DBG("Unhandled NMEA sentence: %s", sentence);
    }
}
#endif

static void initialize_gps_module(void)
{
    // Configure GPIOs
    if (!device_is_ready(gpio0_dev)) 
    {
        LOG_ERR("GPIO device not ready\n");
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
 
    LOG_INF("GPS module initialized\n");
}

static void gnss_work_cb(struct k_work *work)
{
    uint8_t data;
    size_t len;

    while ((len = ring_buf_get(&gnss_ring_buf, &data, 1)) == 1) 
    {
        if (data == '$') 
        {
            // Start of new sentence
            sentence_idx = 0;
            sentence[sentence_idx++] = data;
        }
        else if ((sentence_idx > 0) && (sentence_idx < SENTENCE_MAX_LEN - 1))
        {
            sentence[sentence_idx++] = data;
            
            // Check for complete sentence
            if (data == '\n') 
            {
                sentence[sentence_idx] = '\0';
                nmea_processing(sentence);
                sentence_idx = 0;
            }
        }
    }
}

static void uart_isr(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);

    if (!uart_irq_update(dev)) return;

    while (uart_irq_rx_ready(dev)) 
    {
        uint8_t byte;
        if (uart_fifo_read(dev, &byte, 1) == 1) 
        {
            if (ring_buf_put(&gnss_ring_buf, &byte, 1) != 1) 
            {
                LOG_WRN("Ring buffer full!");
            }
            k_work_submit_to_queue(&gnss_work_q, &gnss_work);
        }
    }
}

int send_nmea_message(const char *sentence)
{
    if (!device_is_ready(uart_dev)) 
    {
        LOG_ERR("UART device not ready\n");
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
            LOG_INF("TX successful: %s", sentence);
            tx_done = true;
            return 0;
        }
        
        k_sleep(K_MSEC(100));  // Short delay between attempts
        attempts++;
    }

    LOG_ERR("Failed to send after %d attempts: %s", max_attempts, sentence);
    return -EIO;
}

int main(void)
{
    print_xtracker();

    initialize_gps_module();

    if (!device_is_ready(uart_dev)) 
    {
        LOG_ERR("UART device not ready");
        return 0;
    }

    // Initialize work queue
    k_work_queue_init(&gnss_work_q);
    k_work_queue_start(&gnss_work_q, gnss_work_q_stack,
                      K_THREAD_STACK_SIZEOF(gnss_work_q_stack),
                      CONFIG_MAIN_THREAD_PRIORITY, NULL);
    k_work_init(&gnss_work, gnss_work_cb);

    // Setup UART interrupt
    uart_irq_callback_user_data_set(uart_dev, uart_isr, NULL);
    uart_irq_rx_enable(uart_dev);
    
#ifdef NMEA_TEST 
    send_nmea_message("$PQTMVER*58\r\n");
    send_nmea_message("$PAIR062,1,1*3F\r\n");
    send_nmea_message("$PAIR062,2,1*3C\r\n");
    send_nmea_message("$PAIR062,3,1*3D\r\n");
    send_nmea_message("$PAIR062,4,1*3A\r\n");
    send_nmea_message("$PAIR062,5,1*3B\r\n");
                   
    if(tx_done == true)
    {
        send_nmea_message("$PQTMRESTOREPAR*13\r\n");
        tx_done = false;
    }

    while (1) 
    {
        k_sleep(K_SECONDS(10));
        LOG_INF("Parser still running...");
    }
#endif
}
