//**************************************************************************************
// Author: Gustavo Gonnet
// Contact: gusgonnet@gmail.com
// Project: https://www.hackster.io/gusgonnet/pool-temperature-monitor-5331f2
// License: Apache-2.0
//**************************************************************************************

// IO mapping
// A0 : pool_THERMISTOR

int _version = 1;

#include <math.h>
#include "application.h"

// this is the thermistor used
// https://www.adafruit.com/products/372
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but measurement is 'smoother'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000

//measure the temperature every POOL_READ_INTERVAL msec
#define POOL_READ_INTERVAL 60000

unsigned long pool_interval = 0;
int samples[NUMSAMPLES];
int pool_THERMISTOR = A0;
//this is coming from http://www.instructables.com/id/Datalogging-with-Spark-Core-Google-Drive/?ALLSTEPS
char pool_temperature_str[64]; //String to store the sensor data
char pool_temperature_ifttt[64];

//by default, we'll display the temperature in degrees celsius, but if you prefer farenheit please set this to true
bool useFahrenheit = false;

void setup() {

    Particle.publish("device starting", "Version: " + String(_version), 60, PRIVATE);

    pool_interval = 0;
    Particle.function("status", status);
    pinMode(pool_THERMISTOR, INPUT);
    //google sheets will get this variable
    //the name of this varriable CANNOT be longer than 12 characters
    //https://docs.particle.io/reference/firmware/photon/#particle-variable-
    Particle.variable("pool_tmp", pool_temperature_str, STRING);

}

void loop() {
    //measure the temperature right away after a start and every POOL_READ_INTERVAL msec after that
    if( (millis() - pool_interval >= POOL_READ_INTERVAL) or (pool_interval==0) ) {
        pool_temp();
        pool_interval = millis();
    }
}

/*******************************************************************************
 * Function Name  : status
 * Description    : this function gets called for the sake of pushing the temperature to your phone
 * Return         : 0
 *******************************************************************************/
int status(String args)
{
 //this triggers a recipe in IFTTT
 Particle.publish("pool_temp", pool_temperature_ifttt, 60, PRIVATE);
 return 0;
}

/*******************************************************************************
 * Function Name  : pool_temp
 * Description    : read the value of the thermistor, convert it to degrees and store it in pool_temperature_str
 * Return         : 0
 *******************************************************************************/
int pool_temp()
{
    uint8_t i;
    float average;

    // take N samples in a row, with a slight delay
    for (i=0; i< NUMSAMPLES; i++) {
        samples[i] = analogRead(pool_THERMISTOR);
        delay(10);
    }

    // average all the samples out
    average = 0;
    for (i=0; i< NUMSAMPLES; i++) {
        average += samples[i];
    }
    average /= NUMSAMPLES;

    // convert the value to resistance
    average = (4095 / average)  - 1;
    average = SERIESRESISTOR / average;


    float steinhart;
    steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                 // Invert
    steinhart -= 273.15;                         // convert to C

  // Convert Celsius to Fahrenheit - EXPERIMENTAL, so let me know if it works please - Gustavo.
  // source: http://playground.arduino.cc/ComponentLib/Thermistor2#TheSimpleCode
  // project source to let me know if this works: https://www.hackster.io/gusgonnet/pool-temperature-monitor-5331f2
  if (useFahrenheit) {
    steinhart = (steinhart * 9.0)/ 5.0 + 32.0;
  }

    char ascii[32];
    int steinhart1 = (steinhart - (int)steinhart) * 100;

    // for negative temperatures
    steinhart1 = abs(steinhart1);

    sprintf(ascii,"%0d.%d", (int)steinhart, steinhart1);
    Particle.publish("pool_temp_dashboard", ascii, 60, PRIVATE);

    char tempInChar[32];
    sprintf(tempInChar,"%0d.%d", (int)steinhart, steinhart1);

    //Write temperature to string, google sheets will get this variable
    sprintf(pool_temperature_str, "{\"t\":%s}", tempInChar);

    //this variable will be published by function status()
    sprintf(pool_temperature_ifttt, "%s", tempInChar);

    return 0;
}
