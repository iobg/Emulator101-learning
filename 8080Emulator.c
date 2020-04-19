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

   void setFlagsArith(State8080* state, uint16_t answer)
   {
        //Check out 0x80 for an elaborated ADD
        state->cc.z = ((answer & 0xFF) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy= (answer > 0xFF);
        state->cc.p = Parity(answer&0xFF); 
   }

   int Emulate8080Op(State8080* state){
    unsigned char *opcode = &state->memory[state->pc];

    //notes because I'm dumb
    //0x80 is 10000000 in binary, we can use this to figure out the sign
    //0xFF is 11111111 in binary, we can use this to mask any bits higher than 8

    switch(*opcode){
        case 0x00: break;
        case 0x01:
            //LXIt
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
            //the & 0xFF masks to keep only the last 8 bits
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
        break;
        case 0x81: {
            //ADD C
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->c;
            setFlagsArith(state, answer);
        }
        break;
        case 0x86: {
            //ADD M
            //adds byte pointed to by HL register pair
            uint16_t offset = (state->h<<8) | (state->l);    
            uint16_t answer = (uint16_t) state->a + state->memory[offset];
            setFlagsArith(state, answer);

        }
        break;
        case 0xC6: {
            //ADI byte
            //adds the next byte to A
            uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1];
            setFlagsArith(state, answer);
        }
        break;
        case 0xc2: 
            //JNZ 
            if(0 == state->cc.z)
                //set pc to the address in the next opcode
                state->pc = (opcode[2] << 8) | opcode[1];
            else 
                state->pc +=  2;
        break;
        case 0xc3: 
            state->pc = (opcode[2] << 8 ) | opcode[1];
        break;

        case 0xcd: {
            //CALL retrives memory address pushes it onto stack, and then jumps to it
            uint16_t ret = state-> pc +2;
            state->memory[state->sp-1] = (ret >> 8) & 0xFF;
            state->memory[state->sp-2] = (ret & 0xFF);
            state->sp = state-> sp-2;
            state->pc = (opcode[2] << 8) | opcode[1];
        }
        break;

        case 0xc9: 
            //RET gets address off of the stack and stores it to PC
            state-> pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
        break;
        case 0x2F:
            //CMA aka NOT
            //reverses bits
            state->a = ~state->a;
        break;
        case 0xe6: {
            //ANI
            uint8_t x = state->a & opcode[1];
            state->cc.z = (x == 0);    
            state->cc.s = (0x80 == (x & 0x80));    
            state->cc.p = Parity(x, 8);    
            state->cc.cy = 0;           //Data book says ANI clears CY    
            state->a = x; 
            state->pc++;
        }
        break;
        //bit rotating instructions (can be used for multiplication and division)
        case 0x0F: {
            //RRC
            uint8_t x = state->a;
            state->a = ((x&1) << 7) | (x >> 1);
            state-> cc.cy = (1 == (x&1));
        }
        break;

        case 0x1F: {
            //RAR
            uint8_t x = state->a;
            state->a = ((x&state->cc.cy) << 7) | (x >> 1);
            state-> cc.cy = (1 == (x&1));      
        }
        break;

        case 0xfe: {
            //CPI
            //Checks for equality by subtracting and checking for 0
            uint8_t x = state->a - opcode[1];
            state->cc.z = (x == 0);
            state->cc.s = (0x80 == (x & 0x80))
            state->cc.p = Parity(x,8);
            //if a is less than opcode we need to set the carry flag
            state->cc.cy = (state->a < opcode[1])
            state->pc++;
        }
        break;


        default: UnimplementedInstruction(state); break;

    }


   }