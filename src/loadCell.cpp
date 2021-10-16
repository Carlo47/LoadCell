/**
 * Program      loadCell.cpp
 * Author       2021-06-02 Charles Geiser (https://www.dodeka.ch)
 * 
 * Purpose      Shows the use of a loadcell in combination with the HX711 24-bit ADC
 * 
 * Board        Arduino Uno
 *
 *                               +------------------+ 
 * Wiring                 red    o E+   HX711       |
 *                 Load-  black  o E-           GND o --> GND    Arduino 
 *                 cell   white  o A-           DT  o --> PIN 3
 *                        green  o A+           SCK o --> PIN 2
 *                               o B-           Vcc o --> 5V
 *                               o B+               |
 *                               +------------------+
 *
 * Remarks
 *
 * References   https://github.com/bogde/HX711 
 *              https://github.com/aguegu/ardulibs/blob/master/hx711/hx711.cpp
 *              https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
 *              https://www.youtube.com/watch?v=LIuf2egMioA
 *              https://www.youtube.com/watch?v=lWFiKMSB_4M
 */
#include <Arduino.h>
#include <EEPROM.h>
#include "HX711_GSR.h"

#define PIN_DOUT    3
#define PIN_PD_SCK  2
#define CLR_LINE    "\r                                                                              \r"
#define MAGIC_NBR   42  //init flag

constexpr uint8_t ADDR_INIT_FLAG  = 0;
constexpr uint8_t ADDR_REF_WEIGHT = ADDR_INIT_FLAG  + sizeof(uint8_t);
constexpr uint8_t ADDR_V0         = ADDR_REF_WEIGHT + sizeof(int32_t);
constexpr uint8_t ADDR_VREF       = ADDR_V0         + sizeof(int32_t);
constexpr uint8_t ADDR_CHN_GAIN   = ADDR_VREF       + sizeof(int32_t);
constexpr uint8_t EEPROM_END      = ADDR_CHN_GAIN   + sizeof(uint8_t);
constexpr uint8_t EEPROM_SiZE     = EEPROM_END - ADDR_INIT_FLAG;

const uint32_t maxLoad = 1000;
typedef struct { const char key; const char *txt; void (&action)(); } MenuItem;

uint8_t initFlagEeprom = 0;

HX711_GSR myScale(PIN_DOUT, PIN_PD_SCK, maxLoad);

void enterRefWeight();
void setZero();
void calibrate();
void getValue();
void getWeight();
void setChnA128();
void setChnB32();
void setChnA64();
void powerDown();
void powerUp();
void storeCalibrationData();
void showCalibrationData();
void showEquation();
void showMenu();

MenuItem menu[] = 
{
  { 'r', "[r] Enter reference weight [grams]",   enterRefWeight },
  { 'z', "[z] Set to 0 (Tare)",                  setZero },
  { 'c', "[c] Calibrate with reference weight",  calibrate },
  { 'g', "[g] Get Raw Sensor Value",             getValue },
  { 'w', "[w] Get Weight [grams]",               getWeight },
  { 'a', "[a] Set CHN_A_128",                    setChnA128 },
  { 'A', "[A] Set CHN_A_64",                     setChnA64 },
  { 'b', "[b] Set CHN_B_32",                     setChnB32 },
  { 'p', "[p] Power down",                       powerDown },
  { 'u', "[u] Power up to normal mode",          powerUp },
  { 'S', "[S] Store calibration data in EEPROM", storeCalibrationData },
  { 's', "[s] Show calibration data from EEPROM",showCalibrationData },
  { 'e', "[e] Show Equation",                    showEquation },
  { 'm', "[m] Show menu",                        showMenu },
};
constexpr uint8_t nbrMenuItems = sizeof(menu) / sizeof(menu[0]);

/**
 * Executes the action assigned to the key
 */
void doMenu()
{
  char key = Serial.read();
  Serial.print(CLR_LINE);
  for (int i = 0; i < nbrMenuItems; i++)
  {
    if (key == menu[i].key)
    {
      menu[i].action();
      break;
    }
  } 
}

void enterRefWeight()
{
  int32_t refWeight = -1;
  char buf[64];
  delay(2000);
  while (Serial.available())
  {
    refWeight = Serial.parseInt();
  }
  if (refWeight < myScale.getMaxLoad() / 10 || refWeight > myScale.getMaxLoad())
  {
      snprintf(buf, sizeof(buf), "Value out of range, allowed: %ld .. %ld [grams] ", myScale.getMaxLoad() / 10, myScale.getMaxLoad());
      Serial.print(buf);
      return;
  }
  myScale.set_wref(refWeight);
  snprintf(buf, sizeof(buf), "Reference weight set to %ld ", myScale.get_wref());
  Serial.print(buf);
}

void setChnA128()
{
  myScale.set_chnGain(CHN_GAIN::CHN_A_128);
  Serial.print("Set channel A with gain 128 ");
}

void setChnA64()
{
  myScale.set_chnGain(CHN_GAIN::CHN_A_64);
  Serial.print("Set channel A with gain 64 ");
}

void setChnB32()
{
  myScale.set_chnGain(CHN_GAIN::CHN_B_32);
  Serial.print("Set channel B with gain 32 ");
}

void powerUp()
{
  myScale.powerup();
  Serial.print("Normal mode set");
}

void powerDown()
{
  myScale.powerdown();
  Serial.print("Ppower down mode set ");
}

void setZero()
{
  char buf[64];
  snprintf(buf, sizeof(buf), "v0 = %ld ", myScale.setZero(32));
  Serial.print(buf);
}

void calibrate()
{
  char buf[64];
  if (myScale.get_wref() < 0)
  {
    Serial.print("First enter reference weight! ");
    return;
  }
  if (myScale.get_v0() == 0)
  {
    Serial.print("First Set 0 (Tare) ");
    return;
  }

  double m = myScale.calibrate(16);
  if (fabs(myScale.get_m()) > 1.0)
  {
    Serial.print("First Calibrate with Reference Weight ");
    return;
  }
  snprintf(buf, sizeof(buf), "Calibrated: Weight = %.9f * v %+9.4f ", m, myScale.get_b());
  Serial.print(buf);

}

void getWeight()
{
  double w = myScale.getWeight(8);
  Serial.print(w, 1);
}

void getValue()
{
  uint32_t v = myScale.getAverageValue(16);
  Serial.print(v);
}

/**
 * Stores maxLoad, refWeight, v0, vref to preferences
 */
void storeCalibrationData()
{
  uint8_t chnGain = (uint8_t)myScale.get_chnGain();
  EEPROM.put(ADDR_INIT_FLAG, MAGIC_NBR);
  EEPROM.put(ADDR_REF_WEIGHT, myScale.get_wref());
  EEPROM.put(ADDR_V0, myScale.get_v0());
  EEPROM.put(ADDR_VREF, myScale.get_vref());
  EEPROM.put(ADDR_CHN_GAIN, chnGain);
  Serial.print("Calibration Data stored ");
}

void showCalibrationData()
{
  char buf[80];
  int32_t wRef, vRef, v0;;
  uint8_t magicNbr, c_g;
  EEPROM.get(ADDR_INIT_FLAG, magicNbr);
  EEPROM.get(ADDR_REF_WEIGHT, wRef);
  EEPROM.get(ADDR_V0, v0);
  EEPROM.get(ADDR_VREF, vRef);
  EEPROM.get(ADDR_CHN_GAIN, c_g);
  snprintf(buf, sizeof(buf), "initFlag = %u, wRef = %ld, vRef = %ld, v0 = %ld, chn_gain = %u ", magicNbr, wRef, vRef, v0, c_g);
  Serial.print(buf);
}

void showEquation()
{
  myScale.printEquation();
}

/**
 * Display menu on monitor
 */
void showMenu()
{
  char buf[128];
  snprintf(buf, sizeof(buf), "%s%s %ld%s%s", 
  "\n------------------\n",
  " HX711", 
  myScale.getMaxLoad() / 1000,
  " kg scale\n",
  "------------------\n");
  Serial.print(buf);

  for (int i = 0; i < nbrMenuItems; i++)
  {
    Serial.println(menu[i].txt);
  }
  Serial.print("\nPress a key: ");
}

void initScale()
{
  // if the magic number is present, coefficients were stored in EEPROM 
  // and the relevant values can be retrieved from it
  if (EEPROM.get(ADDR_INIT_FLAG, initFlagEeprom) == MAGIC_NBR)
  {
    int32_t v = 0;
    uint8_t chnGain = 0;
    EEPROM.get(ADDR_REF_WEIGHT, v);     myScale.set_wref(v);
    EEPROM.get(ADDR_V0, v);             myScale.set_v0(v);
    EEPROM.get(ADDR_VREF, v);           myScale.set_vref(v);
    EEPROM.get(ADDR_CHN_GAIN, chnGain); myScale.set_chnGain((CHN_GAIN)chnGain);
    myScale.calculateCoefficients();
  }
}

void setup() 
{
  Serial.begin(115200);
  initScale();
  showMenu();
}

void loop() 
{
  if(Serial.available())
  {
    doMenu();
  }
}

