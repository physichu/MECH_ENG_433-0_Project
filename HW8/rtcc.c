#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include "rtcc.h"
#include "ssd1306.h"

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = ON // enable secondary oscillator
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
    
    unsigned char ssd1306_write = 0b01111000; // 0111100 i2c address unique address of ssd1306
    unsigned char ssd1306_read = 0b01111001; //   
    unsigned char ssd1306_buffer[512]; // 128x32/8. Every bit is a pixel      
    char message[200];
    char day[8];
    int count=0;
    //Set up time to the core
    rtcc_setup(0x20123000,0x20060700);
    rtccTime mytime;
    while (1) {
        _CP0_SET_COUNT(0);
        //Read time from core   
        mytime = readRTCC();
        //Convert week from char to string
        dayOfTheWeek(mytime.wk, day); 
        //update refreshed frame times
        sprintf(message, "Hi!  %d", count);        
        drawMessage(0, 0, message);
        //Update TIME
        sprintf(message, "Current Time: %d%d:%d%d:%d%d", mytime.hr10, mytime.hr01, mytime.min10, mytime.min01, mytime.sec10, mytime.sec01);
        drawMessage(4, 8, message);
        //Update DATE        
        sprintf(message, "Date: %s, %d%d/%d%d/20%d%d", day, mytime.mn10, mytime.mn01, mytime.dy10, mytime.dy01, mytime.yr10, mytime.yr01);
        drawMessage(4, 16, message);
        //Set 2Hz update rate
        while(_CP0_GET_COUNT() < 24000000/2){}
        ssd1306_update();
        //Update display at 2Hz
        count+=1;
    }
}

char DAYOFTHEWEEK[7][11] = { "Sunday.", "Monday.", "Tuesday.", "Wednesday.", "Thursday.", "Friday.", "Saturday."};

void dayOfTheWeek(unsigned char position, char* day){
    // given the number of the day of the week, return the word in a char array
    int i = 0;
    while(DAYOFTHEWEEK[position][i]!='.'){
        day[i] = DAYOFTHEWEEK[position][i];
        i++;
    }
    day[i] = '\0';
}

void rtcc_setup(unsigned long time, unsigned long date) {
    OSCCONbits.SOSCEN = 1; // turn on secondary clock
    while (!OSCCONbits.SOSCRDY) {
    } // wait for the clock to stabilize, touch the crystal if you get stuck here

    // unlock sequence to change the RTCC settings
    SYSKEY = 0x0; // force lock, try without first
    SYSKEY = 0xaa996655; // write first unlock key to SYSKEY
    SYSKEY = 0x556699aa; // write second unlock key to SYSKEY
    // RTCWREN is protected, unlock the processor to change it
    RTCCONbits.RTCWREN = 1; // RTCC bits are not locked, can be changed by the user

    RTCCONbits.ON = 0; // turn off the clock
    while (RTCCONbits.RTCCLKON) {
    } // wait for clock to be turned off

    RTCTIME = time; // safe to update the time
    RTCDATE = date; // update the date

    RTCCONbits.ON = 1; // turn on the RTCC

    while (!RTCCONbits.RTCCLKON); // wait for clock to start running, , touch the crystal if you get stuck here
    LATBbits.LATB5 = 0;
}

rtccTime readRTCC() {
    rtccTime time;
    time.sec01 = RTCTIMEbits.SEC01;
    time.sec10 = RTCTIMEbits.SEC10;
    time.min01 = RTCTIMEbits.MIN01;
    time.min10 = RTCTIMEbits.MIN10;
    time.hr01 = RTCTIMEbits.HR01;
    time.hr10 = RTCTIMEbits.HR10;    
    time.yr01 = RTCDATEbits.YEAR01;
    time.yr10 = RTCDATEbits.YEAR10;
    time.mn01 = RTCDATEbits.MONTH01;
    time.mn10 = RTCDATEbits.MONTH10;
    time.dy01 = RTCDATEbits.DAY01;
    time.dy10 = RTCDATEbits.DAY10;
    time.wk = RTCDATEbits.WDAY01;      

    return time;
}