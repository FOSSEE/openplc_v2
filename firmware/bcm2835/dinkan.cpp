//ADS1018 Configuration 
//http://www.ti.com/lit/ds/symlink/ads1018.pdf
//Page 19-20

// |(1=Start Single-shot), (0=no effect)
// |
// | |Mux  (000 = P is AIN0 N is AIN1, 001 = P is AIN0 N is AIN3, 010 = P is AIN1 N is AIN3 , 011 = P is AIN2 N is AIN3
// | |     100 = P is AIN0 N is GND, 101 = P is AIN1 N is GND, 110 = P is AIN2 N is GND, 111 = P is AIN3 N is GND)
// | |    
// | |   |PGA Gain (000 = ±6.144V, 001 = ±4.096V, 010 = ±2.048V, 011 = ±1.024V, 100 = ±0.512V, 101,110,111 = ±0.256V)
// | |   |
// | |   |   |Mode (0 = Continuous conversion mode, 1 = Power-down and single-shot mode (default))
// | |   |   |
// | |   |   | |Data Rate (000 = 128SPS, 001 = 250SPS, 010 = 490SPS, 011 = 920SPS, 100 = 1600SPS, 101 = 2400SPS, 110 = 33005SPS, 111 = not used))
// | |   |   | |   
// | |   |   | |   |Sensor Mode (1=Internal Temp Mode, 0=ADC Mode)
// | |   |   | |   | 
// | |   |   | |   | |PullUp (0 = Pullup resistor disabled on DOUT/DRDY pin, 1 = Pullup resistor enabled on DOUT/DRDY pin (default))
// | |   |   | |   | |   
// | |   |   | |   | | |Update Config (01 = Valid data, update the Config register (default), 00,11,10 = do not update config register)
// | |   |   | |   | | |  
// | |   |   | |   | | |  |You must Always Write a 1 to this bit
// | |   |   | |   | | |  |
// 1 000 010 1 110 1 1 01 1  = 1000010111011011 = 0x85DB (this sets up a single shot for the internal temperature sensor 3300SPS)
// 1 100 101 1 110 0 1 01 1  = 1100101111001011 = 0xCBCB (this sets up a single shot for single ended AIN0, PGA 0.256V, 3300SPS)
// 1 000 101 1 110 0 1 01 1  = 1000101111001011 = 0x8BCB (this sets up a single shot for differnetial AIN0 AIN1, PGA 0.256V, 3300SPS)

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t flag = 0;

void sigint(int a)
{
	flag = 1;
}
int main() 
{        
        if(!bcm2835_init())
                return 1;

        bcm2835_spi_begin();   
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE1);
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256);
	bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

        uint8_t dataMSB = 0, dataLSB = 0, configMSB, configLSB ;
        uint16_t convRegister;
        double dinkan;

        for(;;)
        {
                if (flag)
                {
                        bcm2835_spi_end();
                        bcm2835_close();
                        break;
                }
                
                signal(SIGINT, sigint) ;                                // Ctr+C handling
                
                dataMSB   = bcm2835_spi_transfer(0x85);
                dataLSB   = bcm2835_spi_transfer(0xDB);
                usleep(350);
                configMSB = bcm2835_spi_transfer(0x85);
                configLSB = bcm2835_spi_transfer(0xDB);
                
                //Temp Calculation
                convRegister  = (((dataMSB)<<8 | (dataLSB)) >> 4);      //Moving MSB and LSB to 12 bit and making it right-justified; 4 because 12bit value
	        convRegister &= 0x0FFF;
                if((convRegister) >= 0x0800)
                {
		        convRegister=((~convRegister)+1 & 0x0fff);      //Applying binary twos complement format
		        dinkan = (double)(convRegister*0.125*-1);
	        }
	        dinkan = (double)(convRegister*0.125);
                
                // printf("\nRecieved \nMSB = %2.2X \nLSB = %2.2X\n", dataMSB, dataLSB);
                printf("Temp = %2.2f C\n\n", dinkan);
        }
	return 0;
}