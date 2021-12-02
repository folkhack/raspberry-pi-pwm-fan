#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <wiringPi.h>

// Used for colored debugging output
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"

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

const int WPI_GPIO_PIN = 24;

const int CPU_FULL   = 40;   // Correlates with setting FAN_FULL/FAN_LOW/FAN_MID
const int CPU_MID    = 37;
const int CPU_LOW    = 34;

const int FAN_FULL   = 1023; // PWM motor control is 0-1023 speed for hot CPU
const int FAN_MID    = 800;
const int FAN_LOW    = 600;

const int INTERVAL_S = 2;    // Temp check + fan set interval in seconds

void set_pwm( int pwm_fan_speed, bool has_run ) {

    pwmWrite( WPI_GPIO_PIN, pwm_fan_speed );

    #ifdef DEBUG

        printf( " - Set fan to \x1B[31m%d\x1B[0m \n", pwm_fan_speed );

    #else

        // Only show startup messages on first main loop iteration
        if( ! has_run ) {

            printf( "Initialized fan to %d!\n", pwm_fan_speed );
        }

    #endif
}

int main() {

    double cpu_temp_raw;   // Value in "temp" file is degrees in C * 1000
    double cpu_temp_c;     // Converted to correct temp
    bool has_run = false;  // Tracks if main loop has ran yet

    FILE * cpu_temp_file; // CPU/GPU on same chip - only need to poll this file for both

    printf( "Starting PWM fan control service... <3 folkhack 2021\n" );

    // Setup wiringPi
    if( wiringPiSetup() == -1 ) {

        printf( "Error initializing wiring PI library!\n" );
        return 1;
    }

    printf( "Wiring PI library initialized!\n" );

    // Set the PWM pin to output mode (we do not need to read fan speeds)
    pinMode( WPI_GPIO_PIN, PWM_OUTPUT );

    printf( "PWM pin #%d set to output! Starting CPU temp polling at %ds interval...\n", WPI_GPIO_PIN, INTERVAL_S );

    set_pwm( FAN_FULL, false );
    sleep( 5 );
    set_pwm( 0, false );
    sleep( 5 );
    set_pwm( FAN_FULL, false );
    sleep( 5 );
    set_pwm( 0, false );
    sleep( 5 );
    set_pwm( FAN_FULL, false );
    sleep( 5 );
    set_pwm( 0, false );
    sleep( 5 );

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
            pwmWrite( WPI_GPIO_PIN, FAN_FULL );

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

            set_pwm( FAN_FULL, has_run );

        } else if( cpu_temp_c >= CPU_MID ) {

            set_pwm( FAN_MID, has_run );

        } else if( cpu_temp_c >= CPU_LOW ) {

            set_pwm( FAN_LOW, has_run );

        } else {

            set_pwm( 0, has_run );
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
