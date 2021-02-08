#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <wiringPi.h>

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"

const int PWM_PIN    = 1;
const int FAN_FULL   = 1023;
const int FAN_LOW    = 750;
const int FAN_OFF    = 0;
const int INTERVAL_S = 2;

int main() {

    double cpu_temp_raw;
    double cpu_temp_c;
    bool has_run = false;

    FILE * cpu_temp_file;

    printf( "Starting PWM fan control service... <3 folkhack 2021\n" );

    if( wiringPiSetup() == -1 ) {

        printf( "Error initializing wiring PI library!\n" );
        return 1;
    }

    printf( "Wiring PI library initialized!\n" );

    pinMode( PWM_PIN, PWM_OUTPUT );

    printf( "PWM pin #%d set to output! Starting CPU temp polling at %ds interval...\n", PWM_PIN, INTERVAL_S );

    while( 1 ) {

        cpu_temp_file = fopen( "/sys/class/thermal/thermal_zone0/temp", "r" );

        if( cpu_temp_file == NULL ) {

            printf( "Error reading CPU temp! Setting fan to full and exiting with code 1...\n" );
            pwmWrite( PWM_PIN, FAN_FULL );

            return 1;
        }

        fscanf( cpu_temp_file, "%lf", &cpu_temp_raw );

        cpu_temp_c = cpu_temp_raw / 1000;

        #ifdef DEBUG
            printf( "Current CPU/GPU temp RAW %f\n", cpu_temp_raw );
            printf( "Current CPU/GPU temp %6.3fC\n", cpu_temp_c );
        #else

            if( ! has_run ) {

                printf( "CPU/GPU temp read successfully for first time: %6.3fC\n", cpu_temp_c );
            }

        #endif

        if( cpu_temp_c >= 46 ) {

            pwmWrite( PWM_PIN, FAN_FULL );

            #ifdef DEBUG

                printf( " - Set fan to \x1B[31m%d\x1B[0m \n", FAN_FULL );

            #else

                if( ! has_run ) {

                    printf( "Initialized fan to %d!\n", FAN_FULL );
                }

            #endif

        } else if( cpu_temp_c >= 42 ) {

            pwmWrite( PWM_PIN, FAN_LOW );

                        #ifdef DEBUG

                printf( " - Set fan to \x1B[33m%d\x1B[0m \n", FAN_LOW );

            #else

                if( ! has_run ) {

                    printf( "Initialized fan to %d!\n", FAN_LOW );
                }

            #endif

        } else {

            pwmWrite( PWM_PIN, FAN_OFF );

            #ifdef DEBUG

                printf( " - Set fan to \x1B[32m%d\x1B[0m \n", FAN_FULL );

            #else

                if( ! has_run ) {

                    printf( "Initialized fan to %d!\n", FAN_FULL );
                }

            #endif
        }

        has_run = true;
        sleep( INTERVAL_S );
    }

    fclose( cpu_temp_file );

    return 0;
}