#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wiringPi.h>

const int CPU_FULL   = 40;   // Correlates with setting FAN_FULL/FAN_LOW/FAN_MID
const int CPU_MID    = 37;
const int CPU_LOW    = 34;

const int FAN_FULL   = 1023; // PWM fan speed range 0-1023
const int FAN_MID    = 800;
const int FAN_LOW    = 600;

const int INTERVAL_S = 2;    // Temp check + fan set interval in seconds

void set_pwm( int gpio, int pwm_fan_speed, bool has_run ) {

    pwmWrite( gpio, pwm_fan_speed );

    #ifdef DEBUG

        printf( " - Set fan to \x1B[31m%d\x1B[0m on WiringPi PWM GPIO %d \n", pwm_fan_speed, gpio );

    #else

        if( ! has_run ) {

            printf( "Initialized fan to %d on WiringPi PWM GPIO %d! \n", pwm_fan_speed, gpio );
        }

    #endif
}

int main( int argc, char *argv[] ) {

    double cpu_temp_raw;   // Value in "temp" file is degrees in C * 1000
    double cpu_temp_c;     // Converted to correct temp
    bool has_run = false;  // Tracks if main loop has ran yet
    int bcm_gpio_pin = 18; // GPIO pin to use for setting PWM
    int wiringpi_pin = 1;

    FILE * cpu_temp_file; // CPU/GPU on same chip - only need to poll this file for both

    // Check for help
    if( argc > 1 && strcmp( argv[1], "--help" ) == 0 ) {

        printf( "\nRaspberry Pi CPU PWM Fan Controller \n"
                "\n"
                "Usage: pwm_fan_control [OPTION]... \n"
                "\n"
                "Watches CPU temp and sets PWM fan speed accordingly with WiringPi library.\n"
                "\n"
                "--gpio BCM_GPIO_PIN_NUMBER       (default 18) BCM GPIO pin # for setting PWM fan speed \n"
                "                                              12, 13, 18, 19 pins supported (hardware PWM) \n"
                "\n"
                "Exit status: 1 if error \n"
                "\n"
                "Online help, docs & bug reports: <https://github.com/folkhack/raspberry-pi-pwm-fan> \n"
        );

        return 0;
    }

    // Check for GPIO override
    if( argc == 3 && strcmp( argv[1], "--gpio" ) == 0 ) {

        bcm_gpio_pin = atoi( argv[2] );
    }

    //
    // PWM-capable pins
    // - https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf
    //
    // BCM GPIO | RPI PIN | WiringPi PIN | PWM Channel | ALT Mode
    // -------------------------------------------------------
    //       12 |      32 |           26 | 0           | ALT0
    //       13 |      33 |           23 | 1           | ALT0
    //       18 |      12 |            1 | 0           | ALT5
    //       19 |      35 |           24 | 1           | ALT5
    //

    // Ensure GPIO correct and map with WiringPi pin
    switch( bcm_gpio_pin ) {

        case 12:
            wiringpi_pin = 26;
            break;

        case 13:
            wiringpi_pin = 23;
            break;

        case 18:
            wiringpi_pin = 1;
            break;

        case 19:
            wiringpi_pin = 24;
            break;

        default:
            printf( "Invalid GPIO pin specified! Must be 12, 13, 18, or 19. Exiting with code 1.\n" );
            return 1;
    }

    printf( "Starting PWM fan control service on GPIO pin #%d (WiringPi pin #%d)... <3 folkhack 2021\n", bcm_gpio_pin, wiringpi_pin );

    // Setup wiringPi
    if( wiringPiSetup() == -1 ) {

        printf( "Error initializing wiring PI library!\n" );
        return 1;
    }

    printf( "Wiring PI library initialized!\n" );

    // Set the PWM pin to output mode (we do not need to read fan speeds)
    pinMode( wiringpi_pin, PWM_OUTPUT );

    printf( "GPIO #%d set to output! Starting CPU temp polling at %ds interval...\n", bcm_gpio_pin, INTERVAL_S );

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
            set_pwm( wiringpi_pin, FAN_FULL, true );
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

        // Check current CPU temp to see what we want to set the fan to and write the
        //    correct motor speed value to the PWM pin
        if( cpu_temp_c >= CPU_FULL ) {

            set_pwm( wiringpi_pin, FAN_FULL, has_run );

        } else if( cpu_temp_c >= CPU_MID ) {

            set_pwm( wiringpi_pin, FAN_MID, has_run );

        } else if( cpu_temp_c >= CPU_LOW ) {

            set_pwm( wiringpi_pin, FAN_LOW, has_run );

        } else {

            set_pwm( wiringpi_pin, 0, has_run );
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
