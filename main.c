#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <pigpio.h>
#include <math.h>

// Used for colored debugging output
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"

//
// PWM-capable pins
// - https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf
//
// GPIO | PIN | PWM Channel | ALT Mode
// -----------------------------------
//   12 |  32 | 0           | ALT0
//   13 |  33 | 1           | ALT0
//   18 |  12 | 0           | ALT5
//   19 |  35 | 1           | ALT5
//

const int GPIO_PWM_PIN = 18;

const int CPU_FULL   = 40;  // Correlates with setting FAN_FULL/FAN_LOW/FAN_MID
const int CPU_MID    = 37;
const int CPU_LOW    = 34;

const int FAN_FULL   = 100; // PWM motor control percent based
const int FAN_MID    = 70;
const int FAN_LOW    = 40;

const int INTERVAL_S = 2;   // Temp check + fan set interval in seconds

int get_fan_speed( int pct ) {

    return round( ( (double) pct / (double) 100 ) * 255 );
}

int set_gpio_mode() {

    int set_alt_mode; // Alternate GPIO function as-defind by Broadcom spec above
    int set_result;   // Stores the result from gpioSetMode for error checking

    switch( GPIO_PWM_PIN ) {

        case 12:
        case 13:
            set_alt_mode = PI_ALT0;
            break;

        case 18:
        case 19:
            set_alt_mode = PI_ALT5;
            break;

        default:
            printf( "Invalid GPIO! Must be 12, 13, 18, or 19 for PWM support. Returning with error.\n" );
            return 1;
    }

    set_result = gpioSetMode( GPIO_PWM_PIN, set_alt_mode );

    if( set_result == PI_BAD_GPIO ) {

        printf( "Setting GPIO returned PI_BAD_GPIO! Returning with error.\n" );
        return 1;

    } else if( set_result == PI_BAD_MODE ) {

        printf( "Setting GPIO returned PI_BAD_MODE! Returning with error.\n" );
        return 1;

    } else if( set_result != 0 ) {

        printf( "Setting GPIO returned non-zero status! Returning with error.\n" );
        return 1;
    }

    printf( "GPIO %d set to PWM\n", GPIO_PWM_PIN );

    return 0;
}

int set_pwm( int pwm_fan_speed, bool has_run ) {

    int set_result = 0;

    set_result = gpioPWM( GPIO_PWM_PIN, pwm_fan_speed );

    if( set_result == PI_BAD_USER_GPIO ) {

        printf( "Setting GPIO PWM returned PI_BAD_USER_GPIO! Returning with error.\n" );
        return 1;

    } else if( set_result == PI_BAD_DUTYCYCLE ) {

        printf( "Setting GPIO PWM returned PI_BAD_DUTYCYCLE! Returning with error.\n" );
        return 1;

    } else if( set_result != 0 ) {

        printf( "Setting GPIO PWM returned non-zero status! Returning with error.\n" );
        return 1;
    }

    #ifdef DEBUG

        printf( " - Set fan to \x1B[31m%d\x1B[0m \n", pwm_fan_speed );

    #else

        // Only show startup messages on first main loop iteration
        if( ! has_run ) {

            printf( "Initialized fan to %d!\n", pwm_fan_speed );
        }

    #endif

    return 0;
}

int main() {

    double cpu_temp_raw;   // Value in "temp" file is degrees in C * 1000
    double cpu_temp_c;     // Converted to correct temp
    bool has_run = false;  // Tracks if main loop has ran yet
    int set_fan_speed;     // Fan speed to set
    int set_result;        // Fan speed set result

    FILE * cpu_temp_file; // CPU/GPU on same chip - only need to poll this file for both

    printf( "Starting PWM fan control service... <3 folkhack 2021\n" );

    // Setup pigpio
    if( gpioInitialise() < 0 ) {

        printf( "Error initializing GPIO! Exiting with code 1.\n" );
        return 1;
    }

    // Set the PWM pin to output mode (we do not need to read fan speeds)
    if( set_gpio_mode() != 0 ) {

        printf( "Error setting GPIO mode! Exiting with code 1.\n" );
        return 1;
    }

    printf( "GPIO initialized! Starting CPU temp polling at %ds interval...\n", INTERVAL_S );

    //
    //  MAIN LOOP
    //
    while( 1 ) {

        // `/sys/class/thermal/thermal_zone0/temp` on Raspberry Pi contains current temp
        //    in Celsius * 1000
        cpu_temp_file = fopen( "/sys/class/thermal/thermal_zone0/temp", "r" );

        if( cpu_temp_file == NULL ) {

            printf( "Error reading CPU temp! Setting fan to full and exiting with code 1...\n" );

            // If CPU temp can't be read, do the safe thing and push fan to full throttle
            //    throwing an exit code
            // gpioPWM( GPIO_PWM_PIN, 255 );

            return 1;
        }

        // Read the temp into the `cpu_temp_raw` variable by reference
        fscanf( cpu_temp_file, "%lf", &cpu_temp_raw );

        // Convert to correct Celsius temp double
        cpu_temp_c = cpu_temp_raw / 1000;

        #ifdef DEBUG
            printf( "Current CPU/GPU temp RAW %f\n", cpu_temp_raw );
            printf( "Current CPU/GPU temp %6.3fC\n", cpu_temp_c );
        #else

            // Only show startup messages on first main loop iteration
            if( ! has_run ) {

                printf( "CPU/GPU temp read successfully for first time: %6.3fC\n", cpu_temp_c );
            }

        #endif

        // Check current CPU temp and set fan speed accordingly
        set_fan_speed = 0;

        if( cpu_temp_c >= CPU_FULL ) {

            set_fan_speed = get_fan_speed( FAN_FULL );

        } else if( cpu_temp_c >= CPU_MID ) {

            set_fan_speed = get_fan_speed( FAN_MID );

        } else if( cpu_temp_c >= CPU_LOW ) {

            set_fan_speed = get_fan_speed( FAN_LOW );
        }

        set_result = set_pwm( set_fan_speed, has_run );

        if( set_result != 0 ) {

            printf( "Setting PWM returned non-zero result! Exiting with code 1.\n" );
            return 1;
        }

        // Set has run flag to ensure first-run logging correctly displays
        has_run = true;

        // Sleep for the interval before running main loop again
        sleep( INTERVAL_S );
    }

    // Shouldn't execute due to main loop!
    fclose( cpu_temp_file );
    return 0;
}
