#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include "nmea.h"
#include "gps.h"

int main(void) 
{
    nmea_init();
	
	while(1) 
	{	
		//gps_location(&current_location);
		//printk("Lat: %f, Lon: %f\n", current_location.latitude, current_location.longitude);
        k_sleep(K_SECONDS(5));
    }
}
