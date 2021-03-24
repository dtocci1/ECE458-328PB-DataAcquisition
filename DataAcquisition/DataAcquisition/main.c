/*
 *main.c
 *
 * Created: 1/11/2021 12:01:58 PM
 * Author : dylma
 *
 * ADC Pin setup:
 *	C0 = RTD probe
 *  C1 = Pressure sensor
 *  C2 = Moisture sensor
 *  C3 = Windspeed (TBD)
 */ 

#include <stdio.h>
#include <avr/io.h>
#define F_CPU 16000000 // Need to specify before util/delay.h, it won't work otherwise
#include <util/delay.h>
#define USART_BAUDRATE 9600
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)
#define VREF 5

float returnPressure(uint16_t pressure)
{
	float conversion;
	conversion = ((((5 * ((double)pressure / 1024)) - 0.5) * 25.3815) + 12.422) * 1000;// * 25.3815) - 0.6351236) * 1000; // convert to 10-bit value to Volts, then multiply by 23.3815 (PSI/V) for pressure
	return conversion;
}

float returnTemperature(uint16_t resistance) // requires compensation
{
	float conversion;
	conversion = 0.260075107866203 * resistance - 260.068512926113;
	return conversion;
}

int returnMoisture(uint16_t moisture) {
	// Moisture sensor measures from 0 to 3.085V max based on water level
	// In theory, more voltage indicates more water is hitting the sensor
	// This needs to be validated in testing.
	// If statement need to be adjusted, reaches level 5 at half way on sensor somehow
	float conversion;
	conversion = ((double)moisture/1024) * 5000; // convert to voltage (0-5V)
	return conversion;
	// Determine moisture content
	if(conversion < 0.6)
		return 1;
	else if (conversion < 1.2)
		return 2;
	else if (conversion < 1.8)
		return 3;
	else if (conversion < 2.4)
		return 4;
	else
		return 5;
		
}

void USART0Init(void)
{
	// Set baud rate
	UBRR0H = (uint8_t)(UBRR_VALUE>>8);
	UBRR0L = (uint8_t)UBRR_VALUE;
	// Set frame format to 8 data bits, no parity, 1 stop bit
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);
	//enable transmission and reception
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
}

int USART0SendByte(char u8Data, FILE *stream)
{
	if(u8Data == '\n')
	{
		USART0SendByte('\r', stream);
	}
	//wait while previous byte is completed
	while(!(UCSR0A&(1<<UDRE0))){};
	// Transmit data
	UDR0 = u8Data;
	return 0;
}

//set stream pointer
FILE usart0_str = FDEV_SETUP_STREAM(USART0SendByte, NULL, _FDEV_SETUP_WRITE);
void InitADC()
{
    // Select Vref = AVcc
    ADMUX |= (1<<REFS0);
    //set prescaller to 128 and enable ADC  
    ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);     
}

uint16_t ReadADC(uint8_t ADCchannel)
{
    //select ADC channel with safety mask
    ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F); 
    //single conversion mode
    ADCSRA |= (1<<ADSC);
    // wait until ADC conversion is complete
    while( ADCSRA & (1<<ADSC) );
   return ADC;
}

int main()
{
	double rtdVal, presVal, moisVal, tempVal;
	uint16_t curTime = 0;
	uint16_t convTempVal = 0;
	//initialize ADC
	InitADC();
	//Initialize USART0
	USART0Init();
	//assign our stream to standard I/O streams
	stdout=&usart0_str;
	printf("Time (min)\tTemperature (C)\tPressure (mPSI)\tMoisture (V)\n"); // 15 char columns with tab spacing
	printf("------------------------------------------------------------------------------\n");
	while(1)
	{
		rtdVal = 2.042990654 * ReadADC(0); // translate voltage change to resistance
		tempVal = returnTemperature(rtdVal); // Convert resistance to temperature via linear regression based on table
		presVal = returnPressure(ReadADC(1)); // Calculate pressure
		moisVal = returnMoisture(ReadADC(2));	// Level from 1 - 5: 1 being light mist, 5 being heavy rainfall
		
		// "Print" results to stdout (USART)
		printf("%u\t\t\t",curTime);
		if(tempVal < 0) {
			convTempVal = 0 + -1*tempVal; // Don't use abs(), destorys int value - may be because its a float
			printf("-%u\t\t\t", convTempVal);
		}
		else
		{
			convTempVal = tempVal;
			printf("%u\t\t\t", convTempVal);
		}
		printf("%u\t\t\t",(uint16_t)presVal);
		printf("%u\t\t\t\n",(uint16_t)moisVal);
		_delay_ms(60000);
		curTime += 1;
	} 
}
