
#ifndef MA_ICS43434_H_
#define MA_ICS43434_H_

#include "arm_const_structs.h"

#define I2S_BUFFER_SIZE                  2048        // Data handler is called when I2S data bufffer contains (I2S_BUFFER_SIZE/2) 32bit words
#define AUDIO_RATE                       10000L      // Audio frame rate; 16000000/AUDIO_RATE must be an integral multiple of 64!
#define FPU_EXCEPTION_MASK               0x0000009F  // FPU exception mask used to clear exceptions in FPSCR register

typedef struct
{
  float     max;
  float     avg;
  float32_t freq[4];
  float32_t complex_mag[4];
} fft_results_t;

void          ICS_Turn_On(void);
fft_results_t sample_mic(float sample_time);
    
#endif /* MA_ICS43434_H_ */
