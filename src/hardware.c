#include "hardware.h"

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


#define GO2_ADC0_PATH "/sys/devices/platform/ff288000.saradc/iio:device0/in_voltage0_raw"
#define GO2_ADC0_VALUE_MAX (1024)


static bool check_range(int value, int min, int max)
{
    bool result;

    if (value >= min && value <=max)
    {
        result = true;
    }
    else
    {
        result = false;
    }
    
    return result;
}


go2_hardware_revision_t go2_hardware_revision_get()
{
    go2_hardware_revision_t result = Go2HardwareRevision_Unknown;


    // /sys/devices/platform/ff288000.saradc/iio:device0# cat in_voltage0_raw
    // 675
    // check_range(655, 695, hwrev_adc)

    int fd = open(GO2_ADC0_PATH, O_RDONLY);
    if (fd > 0)
    {
        char buffer[GO2_ADC0_VALUE_MAX];
        memset(buffer, 0, GO2_ADC0_VALUE_MAX);

        int value;
        ssize_t count = read(fd, buffer, GO2_ADC0_VALUE_MAX);
        if (count > 0)
        {
            value = atoi(buffer);

            if (check_range(value, 655, 695))
            {
                result = Go2HardwareRevision_V1_1;
            }
            else if(value < 655)
            {
                result = Go2HardwareRevision_V1_0;
            }
        }

        close(fd);
    }

    return result;
}
