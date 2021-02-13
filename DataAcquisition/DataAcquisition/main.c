/*
 * ADC Thermistor Test.c
 *
 * Created: 1/11/2021 12:01:58 PM
 * Author : dylma
 */ 

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#define F_CPU 16000000
#define USART_BAUDRATE 9600
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)
#define VREF 5
#define VMAX 3.076923077
#define VMIN 2.142857143

float returnPressure(uint16_t pressure)
{
	
	float conversion;
	conversion = ((5 * ((double)pressure / 1024) * 25.3815) - 0.6351236) * 1000; // convert to 10-bit value to Volts, then multiply by 23.3815 (PSI/V) for pressure
	return conversion;
}

float returnTemp(uint16_t resistance)
{
	float conversion;
	conversion = 0.260075107866203 * resistance - 260.068512926113;
	return conversion;
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
    // Select Vref=AVcc
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
	double potval, presVal, tmp;
	uint16_t vbg;
	//initialize ADC
	InitADC();
	//Initialize USART0
	USART0Init();
	//assign our stream to standard I/O streams
	stdout=&usart0_str;
	while(1)
	{
		//reading potentiometer value and recalculating to Ohms
		potval = 2.042990654 * ReadADC(0); // = something in range of 2.143 to 5V (3.1~ max)
		presVal = returnPressure(ReadADC(1));
		//potval = (tmp*1000) / (VREF - tmp);
		//sending potentiometer value to terminal
		printf("Potentiometer value = %u Ohm\n", (uint16_t)potval);
		//reading band gap voltage and recalculating to volts
		vbg=returnTemp(potval);
		//printing value to terminal
		printf("Temperature =  %u C\n", vbg);
		//approximate 1s
		printf("Pressure Value = %u mPSI\n", (uint16_t)presVal);
		_delay_ms(1000);
	} 
}
