/**
 * Class        HX711.cpp
 * Author       2021-06-02 Charles Geiser (https://www.dodeka.ch)
 *
 * Purpose      A class that supports loadcells in combination with
 *              the HX711 differential amplifier / 24-bit ADC  
 *   
 * Board        Arduino Uno
 * 
 * Remarks      To calibrate the scale only two measuremnts with two  
 *              different weights are needed 
 * 
 * References   https://github.com/bogde/HX711 
 *              https://github.com/aguegu/ardulibs/blob/master/hx711/hx711.cpp
 *              https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf 
 */
#include "HX711_GSR.h"

uint8_t readByte(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder) 
{
    uint8_t value = 0;

    for (uint8_t i = 0; i < 8; ++i) 
    {
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(2);			// stretch pulse for safety
        if(bitOrder == LSBFIRST)
            value |= digitalRead(dataPin) << i;
        else
            value |= digitalRead(dataPin) << (7 - i);
        digitalWrite(clockPin, LOW);	
        delayMicroseconds(2);			// stretch pulse for safety
    }
    return value;
}

/**
 * Read the raw value from the HX711
 */
int32_t HX711_GSR::getRawValue()
{
	// HX711 is ready when pinDout goes LOW
	while (digitalRead(_pinDOUT) != LOW) {}

	int32_t value = 0;
	uint8_t bytes[3] = { 0 };

	// read 3 bytes, highest byte first
	for (uint8_t i = 0; i < 3; i++)
		bytes[2 - i] = readByte(_pinDOUT, _pinPD_SCK, MSBFIRST);

	// select channel and the gain for the next reading
	for (uint8_t i = 0; i < (uint8_t)_chn_gain; i++) 
	{
		digitalWrite(_pinPD_SCK, HIGH);
		delayMicroseconds(2);				// stretch pulse for safety
		digitalWrite(_pinPD_SCK, LOW);
		delayMicroseconds(2);				// stretch pulse for safety
	}

    // convert 24-bit 2's complement into 32- bit 2's complement
	value = (int8_t)bytes[2]; // C guarantees the sign extension
	value =  value << 16 | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[0]; 
	return value;
}

/**
 * Set channel and gain
 */
CHN_GAIN HX711_GSR::set_chnGain(CHN_GAIN chnGain)
{
	_chn_gain = chnGain;
	return _chn_gain;
}

int32_t HX711_GSR::set_v0(int32_t v0)
{
	_v0 = v0;
	return _v0;
}

int32_t HX711_GSR::set_vref(int32_t vref)
{
	_vref = vref;
	return _vref;
}

/**
 * Sets the refWeight asked from the user
 */
int32_t HX711_GSR::set_wref(int32_t gramsRefWeight)
{
	_gramsRefWeight = gramsRefWeight;
	return _gramsRefWeight;
}

void HX711_GSR::calculateCoefficients()
{
	_m = (double)_gramsRefWeight / (double)(_vref - _v0);
	_b = -_m * (double)_v0;
}

/**
 * Set power down mode
 */
void HX711_GSR::powerdown()
{
	digitalWrite(_pinPD_SCK, LOW);
	digitalWrite(_pinPD_SCK, HIGH);
}

/**
 * Set normal mode
 */
void HX711_GSR::powerup()
{
	digitalWrite(_pinPD_SCK, LOW);
}

/**
 * Averages nbr readings
 * Measurement is done nonblocking every 100 ms
 */
int32_t HX711_GSR::getAverageValue(uint8_t nbr)
{
	int32_t v = 0;
	uint8_t  i = 0;
	
	do
	{
		if (millis() % 150 == 0)
		{
			v += getRawValue();
			i++;
		}
	} while (i < nbr);
	return v / nbr;
}

/**
 * Zero the scale by averaging the offset a nbr times
 */
int32_t HX711_GSR::setZero(uint8_t nbr)
{
	_v0 = getAverageValue(nbr);
	return _v0;
}

/**
 * Calibrate the scale by averaging the refWeigt a nbr times
 * and calculating the slope m and the offset b of the linaear equation
 */
double HX711_GSR::calibrate(uint8_t nbr)
{
	_vref = getAverageValue(nbr);
	_m = (double)_gramsRefWeight / double(_vref - _v0);
	_b = -_gramsRefWeight * (double)_v0 / (double)(_vref - _v0);
	return _m; 
}

/**
 * Returns the weight averaged over nbr measurements
 */
double HX711_GSR::getWeight(uint8_t nbr)
{
	int32_t v = getAverageValue(nbr);
	double w = (double)_gramsRefWeight * (double)(v - _v0) / (double)(_vref - _v0);
	w = round(10.0 * w) / 10.0; 
	return w;
}

int32_t HX711_GSR::get_wref()
{
	return _gramsRefWeight;
}

int32_t HX711_GSR::getMaxLoad()
{
	return (_gramsMaxLoad);
}

int32_t HX711_GSR::get_v0()
{
	return _v0;
}

int32_t HX711_GSR::get_vref()
{
	return _vref;
}

double HX711_GSR::get_m()
{
	return _m;
}

double HX711_GSR::get_b()
{
	return _b;
}

CHN_GAIN HX711_GSR::get_chnGain()
{
	return _chn_gain;
}
/**
 * Print the linear equation which converts 
 * raw scale values to weight
 * weight = m * v + b              // v = raw value read for that weight
 * with m = refWeight / (v1 - v0)  // v1 = raw value read with refWeight loaded, v0 = raw value read without load (or with tare)
 *      b = - refWeight * v0 / (v1 - v0)
 * or
 * weight = refWeight * (v - v0) / (v1 -v0) 
 */
void HX711_GSR::printEquation()
{
	char buf[64];
	snprintf(buf, sizeof(buf), "weight = %.9f * v %+9.4f ", _m, _b);
	Serial.print(buf);
}