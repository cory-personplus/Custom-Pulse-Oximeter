#include "maxim.h"

uint8_t MAXIM_Init( MAX_30102 *dev, uint8_t sampleAvg, uint8_t ledMode, uint8_t ledPulse)
{
    //Initialize values to 0 
    for(uint8_t i = 0; i < 32; ++i)
    {
        dev->red_led[i] = 0;
        dev->ir_led[i] = 0;
    }

    dev->temp_C = 0.0f;

    //Store the number of transaction errors
    uint8_t errNum = 0;
    esp_err_t status;

    uint8_t regData = 0;
    uint8_t fifoMask = 0;
    uint8_t modeMask = 0;
    uint8_t pulseMask = 0;
    uint8_t sp02Mask = 0;

    //Check device rev and part id this return sthe rev id and part id 
    status = MAXIM_readRegister(MAXIM_PART_ID, &regData, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("I2C status: %d, regData: 0x%02X\n", status, regData);


    //The esp_err_t class will always return an error, if the operation is not executed as an 8 bit value. 
    errNum += (status != ESP_OK);

    if (regData != MAXIM_PART_ID_VALUE)
    {
        return 255;
    }

    status = MAXIM_readRegister(MAXIM_REV_ID, &regData, 1);

    errNum += (status != ESP_OK);


    //Configure registers

    //bitmask regData FIFO mask 
    switch(sampleAvg)
    {
        case 1:
            fifoMask = 0x1F;
            break;
        case 2:
            fifoMask = 0x3F;
            break;
        case 4:
            fifoMask = 0x5F;
            break;
        case 8:
            fifoMask = 0x7F;
            break;
        case 16:
            fifoMask = 0x9F;
            break;
        case 32:
            fifoMask = 0xBF;
            break;
        default:
            fifoMask = 0x5F;
            break;
    }

    switch(ledMode)
    {
        //RED ONLY
        case 1:
            modeMask = 0x02;
            break;
        //RED AND IR
        case 2:
            modeMask = 0x03;
            break;
    }

    //Config sp02 default. 100 samples per second,  4096 ADC range, 17-bit ADC resolution
    sp02Mask = 0x27;

    switch(ledPulse)
    {
        case 0:
            pulseMask = 0x1F;
            break;
        case 1:
            pulseMask = 0x3F;
            break;
        case 2:
            pulseMask = 0x7F;
            break;
        case 3:
            pulseMask = 0xFF;
            break;
        default: 
            pulseMask = 0x7F;
            break;
    }

    //Ditch multi, really only viable for reducing ambient light but
    //This is for green light 

    //this is the sample rate. can we make a fucntion similar to the other, that takes in these values as parameters
    //Sets this register to 01011111, 4 sample averaging, rollover, and no fifo almost full

    status = MAXIM_writeRegister(MAXIM_REG_FIFO_CONFIG, fifoMask);
    errNum += (status != ESP_OK);

    status = MAXIM_writeRegister(MAXIM_REG_MODE_CONFIG, modeMask);
    errNum += (status != ESP_OK);

    status = MAXIM_writeRegister(MAXIM_REG_SPO2_CONFIG, sp02Mask);
    errNum += (status != ESP_OK);

    status = MAXIM_writeRegister(MAXIM_REG_LED1_PA, pulseMask);
    errNum += (status != ESP_OK);

    status = MAXIM_writeRegister(MAXIM_REG_LED2_PA, pulseMask);
    errNum += (status != ESP_OK);

    //Temp enable
    status = MAXIM_writeRegister(MAXIM_REG_TEMP_CONFIG, 0x01);

    //Cllear the fifo
    clear_Fifo();

    //Conclude initialization function
    return errNum;

}
void clear_Fifo()
{
    //We must clear the FIFO buffer before running the FIFO
    uint8_t reg = 0x00;
    MAXIM_writeRegister(MAXIM_REG_FIFO_RD_PTR, reg);
    MAXIM_writeRegister(MAXIM_REG_FIFO_WR_PTR, reg);
    MAXIM_writeRegister(MAXIM_REG_OVF_COUNTER, reg);
}
esp_err_t MAXIM_readTemp( MAX_30102 *dev)
{
    //Need bit mask for tempFrac
    uint8_t tempInt = 0;
    uint8_t tempFrac = 0;

    //Clear the temperature enable bit everytime 
    MAXIM_writeRegister(MAXIM_REG_TEMP_CONFIG, 0x01);

    //In order to read the temp properly, we need to add the integer temp register to the fraction temperature register.
    esp_err_t statusInt = MAXIM_readRegister(MAXIM_REG_TEMP_INTEGER, &tempInt, 1);
    esp_err_t statusFrac = MAXIM_readRegister(MAXIM_REG_TEMP_FRACTION, &tempFrac, 1);


    //Dont forget temp enable
    int8_t signedTemp = (int8_t)tempInt;
    uint8_t newTempFrac = tempFrac & 0x0F;

    dev->temp_C = signedTemp + (newTempFrac * .0625);

    if (statusInt != ESP_OK) return statusInt; 
    if (statusFrac != ESP_OK) return statusFrac; 
    return ESP_OK;
}
void MAXIM_readSamples(MAX_30102 *dev, circular_buff *c)
{
    //We need to utiliz the FIFO for this. 
    //Then, we need to get the FIFO_wr_PTR
    uint8_t readFifo;
    uint8_t writeFifo;
    uint8_t fifoData;
    
    //Then, we need to READ the FIFO_wr_PTR
    MAXIM_readRegister(MAXIM_REG_FIFO_WR_PTR, &writeFifo, 1);
    MAXIM_readRegister(MAXIM_REG_FIFO_RD_PTR, &readFifo, 1);

    int8_t num_available_samples = (int8_t)writeFifo - (int8_t)readFifo;
    //rollover samples
    if (num_available_samples < 0)
    {
        num_available_samples += 32;
    }
    //Send address of fifo data
    MAXIM_readRegister(MAXIM_REG_FIFO_DATA, &fifoData, 1);

    for (int i = 0; i < num_available_samples; ++i)
    {
        //6 bytes were reading
        uint8_t samples[6];
        MAXIM_readRegister(MAXIM_REG_FIFO_DATA, samples, 6);

        uint32_t sample_ir = (uint32_t)((((samples[0] & 0x03) << 24) | (uint32_t)(samples[1] << 16) | (uint32_t)(samples[2] << 8)) >> 8);
        uint32_t sample_red = (uint32_t)((((samples[3] & 0x03) << 24) | (uint32_t)(samples[4] << 16) | (uint32_t)(samples[5] << 8)) >> 8);
        
        //Consistently write to buffer
        write_to_buffer(c, dev, sample_ir, sample_red);

    }

}
/*
    Low level functions
*/
//Takes in an 8 bit for register name (R or W), and 8 bits for data. The point of this function is to reduce the compelxity of reading from the register by only using 3 variables.
esp_err_t MAXIM_readRegister(uint8_t reg, uint8_t *data, uint8_t length)
{
    return i2c_master_write_read_device(I2C_NUM, MAXIM_I2C_ADDR, &reg, 1, data, length, pdMS_TO_TICKS(100));
}
 
esp_err_t MAXIM_writeRegister(uint8_t reg, uint8_t data)
{
    uint8_t write_buffer[2] = {reg, data};
    return i2c_master_write_to_device(I2C_NUM, MAXIM_I2C_ADDR, write_buffer, 2, pdMS_TO_TICKS(100));
}

void buff_init(circular_buff *c, MAX_30102 *dev, uint8_t cap)
{
    if (c == NULL) return;
    
    c->capacity = cap;
    c->temp = 0;
    //capacity is our buffer sizer. do we want this inputtable by user? should be 32
    c->head = 0;
    c->tail = 0;
    c->bufferFull = false;
    //Do we need capacity?
}
//Add data to the buffer
void write_to_buffer(circular_buff *c, MAX_30102 *dev, uint32_t ir_sample, uint32_t red_sample)
{

    dev->ir_led[c->head] = ir_sample;
    dev->red_led[c->head] = red_sample;
    // printf("HEAD: %u", c->head);
    c->head = (c->head + 1) % c->capacity;
    // printf("HEAD: %u", c->head);
        //rollover
    if (c->bufferFull)
    {
      c->tail = (c->tail + 1) % c->capacity;
    }
    else if(c->head == c->tail)
    {
        c->bufferFull = true;
    }
}
//Read data = pop 
bool buffer_available(circular_buff *c)
{
    //
    printf("HEAD: %u\n", c->head);
    printf("TAIL: %u\n", c->tail);
    printf("BUFF STAT\n: %u", c->bufferFull);

    if ((c->head == c->tail) && (c->bufferFull == false))
    {
        return false;
    }
    else
    {
        return true;
    }
}
void next_sample(circular_buff *c, MAX_30102 *dev)  //Why do we make these values pointers but not the write to buffer? - because without this they would be equal to local copies and not actually modify the caller value
{
    if (buffer_available(c))
    {
        c->tail = (c->tail + 1) % c->capacity;
        c->bufferFull = false;
    }
}
//We need three getters - one for red and one for ir, and one for temp
uint32_t get_ir(circular_buff *c, MAX_30102 *dev)  //Why do we make these values pointers but not the write to buffer? - because without this they would be equal to local copies and not actually modify the caller value
{
    return dev->ir_led[c->tail];  //value of ir_sample
}
uint32_t get_red(circular_buff *c, MAX_30102 *dev)  //Why do we make these values pointers but not the write to buffer? - because without this they would be equal to local copies and not actually modify the caller value
{
    return dev->red_led[c->tail];  //value of ir_sample
}
float get_tempC( MAX_30102 *dev)
{
    return dev->temp_C;
}
