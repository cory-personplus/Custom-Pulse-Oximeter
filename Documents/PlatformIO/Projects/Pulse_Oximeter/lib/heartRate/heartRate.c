/*

    Heart Rate algorithm for MAXIM 30102 using signal filtering and peak detection

    Author: Cory Person
    Created: 14 September 2026

*/
#include "heartRate.h"

MAX_30102 sensor;

int64_t ir_estimate;
int32_t ir_estimated_val;
float filteredir;l


int8_t positiveEdge;
int8_t negativeEdge;
uint8_t head = 0;


//Circular buffer for FIR
int32_t cbuff[32];
float FIRcoef[22] = {-0.00162978, -0.00098179,  0.00091143,  0.00643129,  0.01770807,  0.03560006,
  0.05899041,  0.08471385,  0.10820798,  0.12471711,  0.13066276,  0.12471711,
  0.10820798,  0.08471385,  0.05899041,  0.03560006,  0.01770807,  0.00643129,
  0.00091143, -0.00098179, -0.00162978};

//Test plotter, not actually detecting beat rn
//100 samples per sec = 100hz sample rate
//Seperate AC and DC voltage 
void detectBeat(uint32_t raw_ir) {

    //bool beatDetected = false;

    //First we get the isolated main component of the signal. necessary for variable PPG fluctuation
    ir_estimated_val = averageDCEstimator(&ir_estimate, raw_ir);

    //Then, we filter it. input parameter is the raw ir we obtain, subtracted by our dc offset estimator (for dc removal)
    filteredir = ppgFIRFilter((int32_t)raw_ir - (int32_t)ir_estimated_val);

    printf(">DC Baseline:%lu\n", ir_estimated_val);
    printf(">Filtered Ir:%f\n", filteredir);
    //peak detection
}

/*

Seperate AC from DC voltage to get a reading close to 0. This is an exponential moving avergae 
estimate = alpha * new_sample + (1 - alpha) * estimate
>> 4 is our alpha (1/16)
x = new_sample, est = p.
est = (alpha * x) + (estimate - (estimate * alpha))

*/
int32_t averageDCEstimator(int64_t *est, uint32_t x)
{
    //Fixed point math 
    *est += ((((int64_t)x << 15) - *est) >> ALPHA);
    return (int32_t)(*est >> 15);
}

/*
FIR filter. basic concept: multiply each sample by our filter, than advance the sample in a circular buffer. 

Digital input, is the IR ac - the IR filttered. So lets review

an EMA (Exponential moving average) relies on one runnign umber. An FIR filter needs acces to multiple past raw samples simultaneously to compute an output
This is why we need a circular buffer (cbuff) of around 22 raw samples. 
*/
float ppgFIRFilter(int32_t din)
{    //set equal to val 
    cbuff[head] = din;
    //advance head
        //does this make it so, the ciecular buffer head is always matching the fir coef head? also , 
    float z = FIRcoef[11] * cbuff[(head - 11) & 0x1F];

    for (uint8_t i = 0; i < 11; i++)
    {
        //SUM of time series samples - each convoluted with the coefficient
        //Since the coeficeint list is symmetrical, a more efficient way could be adding the 2 samples 
        //(one on the end, the other from the beginnig) and multiplying by the coeff
        z += FIRcoef[i] * (cbuff[(head - i) & 0x1F] + cbuff[(head - 22 + i) & 0x1F]);

    }

    head = (head + 1) & 0x1F;//%32
    return z;
}

