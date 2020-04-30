// based on adafruit and sparkfun libraries

#include <xc.h> // for the core timer delay
#include<sys/attribs.h>
#include <string.h> // for memset
#include "ssd1306.h"
#include "font.h"

//Initialize PIC32MX170F256B
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

unsigned char ssd1306_write = 0b01111000; // 0111100 i2c address unique address of ssd1306
unsigned char ssd1306_read = 0b01111001; //   
unsigned char ssd1306_buffer[512]; // 128x32/8. Every bit is a pixel    


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

    // Turn off LED(green)@1Hz
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 0;
    
    __builtin_enable_interrupts();
    //Initialize I2C communication   
    i2c_master_setup();
    //Initialize ssd1306 communication
    //ssd1306_setup();
    float fps;
    while(1){
        //Set the core timer to 0
        //_CP0_SET_COUNT(0);
        //Blink a LED(green)@1Hz
        LATAbits.LATA4 = 1;                
        //Blink a pixel@1Hz
        //ssd1306_drawPixel(64, 16, 1);
        //ssd1306_update();
    /////////////////////////////////////
    ///////Write Message Function////////
        //Make a message
        //char message[50];
        //int i=1;
        //sprintf(message, "my var = %d", i);
        //drawMessage(x, y, *message);
        //ssd1306_update();
    /////////////////////////////////////    
    /////////////////////////////////////                  
        //ssd1306_drawPixel(64, 16, 0);
        //ssd1306_update();
    ////////////FPS Display//////////////    
    /////////////////////////////////////
        //fps = 1/_CP0_GET_COUNT();
        //char fps_message[50];
        //sprintf(fps_message, "FPS=%0.2f", i);
        //drawMessage(70, 24, *fps_message);
        //ssd1306_update();
        _CP0_SET_COUNT(0);
        while(_CP0_GET_COUNT() < 24000000){}
        LATAbits.LATA4 = 0;
        _CP0_SET_COUNT(0);
        while(_CP0_GET_COUNT() < 24000000){}
    }
}


void ssd1306_setup() {
    // give a little delay for the ssd1306 to power up
    _CP0_SET_COUNT(0);
    while (_CP0_GET_COUNT() < 48000000 / 2 / 50) {//Small Delay for the ssd1306 to settle
    }
    ssd1306_command(SSD1306_DISPLAYOFF);
    ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);
    ssd1306_command(0x80);
    ssd1306_command(SSD1306_SETMULTIPLEX);
    ssd1306_command(0x1F); // height-1 = 31
    ssd1306_command(SSD1306_SETDISPLAYOFFSET);
    ssd1306_command(0x0);
    ssd1306_command(SSD1306_SETSTARTLINE);
    ssd1306_command(SSD1306_CHARGEPUMP);
    ssd1306_command(0x14);
    ssd1306_command(SSD1306_MEMORYMODE);
    ssd1306_command(0x00);
    ssd1306_command(SSD1306_SEGREMAP | 0x1);
    ssd1306_command(SSD1306_COMSCANDEC);
    ssd1306_command(SSD1306_SETCOMPINS);
    ssd1306_command(0x02);
    ssd1306_command(SSD1306_SETCONTRAST);
    ssd1306_command(0x8F);
    ssd1306_command(SSD1306_SETPRECHARGE);
    ssd1306_command(0xF1);
    ssd1306_command(SSD1306_SETVCOMDETECT);
    ssd1306_command(0x40);
    ssd1306_command(SSD1306_DISPLAYON);
    ssd1306_clear();
    ssd1306_update();
}

// send a command instruction (not pixel data)
void ssd1306_command(unsigned char c) {
    i2c_master_start();
    i2c_master_send(ssd1306_write);
    i2c_master_send(0x00); // bit 7 is 0 for Co bit (data bytes only), bit 6 is 0 for DC (data is a command))
    i2c_master_send(c);
    i2c_master_stop();
}

// update every pixel on the screen
void ssd1306_update() {
    ssd1306_command(SSD1306_PAGEADDR);
    ssd1306_command(0);
    ssd1306_command(0xFF);
    ssd1306_command(SSD1306_COLUMNADDR);
    ssd1306_command(0);
    ssd1306_command(128 - 1); // Width

    unsigned short count = 512; // WIDTH * ((HEIGHT + 7) / 8)
    unsigned char * ptr = ssd1306_buffer; // first address of the pixel buffer
    i2c_master_start();
    i2c_master_send(ssd1306_write);
    i2c_master_send(0x40); // send pixel data
    // send every pixel
    while (count--) {
        i2c_master_send(*ptr++);
    }
    i2c_master_stop();
}

// set a pixel value. Call update() to push to the display)
void ssd1306_drawPixel(unsigned char x, unsigned char y, unsigned char color) {//Color = ON or OFF
    if ((x < 0) || (x >= 128) || (y < 0) || (y >= 32)) {
        return;
    }

    if (color == 1) {
        ssd1306_buffer[x + (y / 8)*128] |= (1 << (y & 7));
    } else {
        ssd1306_buffer[x + (y / 8)*128] &= ~(1 << (y & 7));
    }
}

// zero every pixel value
void ssd1306_clear() {
    memset(ssd1306_buffer, 0, 512); // make every bit a 0, memset in string.h
}

void drawLetter(unsigned char x, unsigned char y, char character){
    //Takes x and y as the position of the character
    //Draw a letter with x & y increase gradually
    //int j;
    //int k;
    //int color;
    //for(j=0; j <= 4; j++){    
        //for(k=0; k <= 7; k++){
            //color = ASCII[character-0x20][j]>>k & 1;
            //ssd1306_drawPixel(x,y,color);
            //y = y + 1;
        //}
        //x = x + 1;
    //}
}
void drawMessage(unsigned char x, unsigned char y, char *arr){
    //int s=0;
    //while(arr[s]!=0){
        //drawLetter(x,y,arr[s]);
        //s = s + 1;
        //Change line if the x position is outside the ssd1306 display range after writing the character
        //x = x + 5;
        //if(x+5 >= 128){//Don't need to change the line, just idle
        //  x = 0;
        //  y = y + 8;
        //}
}