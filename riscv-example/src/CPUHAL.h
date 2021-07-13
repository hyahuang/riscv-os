#ifndef CPUHSL_H
#define CPUHAL_H

#include <stdint.h>

typedef uint32_t TCPUInterruptState, *TCPUInterruptStateRef;
typedef uint32_t *TCPUStackRef;
typedef uint32_t (*TCPUContextEntry)(void *param);


__attribute__((always_inline)) inline TCPUInterruptState CPUHALSuspendInterrupts(void){
    uint32_t result;
    asm volatile ("csrrci %0, mstatus, 0x8": "=r"(result));
    return result;
}
__attribute__((always_inline)) inline void CPUHALResumeInterrupts(TCPUInterruptState state){
     asm volatile ("csrs mstatus, %0" : : "r"(state & 0x8));
}

__attribute__((always_inline)) inline void CPUHALEnableInterrupts(void){
    asm volatile ("csrsi mstatus, 0x8");
}

__attribute__((always_inline)) inline void CPUHALDisableInterrupts(void){
    asm volatile ("csrci mstatus, 0x8");
}

TCPUStackRef CPUHALContextInitialize(TCPUStackRef stacktop, TCPUContextEntry entry, void *param);
void CPUHALContextSwitch (TCPUStackRef *storecurrent, TCPUStackRef restore);

#endif