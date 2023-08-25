/**************************************************************
WinFilter version 0.8
http://www.winfilter.20m.com
akundert@hotmail.com

Filter type: Band Pass
Filter model: Rectangular Window
Sampling Frequency: 9 KHz
Fc1 and Fc2 Frequencies: 0.300000 KHz and 2.800000 KHz
Coefficents Quantization: 16-bit
***************************************************************/
#define Ntap 8

#define DCgain 32768

__int16 fir(__int16 NewSample) {
    __int16 FIRCoef[Ntap] = { 
        -5826,
        -6019,
        10614,
        22443,
        10614,
        -6019,
        -5826,
          517
    };

    static __int16 x[Ntap]; //input samples
    __int32 y=0;            //output sample
    int n;

    //shift the old samples
    for(n=Ntap-1; n>0; n--)
       x[n] = x[n-1];

    //Calculate the new output
    x[0] = NewSample;
    for(n=0; n<Ntap; n++)
        y += FIRCoef[n] * x[n];
    
    return y / DCgain;
}
