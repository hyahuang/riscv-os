# Design a basic operating system for riscv-console
Implement multiple system calls from your OS design.
* Launch the application program when cartridge is entered
* Access multi-button controllerinput
* React to periodic timer
* React to video interrupt
* Draw to the background, large and small sprites

# List some functions of the Operating System

 

1. my_printf: print string on the screen in text mode (implement using C library)
    ### Demo:
    ![preview](https://imgur.com/Tumx4zu.png)
 


2. move sprite:
    ### Demo:
    ![preview](https://i.imgur.com/qgTSQ9n.png)
    Press w,a,s,d can move large and small sprite.
    Ex. Press a Large sprite moves left, small sprite moves right.

3. thread_initiallize: initialize a thread.
    My thread function in cartridge
    ### Demo:
    ![preview](https://i.imgur.com/utewlfJ.png)
    
    Number after “Cartridge:” is the timer tick. The OS will do context switch every 500 timer ticks.



4. malloc:
    implement _sbrk in cartridge and include c library. This allows me to allocate memory space in the cartridge’s memory space.


    ### Demo:
    ![preview](https://i.imgur.com/Motnjiu.png)

  


