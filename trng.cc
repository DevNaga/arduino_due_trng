#include "Arduino.h"

class trng_device {
    public:
      explicit trng_device() {
      }
      ~trng_device() {
      }

      virtual void init() = 0;
      virtual int get_trng_isr_status() = 0;
      virtual bool generate(unsigned int &bytes) = 0;
};

class atsam3x8e_trng : public trng_device {
    public:
      explicit atsam3x8e_trng() {
      }
      ~atsam3x8e_trng() {
      }

      void init()
      {
          // enable clock on TRNG
          pmc_enable_periph_clk(ID_TRNG);

          // write RNG and last bit set it
          *TRNG_CR = 0x524e4701;

          // enable interrupt
          *TRNG_IER = 0x01;
      }

      int get_trng_isr_status()
      {
          return (*TRNG_IMR == 0x01);
      }
      
      bool generate(unsigned int &bytes)
      {
          bool trng_read = false;
          int retries = 0;
          
          while (1) {
              if (*TRNG_ISR & 0x01) {
                  bytes = *TRNG_ODATA;
                  trng_read = true;
              }

              retries ++;
              if (retries > 100) {
                  break;
              }
          }

          return trng_read;
      }

    private:
      unsigned int *TRNG_CR = (unsigned int *)0x400BC000;
      unsigned int *TRNG_IER = (unsigned int *)0x400BC010;
      unsigned int *TRNG_IDR = (unsigned int *)0x400BC014;
      unsigned int *TRNG_IMR = (unsigned int *)0x400BC018;
      unsigned int *TRNG_ISR = (unsigned int *)0x400BC01C;
      unsigned int *TRNG_ODATA = (unsigned int *)0x400BC050;
};

enum class trng_device_types {
    ATSAM3x8E,
};

class trng_factory {
    public:
      ~trng_factory()
      {
      }
      static trng_factory *instance() {
          static trng_factory tr;

          return &tr;
      }

      trng_device *create(trng_device_types type)
      {
          if (type == trng_device_types::ATSAM3x8E) {
              return new atsam3x8e_trng();
          }

          return nullptr;
      }
};

class key_gen {
    public:
      explicit key_gen()
      {
          dev = trng_factory::instance()->create(trng_device_types::ATSAM3x8E);
          dev->init();
      }
      ~key_gen()
      {
      }

      int get_key128(unsigned char *bytes)
      {
          int counter = 0;
          if (dev->get_trng_isr_status() == 0) {
              return -1;
          }

          int i = 0;

          for (i = 0; i < 4; i ++) {
              unsigned int data = 0;
              
              if (dev->generate(data)) {
                  bytes[counter] = data & 0x000000ff;
                  counter ++;
                  
                  bytes[counter] = (data & 0x0000ff00) >> 8;
                  counter ++;
                  
                  bytes[counter] = (data & 0x00ff0000) >> 16;
                  counter ++;

                  bytes[counter] = (data & 0xff000000) >> 24;
                  counter ++;
              }
          }

          return 0;
      }

    private:
      trng_device *dev;
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.println("init key_gen");
  key_gen key;
 
  while (1) {
      delay(1000);
      unsigned char key_val[16];
      key.get_key128(key_val);
      int i;

      Serial.print("key: ");
      for (i = 0; i < 16; i ++) {
          Serial.print(key_val[i], HEX);
      }
      Serial.println();
  }
}
