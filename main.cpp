#include "mbed.h"
#include "math.h"

BufferedSerial uart0(P1_7, P1_6,115200);  //TX, RX
SPI spi(P0_9, P0_8, P0_6);    //mosi, miso, sclk
DigitalOut cs1(P0_11);
DigitalOut cs2(P0_7);
DigitalOut cs3(P0_3);
DigitalOut cs4(P0_2);
DigitalOut cs5(P1_9);   //for pll
DigitalOut ioup(P1_1);
DigitalOut rst(P1_0);

//CS control
void cs_hi(uint8_t num);
void cs_lo(uint8_t num);

//uart read buf
const uint8_t buf_size=15;
void buf_read(uint8_t num); //uart read func.
char read_buf[buf_size];    //uart read buf
void buf2val();             //buf to vals change func. return to 'freq' and 'pha' global var 
uint32_t freq;              //Hz
uint16_t pha;               //deg.
int8_t ampl;                //power dBm

//DDS control
const uint32_t mclk=1500000000;
const uint64_t res=4294967296;  //2^32

void dds_init(uint8_t ch);  //dds init func.
void waveset(uint8_t ch, uint32_t freq, uint16_t pha, int8_t ampl);  //dds wave set func.
uint8_t i;

//dds reg. addr. table
const uint8_t CFR1=0x00;    //4B
const uint8_t CFR2=0x01;    //4B
const uint8_t CFR3=0x02;    //4B
const uint8_t AUX=0x03;     //4B
const uint8_t PROF0=0x0E;   //8B

//PLL
uint32_t data[6]={0x3C0000,0x8008011,0x4E42,0x4B3,0x9C803C,0x580005};  //R0,1,2,3,4,5 1500MHz setting
void pll_send(uint32_t in); //pll set func.

int main(){
    ioup=0;
    rst=1;
    for(i=1;i<=4;++i) cs_hi(i); //CS init
    spi.format(8,0);           //spi mode setting. 2byte(16bit) transfer, mode 0
    rst=0;

    //pll set
    for(i=0;i<5;++i){
        pll_send(data[5-i]);
        thread_sleep_for(3);
    }
    thread_sleep_for(20);
    pll_send(data[0]);

    //dds init
    for(i=1;i<=4;++i){
        dds_init(i);
    }
    ioup=1;
    ioup=0;

    while (true){
        for(i=1;i<=4;++i){
            buf_read(buf_size);//uart buf read
            buf2val();
            waveset(i,freq,pha,ampl);
        }
        ioup=1;
        ioup=0;
    }
}

//uart char number read func.
void buf_read(uint8_t num){
    char local_buf[1];
    uint8_t i;
    for (i=0;i<num;++i){
        uart0.read(local_buf,1);
        read_buf[i]=local_buf[0];
    }
}

//buf to val change func.
void buf2val(){
    uint8_t i,j;
    uint32_t pow10;
    freq=0;
    pha=0;
    ampl=0;
    for(i=0;i<9;++i){
        pow10=1;
        for(j=0;j<8-i;++j){
            pow10=10*pow10;
        }
        freq=freq+(read_buf[i]-48)*pow10;
    }
    for(i=0;i<3;++i){
        pow10=1;
        for(j=0;j<2-i;++j){
            pow10=10*pow10;
        }
        pha=pha+(read_buf[i+9]-48)*pow10;
    }
    for(i=0;i<2;++i){
        pow10=1;
        for(j=0;j<1-i;++j){
            pow10=10*pow10;
        }
        ampl=ampl+(read_buf[i+13]-48)*pow10;
    }
    if(read_buf[12]==43)ampl=ampl;
    else if(read_buf[12]==45)ampl=-1*ampl;
}

//cs control func.
void cs_hi(uint8_t num){
    if(num==1) cs1=1;
    else if(num==2) cs2=1;
    else if(num==3) cs3=1;
    else if(num==4) cs4=1;
}
void cs_lo(uint8_t num){
    if(num==1) cs1=0;
    else if(num==2) cs2=0;
    else if(num==3) cs3=0;
    else if(num==4) cs4=0;
}

//pll set func.
void pll_send(uint32_t in){
    uint8_t buf;
    cs5=0;
    buf=(in>>24)&0xff;
    spi.write(buf);
    buf=(in>>16)&0xff;
    spi.write(buf);
    buf=(in>>8)&0xff;
    spi.write(buf);
    buf=(in>>0)&0xff;
    spi.write(buf);
    cs5=1;
}

//dds init func.
void dds_init(uint8_t ch){
    uint8_t buf;
    cs_lo(ch);
    buf=CFR1; 
    spi.write(buf);
    buf=0x00;
    spi.write(buf);
    buf=0<<6;       //inverse sinc disable
    spi.write(buf);
    buf=1<<5;       //accum. auto clear
    spi.write(buf);
    buf=0x00;
    spi.write(buf);
    cs_hi(ch);

    cs_lo(ch);
    buf=CFR2;       //ASF enable
    spi.write(buf);
    buf=0x01;
    spi.write(buf);
    buf=0x40;
    spi.write(buf);
    buf=0x08;
    spi.write(buf);
    buf=0x20;
    spi.write(buf);
    cs_hi(ch);

    cs_lo(ch);
    buf=CFR3;       //clk input divider disable
    spi.write(buf);
    buf=0x1f;
    spi.write(buf);
    buf=0x3f;
    spi.write(buf);
    buf=0x40+(1<<7);
    spi.write(buf);
    buf=0x00;
    spi.write(buf);
    cs_hi(ch);
}

//dds wave set func.
void waveset(uint8_t ch, uint32_t freq, uint16_t pha, int8_t ampl){
    uint8_t buf;
    uint32_t reg;
    float ampl_f;
    if(freq>600000000)freq=600000000;
    if(pha>360)pha=360;
    if(ampl>=0)ampl=0;
    if(ampl<=-80)ampl=-80;
    
    ampl_f=(float)ampl;
    reg=(uint32_t)15500*sqrt(pow(10,ampl_f/10));     //2/sqrt(10)*sqrt(pow(10,dBm/10))*...;
    cs_lo(ch);
    buf=PROF0;          //addr
    spi.write(buf);
    buf=(reg>>8)&0x3f;  //asf
    spi.write(buf);
    buf=reg&0xff;       //asf
    spi.write(buf);

    reg=pha*65536/360;
    buf=(reg>>8)&0xff;  //pow
    spi.write(buf);
    buf=reg&0xff;       //pow
    spi.write(buf);

    reg=freq*res/mclk;
    buf=(reg>>24)&0xff; //ftw
    spi.write(buf);
    buf=(reg>>16)&0xff; //ftw
    spi.write(buf);
    buf=(reg>>8)&0xff;  //ftw
    spi.write(buf);
    buf=reg&0xff;       //ftw
    spi.write(buf);
    cs_hi(ch);
}
