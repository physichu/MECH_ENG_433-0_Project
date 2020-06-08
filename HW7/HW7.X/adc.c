#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include "adc.h"
#include "ws2812b.h"
#include "ssd1306.h"

#define SAMPLE_TIME 10 // in core timer ticks, use a minimum of 250 ns


// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1048576 // use largest wdt
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations



int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;
    
    i2c_master_setup();        
    ssd1306_setup();    
    ws2812b_setup();
    adc_setup();
    ctmu_setup();
       
    unsigned char ssd1306_write = 0b01111000; // 0111100 i2c address unique address of ssd1306
    unsigned char ssd1306_read = 0b01111001; //   
    unsigned char ssd1306_buffer[512]; // 128x32/8. Every bit is a pixel      
    char message[200];
    // Variables for capacitance
    int Baseline_AN0 = 0;
    int Baseline_AN1 = 0;
    int touched_AN0;
    int touched_AN1;
    int i;
    int int_times=10;
    //Variables for capacitive touch slider
    int Delta_Left;
    int Delta_Right;
    float Left_Position;
    float Right_Position;
    float Position;

    //Calculate baseline capacitance    
    for (i=0; i < int_times; i++){
        Baseline_AN0 += ctmu_read(0, 150);
        Baseline_AN1 += ctmu_read(1, 150);
    }
    // Variables for LEDs
    int numLEDs = 4;
    float Brightness;
    wsColor c[numLEDs];
        
    while (1) {
        touched_AN0 = 0;
        touched_AN1 = 0;
        for (i=0; i < int_times; i++){
            touched_AN0 += ctmu_read(0, 150);
            touched_AN1 += ctmu_read(1, 150);
        }
        Delta_Left = Baseline_AN0 - touched_AN0;
        Delta_Right = Baseline_AN1 - touched_AN1;
        Left_Position = Delta_Left*100./(Delta_Left + Delta_Right);
        Right_Position = ((1 - Delta_Right)*100.)/(Delta_Left + Delta_Right);
        Position = (Left_Position + Right_Position)/2;
        //Control ws2812b according to touched spot
        //Decide whether the triangles are touched, respectively.
        if(touched_AN0 < Baseline_AN0 - 50){
            c[0] = HSBtoRGB(120, 0.5, 0.1);
        }else{
            c[0] = HSBtoRGB(120, 0.5, 0);
        }
        if(touched_AN1 < Baseline_AN1 - 50){
            c[1] = HSBtoRGB(120, 0.5, 0.1);
        }else{
            c[1] = HSBtoRGB(120, 0.5, 0);
        }        
        // light up a WS2812B proportionally according to the position touched
        if((touched_AN0 + touched_AN1) < (Baseline_AN0 + Baseline_AN1) -200){
            // (hue, sat, brightness) correspond to(Color in 360 degree, Full color or gray scale, brightness)
        Brightness = 0.5
                *(Position + 50)/100;
            c[2] = HSBtoRGB(240, 0.5, Brightness);
            c[3] = HSBtoRGB(240, 0.5, 0);
        }else{
            c[2] = HSBtoRGB(240, 0.5, 0);
            c[3] = HSBtoRGB(240, 0.5, 0);          
        }
        //light up the LED
        ws2812b_setColor(&c, numLEDs);
                
        sprintf(message, "AN0_C = %5d", touched_AN0);        
        drawMessage(10, 8, message);
        sprintf(message, "AN1_C = %5d", touched_AN1);
        drawMessage(10, 16, message);
        sprintf(message, "Pos = %f", Position);
        drawMessage(10, 24, message);
        ssd1306_update();                
    }
}


unsigned int adc_sample_convert(int pin) {
    unsigned int elapsed = 0, finish_time = 0;
    AD1CHSbits.CH0SA = pin; // connect chosen pin to MUXA for sampling
    AD1CON1bits.SAMP = 1; // start sampling(Close the switch to start sampling)
    elapsed = _CP0_GET_COUNT();
    finish_time = elapsed + SAMPLE_TIME;
    while (_CP0_GET_COUNT() < finish_time) {
        ; // sample for more than 250 ns
    }
    AD1CON1bits.SAMP = 0; // stop sampling and start converting
    while (!AD1CON1bits.DONE) {
        ; // wait for the conversion process to finish
    }
    return ADC1BUF0; // read the buffer with the result
}

void adc_setup() {
    // set analog pins with ANSEL

    AD1CON3bits.ADCS = 1; // ADC clock period is Tad = 2*(ADCS+1)*Tpb = 2*2*20.3ns = 83ns > 75ns
    IEC0bits.AD1IE = 0; // Disable ADC interrupts
    AD1CON1bits.ADON = 1; // turn on A/D converter
}

void ctmu_setup() {
    // base level current is about 0.55uA
    CTMUCONbits.IRNG = 0b11; // 100 times the base level current
    CTMUCONbits.ON = 1; // Turn on CTMU

    // 1ms delay to let it warm up
    _CP0_SET_COUNT(0);
    while (_CP0_GET_COUNT() < 48000000 / 2 / 1000) {
    }
}

int ctmu_read(int pin, int delay) {
    int start_time = 0;
    AD1CHSbits.CH0SA = pin;// AN0-AN12 --> 0-12
    AD1CON1bits.SAMP = 1; // Manual sampling start
    CTMUCONbits.IDISSEN = 1; // Ground the pin
    // Wait 1 ms for grounding
    start_time = _CP0_GET_COUNT();
    while (_CP0_GET_COUNT() < start_time + 48000000 / 2 / 1000) {
    }
    CTMUCONbits.IDISSEN = 0; // End drain of circuit

    CTMUCONbits.EDG1STAT = 1; // Begin charging the circuit
    // wait delay core ticks
    start_time = _CP0_GET_COUNT();
    while (_CP0_GET_COUNT() < start_time + delay) {
    }
    AD1CON1bits.SAMP = 0; // Begin analog-to-digital conversion
    CTMUCONbits.EDG1STAT = 0; // Stop charging circuit
    while (!AD1CON1bits.DONE) // Wait for ADC conversion
    {}
    AD1CON1bits.DONE = 0; // ADC conversion done, clear flag
    return ADC1BUF0; // Get the value from the ADC
}