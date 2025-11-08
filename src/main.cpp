#include <Arduino.h>
#include "soc/sens_reg.h"
#include "driver/dac.h"
#include "soc/rtc.h"


// put function declarations here:
//int myFunction(int, int);

void setup() {
  Serial.begin(921600);
  delay(1000);
  Serial.println("Starting cosine waveform test...");

  // 1️⃣ Enable cosine waveform generator
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);

  // 2️⃣ Connect generator to DAC channel 1 (GPIO25)
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);

  // 3️⃣ Set frequency step (larger = higher freq)
  uint16_t freq_step = 1000;  // Try 1000 for ~130kHz, lower for slower
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, freq_step, SENS_SW_FSTEP_S);

  // 4️⃣ Fix waveform inversion (makes a proper sine)
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, 2, SENS_DAC_INV1_S);

  // 5️⃣ Enable DAC1 output
  dac_output_enable(DAC_CHANNEL_1);

  Serial.println("Cosine wave enabled on GPIO25 (DAC1).");
}

void loop() {
  // put your main code here, to run repeatedly:
  //delay(10000);
  //Serial.println("Nutz");
  //delay(1000);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}