/**
 * Header       HX711_GSR.h
 * Author       2021-06-02 Charles Geiser (https://www.dodeka.ch)
 * 
 * Purpose      Declaration of the class HX711_GSR which supports a loadcell in
 *              combination with the HX711 differential amplifier / 24-bit ADC
 * 
 * Constructor  pinDOUT       Data Out pin                 (seen from HX11 side its an output)
 * arguments    pinPD_SCK     Powerdown / Serial Clock pin (seen from HX11 side its an input)
 *              gramsMaxLoad  specified maximum load of the loadcell
 */
#ifndef _HX711_GSR_H_
#define _HX711_GSR_H_
#include <Arduino.h>

enum class CHN_GAIN  { NO_CHN, CHN_A_128, CHN_B_32, CHN_A_64 };

class HX711_GSR
{
    public:
        HX711_GSR(uint8_t pinDOUT, uint8_t pinPD_SCK, int32_t gramsMaxLoad) : 
            _pinDOUT(pinDOUT), _pinPD_SCK(pinPD_SCK), _gramsMaxLoad(gramsMaxLoad)
        {
            pinMode(_pinDOUT, INPUT);
            pinMode(_pinPD_SCK, OUTPUT);
            digitalWrite(_pinPD_SCK, HIGH);  // sensor goes to powerdown mode when 
            delayMicroseconds(100);          // pin is HIGH for more than 60 us
            digitalWrite(_pinPD_SCK, LOW);   // go to normal operation
        }

    int32_t getRawValue();
    int32_t getAverageValue(uint8_t nbr);
    int32_t setZero(uint8_t nbr);
    double  calibrate(uint8_t nbr);
    double  getWeight(uint8_t nbr);
    int32_t getMaxLoad();
    int32_t get_v0();
    int32_t set_v0(int32_t v0);
    int32_t get_vref();
    int32_t set_vref(int32_t vref);
    int32_t get_wref();
    int32_t set_wref(int32_t grammRefWeight);
    CHN_GAIN get_chnGain();
    CHN_GAIN set_chnGain(CHN_GAIN chnGain);
    double  get_m();
    double  get_b();
    void    calculateCoefficients();
    void    powerdown();
    void    powerup();
    void    printEquation();

    private:
        uint8_t  _pinDOUT;
        uint8_t  _pinPD_SCK;
        int32_t  _gramsMaxLoad;
        CHN_GAIN _chn_gain = CHN_GAIN::CHN_A_128;
        int32_t  _v0   = 0;
        int32_t  _vref = 0;
        int32_t  _gramsRefWeight = -1;
        double   _b = 0.0;
        double   _m = 0.0;    
};
#endif