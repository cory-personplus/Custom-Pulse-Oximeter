/*

    Heart Rate algorithm for MAXIM 30102 using signal filtering and peak detection

    Author: Cory Person
    Created: 14 September 2026

*/

#include "maxim.h"

#define ALPHA 4 //1/16 == .0625. Try changing for different values

/*
    Peak detection function
*/
void detectBeat(uint32_t raw_ir);

/*
    BPM Calculation
*/
int32_t averageDCEstimator(int64_t *est, uint32_t x);

/*
    Second order butterworth filter 
*/
float ppgFIRFilter(int32_t din);