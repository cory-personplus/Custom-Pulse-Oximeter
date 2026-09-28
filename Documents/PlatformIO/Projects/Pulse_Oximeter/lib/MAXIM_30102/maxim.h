/*

    MAXIM 30102 I2C Driver file

    Author: Cory Person
    Created: 26 August 2026

*/

//I2C write address = 0xAE
//I2C read address = 0xAF
#include "driver/i2c.h" /* ESP32 I2C FUNCTIONS*/
#pragma once

#define MAXIM_I2C_ADDR 0x57 

// I2C Bus default
#define I2C_NUM I2C_NUM_0
// Configure MAXIM Registers
#define MAXIM_REV_ID 0xFE
#define MAXIM_PART_ID 0xFF

#define MAXIM_PART_ID_VALUE 0x15 // expected value when read

// MAXIM Register Addresses (P.10)
#define MAXIM_REG_INTR_STATUS_1 0x00
#define MAXIM_REG_INTR_STATUS_2 0x01
#define MAXIM_REG_INTR_ENABLE_1 0x02
#define MAXIM_REG_INTR_ENABLE_2 0x03
#define MAXIM_REG_FIFO_WR_PTR 0x04
#define MAXIM_REG_OVF_COUNTER 0x05
#define MAXIM_REG_FIFO_RD_PTR 0x06
#define MAXIM_REG_FIFO_DATA 0x07
#define MAXIM_REG_FIFO_CONFIG 0x08
#define MAXIM_REG_MODE_CONFIG 0x09
#define MAXIM_REG_SPO2_CONFIG 0x0A
#define MAXIM_REG_LED1_PA 0x0C
#define MAXIM_REG_LED2_PA 0x0D
#define MAXIM_REG_MULTI_LED_CTRL1 0x11
#define MAXIM_REG_MULTI_LED_CTRL2 0x12
#define MAXIM_REG_TEMP_INTEGER 0x1F
#define MAXIM_REG_TEMP_FRACTION 0x20
#define MAXIM_REG_TEMP_CONFIG 0x21

//8 bit memory address buffer
#define MAXIM_BUFFER_8BIT 0x80

/*
    Sensor Struct
*/
typedef struct {
    //Internal ADC is 18 bits, so we alocate 32 bit integers
    //can hold up to 32 samples of data
    uint32_t red_led[32];

    uint32_t ir_led[32];

    float temp_C;
    
} MAX_30102;

typedef struct {
    uint8_t capacity;   //Good practice to mod the value by this after incrementing
    volatile uint8_t head;
    volatile uint8_t tail;
    uint8_t temp;
    bool bufferFull;

} circular_buff;

/*
    Initialization
    uint8_t is used to return the amount of errors errNum. We're passing in the struct as a parameter
*/
uint8_t MAXIM_Init(MAX_30102 *dev, uint8_t sampleAvg, uint8_t ledMode, uint8_t ledPulse);

/*
    Data acquisition
*/
esp_err_t MAXIM_readTemp( MAX_30102 *dev);
void MAXIM_readSamples(MAX_30102 *dev, circular_buff *c);

/*
    Low level functions
*/
//Takes in an 8 bit for register name, and 8 bits for data. 
esp_err_t MAXIM_readRegister(uint8_t reg, uint8_t *data, uint8_t length);
esp_err_t MAXIM_writeRegister(uint8_t reg, uint8_t data);

/*
    FIFO buffer functions
*/
void clear_Fifo();
void buff_init(circular_buff *c, MAX_30102 *dev, uint8_t cap);
void write_to_buffer(circular_buff *c, MAX_30102 *dev, uint32_t ir_sample, uint32_t red_sample);
bool buffer_available(circular_buff *c);
void next_sample(circular_buff *c, MAX_30102 *dev);

/*
    Getters
*/
uint32_t get_ir(circular_buff *c, MAX_30102 *dev);
uint32_t get_red(circular_buff *c, MAX_30102 *dev);
float get_tempC( MAX_30102 *dev);
