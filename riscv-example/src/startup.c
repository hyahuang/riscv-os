#include <stdint.h>
#include <stdio.h>
#include "CPUHAL.h"
extern uint8_t _erodata[];
extern uint8_t _data[];
extern uint8_t _edata[];
extern uint8_t _sdata[];
extern uint8_t _esdata[];
extern uint8_t _bss[];
extern uint8_t _ebss[];

typedef void (*Tfunc)(void);
typedef struct{
    uint32_t palette:2;
    uint32_t offsetX:10;
    uint32_t offsetY:9;
    uint32_t width:5;
    uint32_t hight:5;
}LScontrol;
typedef struct{
    uint32_t palette:2;
    uint32_t offsetX:10;
    uint32_t offsetY:9;
    uint32_t width:4;
    uint32_t hight:4;
    uint32_t z:3;
}SScontrol;
typedef struct{
    uint32_t palette:2;
    uint32_t offsetX:10;
    uint32_t offsetY:10;
    uint32_t z:3;
}BGcontrol;

typedef int bool;  
#define true 1
#define false 0

void timer_interrupt();
void cmd_interrupt();
void video_interrupt();
void cart_interrupt();
void illegal_inst_interrupt();
void setBGcolor(uint32_t color);

// Adapted from https://stackoverflow.com/questions/58947716/how-to-interact-with-risc-v-csrs-by-using-gcc-c-code
__attribute__((always_inline)) inline uint32_t csr_mstatus_read(void){
    uint32_t result;
    asm volatile ("csrr %0, mstatus" : "=r"(result));
    return result;
}

__attribute__((always_inline)) inline void csr_mstatus_write(uint32_t val){
    asm volatile ("csrw mstatus, %0" : : "r"(val));
}

__attribute__((always_inline)) inline void csr_write_mie(uint32_t val){
    asm volatile ("csrw mie, %0" : : "r"(val));
}

__attribute__((always_inline)) inline void csr_enable_interrupts(void){
    asm volatile ("csrsi mstatus, 0x8");
}

__attribute__((always_inline)) inline void csr_disable_interrupts(void){
    asm volatile ("csrci mstatus, 0x8");
}
__attribute__((always_inline)) inline uint32_t csr_mcause_read(void){
    uint32_t result;
    asm volatile ("csrr %0, mcause" : "=r"(result));
    return result;
}
__attribute__((always_inline)) inline void csr_write_mepc(uint32_t val){
    asm volatile ("csrw mepc, %0" : : "r"(val));
}
__attribute__((always_inline)) inline uint32_t csr_mepc_read(void){
    uint32_t result;
    asm volatile ("csrr %0, mepc" : "=r"(result));
    return result;
}

#define MTIME_LOW       (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH      (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW    (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH   (*((volatile uint32_t *)0x40000014))
#define CONTROLLER      (*((volatile uint32_t *)0x40000018))
#define CARTIDGE_STATUS (*((volatile uint32_t *)0x4000001c))
#define MODE_CONTROL    (*((volatile uint32_t *)0x500FF414))


#define CIS                    0x00000001
#define TIMER_INTERRUPT        0x80000007
#define EXTERNAL_INTERRUPT     0x8000000b
#define ILLEGAL_INST_INTERRUPT 0x00000002
#define RESTART_ADDRESS        0x00000000
#define COMMAND_BIT            0x00000004
#define VIDEO_BIT              0x00000002
#define CART_BIT               0x00000001
//system call number
#define SYSTIMER               0x00000001
#define GET_CONTROLLER_STATUS  0x00000002
#define MOVE_LARGE_SPRITE      0x00000003
#define WRITE_TEXT             0x00000004
#define SYSVIDEO               0x00000005
#define MOVE_SMALL_SPRITE      0x00000006
#define THREAD_INITIALLIZE     0x00000007
#define SET_BG_COLOR           0x00000008

volatile LScontrol *LS_control =  (volatile LScontrol*)(0x500FF114);
volatile SScontrol *SS_control =  (volatile SScontrol*)(0x500FF214);
volatile BGcontrol *BG_control =  (volatile BGcontrol*)(0x500FF100);
volatile uint32_t *BGpalette0 = (volatile uint32_t *)0x500FC000;
volatile uint32_t  *IER = (volatile uint32_t *)(0x40000000);
volatile uint32_t  *IPR = (volatile uint32_t *)(0x40000004);
//Thread variable
uint32_t ThreadStack[2048];
TCPUStackRef ThreadPointers[2];
bool hasThread = false;
int run = 0 , store = 1;


void init(void){
    uint8_t *Source = _erodata;
    uint8_t *Base = _data < _sdata ? _data : _sdata;
    uint8_t *End = _edata > _esdata ? _edata : _esdata;

    while(Base < End){
        *Base++ = *Source++;
    }
    Base = _bss;
    End = _ebss;
    while(Base < End){
        *Base++ = 0;
    }

    csr_write_mie(0x888);       // Enable all interrupt soruces
    csr_enable_interrupts();    // Global interrupt enable
    MTIMECMP_LOW = 1;
    MTIMECMP_HIGH = 0;
    *IER = *IER|COMMAND_BIT;
    *IER = *IER|VIDEO_BIT;
    *IER = *IER|CART_BIT ;

    // palette initialized
    volatile uint32_t* palette0 = (volatile uint32_t *)0x500FD000;
    volatile uint32_t* palette1 = (volatile uint32_t *)0x500FD400;

    uint32_t color = 0xff0000ff;
    for(int i=0;i<256;i++){
        *palette0 = color--;
        palette0+=1;
    }
    color = 0xffff0000;
    for(int i=0;i<256;i++){
        *palette1 = color--;
        palette1+=1;
    }

    LS_control[0].palette = 0;
    LS_control[0].offsetX = 64;
    LS_control[0].offsetY = 64;
    LS_control[0].width = 15;
    LS_control[0].hight = 15;
    LS_control[1].palette = 1;
    LS_control[1].offsetX = 64;
    LS_control[1].offsetY = 64;
    LS_control[1].width = 5;
    LS_control[1].hight = 5;
    SS_control[0].palette = 0;
    SS_control[0].offsetX = 512;
    SS_control[0].offsetY = 200;
    SS_control[0].width = 10;
    SS_control[0].hight = 10;
    SS_control[0].z = 5;
    BG_control[0].palette = 0;
    BG_control[0].offsetX = 512;
    BG_control[0].offsetY = 288;
    BG_control[0].z = 0;
    // palette initialized end

}

volatile int global = 0;
volatile int video = 0;
volatile char *VIDEO_MEMORY = (volatile char *)(0x50000000 + 0xFE800);
void c_interrupt_handler(void){
    uint32_t mcause = csr_mcause_read();
    switch(mcause){
        case TIMER_INTERRUPT:
            timer_interrupt();
            break;
        case ILLEGAL_INST_INTERRUPT:
            illegal_inst_interrupt();
            break;
        case EXTERNAL_INTERRUPT:{
           if(*IPR & COMMAND_BIT){
            cmd_interrupt();
            return;
            }
            if(*IPR & VIDEO_BIT){
                video_interrupt();
                return;
            }
            if(*IPR & CART_BIT){
                cart_interrupt();
                return;
            }
        } 
        default: 
            break;  
    }
}

void illegal_inst_interrupt(){
    csr_write_mepc(RESTART_ADDRESS);
}

void timer_interrupt(){
    uint64_t NewCompare = (((uint64_t)MTIMECMP_HIGH)<<32) | MTIMECMP_LOW;
    NewCompare += 100;
    MTIMECMP_HIGH = NewCompare>>32;
    MTIMECMP_LOW = NewCompare;
    global++;
    if(global%500 == 0 && hasThread){
        switch (run){
            case 1:
                run = 0;
                store = 1;
                break;
            case 0:
                run = 1;
                store = 0;
                break;
        }
        uint32_t mepc  = csr_mepc_read();
        printf("\n");
        TCPUInterruptState PrevState = CPUHALSuspendInterrupts();
        CPUHALContextSwitch(&ThreadPointers[store],ThreadPointers[run]);
        csr_write_mepc(mepc);
        CPUHALResumeInterrupts(PrevState);
    }
    if (global == 9999){
        global =0;
    }

}

void cmd_interrupt(){
    if(MODE_CONTROL&1){
        MODE_CONTROL = 0x00000000;
    }
    else{
        MODE_CONTROL = 0x00000001;
    }
    *IPR = *IPR&COMMAND_BIT;
}

void video_interrupt(){
    video++;
    if(video >= 1000){
        video = 0;
    }
    *IPR = *IPR&VIDEO_BIT;
}
void cart_interrupt(){
    if(CARTIDGE_STATUS&CIS){
        *IPR = *IPR & CART_BIT;
    }
    else{
        *IPR = *IPR & CART_BIT;
    }
}

uint32_t c_syscall(uint32_t param1,uint32_t param2, uint32_t param3, uint32_t param4){
    switch (param1){
        case SYSTIMER:
            return global;
            break;
        case GET_CONTROLLER_STATUS:
            return CONTROLLER;
            break;
        case MOVE_LARGE_SPRITE:
            LS_control[param2].offsetX+= param3;
            LS_control[param2].offsetY+= param4;
            break;
        case MOVE_SMALL_SPRITE:
            SS_control[param2].offsetX+= param3;
            SS_control[param2].offsetY+= param4;
            break;
        case WRITE_TEXT:{
            printf((char* )param2,(int)param3);
            fflush(stdout);
            }
            break;
        case SYSVIDEO:
            return video;
        case THREAD_INITIALLIZE:
            ThreadPointers[1] = CPUHALContextInitialize((TCPUStackRef)(ThreadStack+2048), (TCPUContextEntry)param2, (void*)param3);
            hasThread = true;
            break;
        case SET_BG_COLOR:
            setBGcolor(param2);
            break;
        default:
            break;
    }

}

void setBGcolor(uint32_t color){
    for(int i=0;i<256;i++){
        *BGpalette0 = color--;
        BGpalette0+=1;
    }
    BGpalette0 = (volatile uint32_t *)0x500FC000;
}