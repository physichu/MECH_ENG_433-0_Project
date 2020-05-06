// I2C Master utilities, using polling rather than interrupts
// The functions must be called in the correct order as per the I2C protocol
// I2C pins need pull-up resistors, 2k-10k
#include "i2c_master_noint.h"
#include<sys/attribs.h>  // __ISR macro

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

void setPin(unsigned char address, unsigned char regist, unsigned char value);
unsigned char readPin(unsigned char address, unsigned char regist);

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
    TRISAbits.TRISA4 = 0;//Register A4 as output
    //TRISBbits.TRISB4 = 1;//Register B4 as input
    LATAbits.LATA4 = 1;//Turn on A4
    //LATBbits.LATB4 = 1;//Turn on B4
    
    __builtin_enable_interrupts();

     ////Chip initialization////
    //Initialize I2C communication
    i2c_master_setup();
    //Set GPA7 to output
    //First Byte: Address Chip
    //0 stands for write, 1 stands for read
    //By default, IOCON.BLANK=0
    //Second Byte: Register 0x00(HEX value)) to make all the bits output
    //Third Byte: Value 0xFF to make all the bits input
    setPin(0b01000000, 0x00, 0x00);
    //Set GPB0 to input
    setPin(0b01000000, 0x01, 0xFF); 
    //blink GPA7 once
    //GPIOA GPIO If input, can read high or low
    //OLATA OLAT If output, set high or low
    //Turn on GPA7
    setPin(0b01000000, 0x14, 0b10000000);
    _CP0_SET_COUNT(0);
    //Wait 1 second
    while(_CP0_GET_COUNT() < 24000000){}
    //Turn off GPA7
    setPin(0b01000000, 0x14, 0b00000000);
    unsigned char p;
    while(1){
        //Turn on A4 green LED(Heart beat starts)
        LATAbits.LATA4 = 1;
        //Read from GPB0
        //GPIOA GPIO If input, can read high or low
        //OLATA OLAT If output, set high or low
        //If high GPB0=0b???????1, if low GPB0=0b????????0
        p = readPin(0b01000001, 0x13);
        //let bits other than the one we want to read become 0
        p = p&0b00000001;
        //Shift the bit to the left most
        //p = p>>7;
        //If the User Bottom is pushed(GPB0 is 0)
        if(p==1){
            //Turn on GPA7(Turn on yellow LED)
            setPin(0b01000000, 0x14, 0b00000000);
        }else{
            //Else turn off GPAA7(Turn off yellow LED) 
            setPin(0b01000000, 0x14, 0b10000000);
        }       
        //Turn off A4 green LED(Heart beat ends)
        //_CP0_SET_COUNT(0);
        while(_CP0_GET_COUNT() < 24000000/2){}
        LATAbits.LATA4 = 0;
        _CP0_SET_COUNT(0);
        //Wait 1 second
        while(_CP0_GET_COUNT() < 24000000/2){}
    }
}

void i2c_master_setup(void) {
    // using a large BRG to see it on the nScope, make it smaller after verifying that code works
    // look up TPGD in the datasheet
    I2C1BRG = 1000; // I2CBRG = [1/(2*Fsck) - TPGD]*Pblck - 2 (TPGD is the Pulse Gobbler Delay)
    I2C1CONbits.ON = 1; // turn on the I2C1 module
}

void i2c_master_start(void) {
    I2C1CONbits.SEN = 1; // send the start bit
    while (I2C1CONbits.SEN) {
        ;
    } // wait for the start bit to be sent
}

void i2c_master_restart(void) {
    I2C1CONbits.RSEN = 1; // send a restart 
    while (I2C1CONbits.RSEN) {
        ;
    } // wait for the restart to clear
}

void i2c_master_send(unsigned char byte) { // send a byte to slave
    I2C1TRN = byte; // if an address, bit 0 = 0 for write, 1 for read
    while (I2C1STATbits.TRSTAT) {
        ;
    } // wait for the transmission to finish
    if (I2C1STATbits.ACKSTAT) { // if this is high, slave has not acknowledged
        // ("I2C1 Master: failed to receive ACK\r\n");
        while(1){} // We'll get stuck here if the chip(slave) does not ACK back
    }
}

unsigned char i2c_master_recv(void) { // receive a byte from the slave
    I2C1CONbits.RCEN = 1; // start receiving data
    while (!I2C1STATbits.RBF) {
        ;
    } // wait to receive the data
    return I2C1RCV; // read and return the data
}

void i2c_master_ack(int val) { // sends ACK = 0 (slave should send another byte)
    // or NACK = 1 (no more bytes requested from slave)
    I2C1CONbits.ACKDT = val; // store ACK/NACK in ACKDT
    I2C1CONbits.ACKEN = 1; // send ACKDT
    while (I2C1CONbits.ACKEN) {
        ;
    } // wait for ACK/NACK to be sent
}

void i2c_master_stop(void) { // send a STOP:
    I2C1CONbits.PEN = 1; // comm is complete and master relinquishes bus
    while (I2C1CONbits.PEN) {
        ;
    } // wait for STOP to complete
}

void setPin(unsigned char address, unsigned char regist, unsigned char value){
    i2c_master_start();//Send start bit
    i2c_master_send(address);//8 bit address
    i2c_master_send(regist);//8 bit Register
    i2c_master_send(value);//8 bit value for Register to take
    i2c_master_stop();//Stop bit
}
unsigned char readPin(unsigned char address, unsigned char regist){
    // if an address, bit 0 = 0 for write, 1 for read
    i2c_master_start();//Start bit
    i2c_master_send(0b01000000);//8 bit address
    i2c_master_send(regist);//8 bit Register
    i2c_master_restart();//Restart bit
    i2c_master_send(address);//8 bit address
    unsigned char i;
    i = i2c_master_recv();//Receive Note that we receive 8 bits for all A or B pins
                      //we need to select out the bit we want to know 
    i2c_master_ack(1);//Acknowledge
    i2c_master_stop();//Stop bit
    return i;
}