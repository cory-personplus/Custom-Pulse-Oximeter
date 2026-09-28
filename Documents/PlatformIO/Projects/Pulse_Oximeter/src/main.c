#include <stdio.h>
#include "driver/i2c.h"
#include "maxim.h"
#include "esp_log.h"
#include "heartRate.h"

#define SAMPLE_AVG 4

#define LED_MODE 2
#define LED_PULSE 0
#define BUFFER_CAPACITY 32  //Capacity of cirular buffer - Sparkfun makes this 4, we are making this 32

static const char* TAG = "MAIN";
//Structs use commas. Also, set equal. Then, use . to access variables

void app_main() {

    /*
        Connect our MAXIM 30102 sensor to our ESP32 I2C  Bus
        Through Espressif i2c driver library
    */
    i2c_config_t maxim_config = {
        .mode = I2C_MODE_MASTER,    /*!< I2C mode */
        .sda_io_num = GPIO_NUM_6, /*!< GPIO number for I2C sda signal */
        .scl_io_num = GPIO_NUM_7, /*!< GPIO number for I2C scl signal */
        .sda_pullup_en = true,  /*!< Internal GPIO pull mode for I2C sda signal*/
        .scl_pullup_en = true,  /*!< Internal GPIO pull mode for I2C scl signal*/
        .master.clk_speed = 400000, //MAX SCL clock rate is 400Khz
    };

    esp_err_t config = i2c_param_config(I2C_NUM_0, &maxim_config); //Need poiunter to I2C bus?
    if (config == ESP_OK) printf("INVALID CONFIG");
    esp_err_t install = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (install == ESP_OK) printf("INVALID INSTALL");

    /*
        Creation of sensor and buffer structs
    */

    MAX_30102 cory;
    circular_buff cb;

    uint32_t currIr;

    uint8_t err = MAXIM_Init(&cory, SAMPLE_AVG, LED_MODE, LED_PULSE);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "HELLO !");
    printf("MAXIM_Init returned: %d\n", err);   
        //Print errors returned 
    buff_init(&cb, &cory, BUFFER_CAPACITY);

    //Poll to test - will implement ISR later. 
    while(1)
    {
        //Forever push new samples
        MAXIM_readSamples(&cory, &cb);
        // printf("Reading samples...");
        // if (!buffer_available(&cb))
        // {
        //     printf("BUFFER CLOSED!");
        // }
        while(buffer_available(&cb))
        {
            //Read or pop those samples
            currIr = get_ir(&cb, &cory);
            next_sample(&cb, &cory);

            //To test well make it print values
            detectBeat(currIr);
            printf(">Current Ir:%lu\n", currIr);
            MAXIM_readTemp(&cory);
            printf("Current Temp: %f", get_tempC(&cory));
        }
        //RTOS
        vTaskDelay(pdMS_TO_TICKS(20));
    }

}
// float calculateBPM(MAX_30102 *dev)
// {
// //BPM Calc 


//     //First we get the isolated main component of the signal.
//     // averageDCEstimator(&ir_estimate, dev->ir_led);

//     // //Then, we filter it (using first/second order butterworth)
//     // ppgFilter(dev->ir_led, &states);

//     // if (detectBeat)
//     // {
        
//     // }

// }