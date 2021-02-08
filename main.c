#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <wiringPi.h>

// Used for colored debugging output
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"

const int PWM_PIN    = 1;    // Maps to pin 12 on RPI
const int FAN_FULL   = 1023; // PWM motor control is 0-1023
const int FAN_LOW    = 750;  // Low speed is half of full
const int FAN_OFF    = 0;    // Fan off is zero
const int INTERVAL_S = 2;    // Temp check + fan set interval in seconds

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
    pinMode( PWM_PIN, PWM_OUTPUT );

    printf( "PWM pin #%d set to output! Starting CPU temp polling at %ds interval...\n", PWM_PIN, INTERVAL_S );

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
            pwmWrite( PWM_PIN, FAN_FULL );

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
        if( cpu_temp_c >= 46 ) {

            pwmWrite( PWM_PIN, FAN_FULL );

            #ifdef DEBUG

                printf( " - Set fan to \x1B[31m%d\x1B[0m \n", FAN_FULL );

            #else

                // Only show startup messages on first main loop iteration
                if( ! has_run ) {

                    printf( "Initialized fan to %d!\n", FAN_FULL );
                }

            #endif

        } else if( cpu_temp_c >= 42 ) {

            pwmWrite( PWM_PIN, FAN_LOW );

            #ifdef DEBUG

                printf( " - Set fan to \x1B[33m%d\x1B[0m \n", FAN_LOW );

            #else

                // Only show startup messages on first main loop iteration
                if( ! has_run ) {

                    printf( "Initialized fan to %d!\n", FAN_LOW );
                }

            #endif

        } else {

            pwmWrite( PWM_PIN, FAN_OFF );

            #ifdef DEBUG

                printf( " - Set fan to \x1B[32m%d\x1B[0m \n", FAN_FULL );

            #else

                // Only show startup messages on first main loop iteration
                if( ! has_run ) {

                    printf( "Initialized fan to %d!\n", FAN_FULL );
                }

            #endif
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