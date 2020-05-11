#include "imu.h"
#include<sys/attribs.h>  // __ISR macro
#include <string.h> // for memset
#include "ssd1306.h"
#include "font.h"

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

//unsigned char readPin(unsigned char address, unsigned char regist);

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

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 0;
    
    __builtin_enable_interrupts();
    
    //Initialize I2C communication
    i2c_master_setup();
     //Initialize IMU   
    imu_setup();
    //Initialize ssd1306 communication
    ssd1306_setup();
    
    //Declare data array
    //2 bytes each for a_x, a_y, a_z, omega_x, omega_y, omega_z, temp 2*14 bytes = 2*14*8 bits
    int len = 7;
    signed short data_IMU[len];
    
//    unsigned char ssd1306_write = 0b01111000; // 0111100 i2c address unique address of ssd1306
//    unsigned char ssd1306_read = 0b01111001; //   
//    unsigned char ssd1306_buffer[512]; // 128x32/8. Every bit is a pixel  
    char message[200];   
       
    while (1) {
        
        LATAbits.LATA4 = !LATAbits.LATA4; 

        imu_read(IMU_OUT_TEMP_L, data_IMU, len);        
        
        if(0){
            sprintf(message, "g: %d %d %d  ", data_IMU[1], data_IMU[2], data_IMU[3]);
            drawMessage(0, 0, message);
            sprintf(message, "a: %d %d %d  ", data_IMU[4], data_IMU[5], data_IMU[6]);
            drawMessage(0, 8, message);
            sprintf(message, "t: %d  ", data_IMU[0]);
            drawMessage(0, 16, message);                                     
        }else{
            bar_x(-data_IMU[5],1);
            bar_y(data_IMU[4], 1);
        }
        ssd1306_update();        
              
        _CP0_SET_COUNT(0);
        while(_CP0_GET_COUNT() < 240000){}              
    }
}




void imu_setup(){
    unsigned char who = 0;

    // read from IMU_WHOAMI
    who = readPin(IMU_ADDR, IMU_WHOAMI);
    
    if (who != 0b01101001){
        while(1){
        //Blink the LED 
        LATAbits.LATA4 = 1;
        _CP0_SET_COUNT(0);
        while(_CP0_GET_COUNT() < 24000000/2){}
        LATAbits.LATA4 = 0;
        _CP0_SET_COUNT(0);
        while(_CP0_GET_COUNT() < 24000000/2){}        
        }
    }
    // init IMU_CTRL1_XL(accelerometer)(1.66kHz 1000 + 2g 00+ 100Hz filter 10)
    setPin(IMU_ADDR, IMU_CTRL1_XL, 0b10000010);
    // init IMU_CTRL2_G(gyroscope)(1.66kHz 1000 + 1000dps 10+ Default 00)
    setPin(IMU_ADDR, IMU_CTRL2_G, 0b10001000);
    // init IMU_CTRL3_C(IF_INC=1/0 enable/disable)
    setPin(IMU_ADDR, IMU_CTRL3_C, 0b00000100);
}

void imu_read(unsigned char regist, signed short * data_IMU, int len){
    volatile unsigned char raw_data[len*2];
    // read multiple from the imu, each data takes 2 reads so you need len*2 chars
    i2c_master_read_multiple(0b1101011, regist, raw_data, len*2);
    // turn the chars into the shorts
    int i;
    signed short p;
    for(i=0;i<7;i++){       
        p = raw_data[2*i+1];
        p = p<<8;
        p = p|raw_data[2*i];
        data_IMU[i] = p; 
    } 
}

void bar_x(signed short accel, int color){
    int i;
    int bar_length;
    bar_length = accel/500;
    //Draw x_bar along the acceleration direction
    for (i=-16; i<=16; i++){
        if (bar_length >= 0){
            if(i>=0 && i <=bar_length){
                ssd1306_drawPixel(64 + i, 16, color);
            }else{
                ssd1306_drawPixel(64 + i, 16, color-1); 
            }
        }else{
            if(i<=0 && i >=bar_length){
                ssd1306_drawPixel(64 + i, 16, color);
            }else{
                ssd1306_drawPixel(64 + i, 16, color-1);   
            }
        }
    }
}

void bar_y(signed short accel, int color){
    int i;
    int bar_length;
    bar_length = accel/500;
    //Draw x_bar along the acceleration direction
    for (i=-16; i<=16; i++){
        if (bar_length >= 0){
            if(i>=0 && i <=bar_length){
                ssd1306_drawPixel(64, 16 + i, color);
            }else{
                ssd1306_drawPixel(64, 16 + i, color-1); 
            }
        }else{
            if(i<=0 && i >=bar_length){
                ssd1306_drawPixel(64, 16 + i, color);
            }else{
                ssd1306_drawPixel(64, 16 + i, color-1);   
            }
        }
    }
}