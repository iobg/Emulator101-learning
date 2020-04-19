#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

   typedef struct ConditionCodes {    
    //zero flag
    uint8_t    z:1;    
    //sign flag
    uint8_t    s:1;    
    //parity flag
    uint8_t    p:1;    
    //carry flag
    uint8_t    cy:1;    
    //auxillary carry
    uint8_t    ac:1;    
    uint8_t    pad:3;    
   } ConditionCodes;

   typedef struct State8080 {    
    //registers
    uint8_t    a;    
    uint8_t    b;    
    uint8_t    c;    
    uint8_t    d;    
    uint8_t    e;    
    uint8_t    h;    
    uint8_t    l;    
    //stack pointer
    uint16_t    sp;    
    uint16_t    pc;    
    uint8_t     *memory;    
    struct      ConditionCodes      cc;    
    uint8_t     int_enable;    
   } State8080;    

   void UnimplementedInstruction(State8080* state)    
   {    
    state->pc-=1;
    printf ("Error: Unimplemented instruction\n");    
    exit(1);    
   }

   void genericADD(State8080* state, uint16_t answer)
   {
        //Check out 0x80 for an elaborated ADD
        state->cc.z = ((answer & 0xFF) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy= (answer > 0xFF);
        state->cc.p = Parity(answer&0xFF); 
   }

   int Emulate8080Op(State8080* state){
    unsigned char *opcode = &state->memory[state->pc];

    switch(*opcode){
        case 0x00: break;
        case 0x01:
            state->c = opcode[1];
            state->b = opcode[2];
            state->pc +=2;
            break;
        case 0x80: {
            //ADD B
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->b;
            // Zero flag: if the result is zero,    
            // set the flag to zero    
            // else clear the flag   
            //the & 0xFF masks to the least significant bit
            if((answer & 0xFF) == 0)
                state->cc.z = 1;
            else 
                state->cc.z = 0;
            // Sign flag: if bit 7 is set,    
            // set the sign flag    
            // else clear the sign flag
            //0x80 masks to bit 7 where the sign would be set
            if (answer & 0x80)    
                state->cc.s = 1;    
            else    
                state->cc.s = 0; 
            // Carry flag    
            if (answer > 0xff)    
                state->cc.cy = 1;    
            else    
                state->cc.cy = 0;     
            // Parity is handled by a subroutine    
            state->cc.p = Parity( answer & 0xff);    
            state->a = answer & 0xff;  
        }
        case 0x81: {
            //ADD C
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->c;
            genericADD(state, answer);
        }
        case 0xC6: {
            //ADI byte
            //adds the next byte to A
            uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1];
            genericADD(state, answer)
        }
        case 0x86: {
            //ADD M
            //adds byte pointed to by HL register pair
            uint16_t offset = (state->h<<8) | (state->1);
            uint16_t answer = (uint16_t) state->a + state=>memory[offset];
            genericADD(state, answer);
            

        }


        default: UnimplementedInstruction(state); break;

    }


   }