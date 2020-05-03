#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

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

int parity(int x, int size)
{
    int i;
    int p = 0;
    x = (x & ((1<<size)-1));
    for (i=0; i<size; i++)
    {
        if (x & 0x1) p++;
        x = x >> 1;
    }
    return (0 == (p & 0x1));
}

   void UnimplementedInstruction(State8080* state)    
   {    
    state->pc-=1;
    unsigned char *opcode = &state->memory[state->pc];
    printf ("Error: Unimplemented instruction\n");    
    printf("%04x", *opcode);
    exit(1);    
   }

   void setFlagsArith(State8080* state, uint16_t answer)
   {
        //Check out 0x80 for an elaborated ADD
        state->cc.z = ((answer & 0xFF) == 0);
        state->cc.s = ((answer & 0x80) != 0);
        state->cc.cy= (answer > 0xFF);
        state->cc.p = parity(answer, 8)  ; 
   }

   int Emulate8080Op(State8080* state)
   {
    unsigned char *opcode = &state->memory[state->pc];
    int status = 0;

    state->pc+=1;
    //notes because I'm dumb
    //0x80 is 10000000 in binary, we can use this to figure out the sign
    //0xFF is 11111111 in binary, we can use this to mask any bits higher than 8
    switch(*opcode)
    {
        case 0x00: printf("NOP"); break;
        case 0x01:
            //LXI B
            state->c = opcode[1];
            state->b = opcode[2];
            state->pc +=2;
            break;
        case 0x05:{
            //DCR B
            uint8_t result = state->b -1;
            state-> cc.z = (result == 0);
            state-> cc.s = (0x80 == (result & 0x80));
            state-> cc.p = parity(result, 8 );
            state->b = result;
        }
            break;
        case 0x06:
            //MVI B
            state-> b = opcode[1];
            state-> pc++;
            break;
        case 0x11:
            //LXI D
            state->e = opcode[1];
            state->d = opcode[2];
            state->pc += 2;
            break;
        case 0x13:
            //INX D
            state->d += ((++state->e) == 0)? 0:1;
            break;
        case 0x1a:{
            //LDAX D
            //sets A to memory location stored in DE
            uint16_t offset = (state->d << 8) | state-> e;
            state-> a = state->memory[offset];  
        }
        break;
        case 0x21:
            //LXI H
            state->l = opcode[1];
            state->h = opcode[2];
            state->pc += 2;
            break;
        case 0x23:
            //INX H increment l, if that's 0, increment h
            state->h += ((++state->l) == 0)? 0:1;
            break;
        case 0x31:
            //LXI SP
            //Chars are 8 bit (opcode[2] << 8) | opcode[1]; so this little tidbit combines both into a 16 bit space to use as one value (I think lol)
            state->sp = (opcode[2] << 8 | opcode[1]);
            state->pc += 2;
            break;
        case 0x32:{
            //STA adr defined in next two opcodes
            uint16_t offset = (opcode[2] << 8) | opcode[1];
            state->memory[offset] = state->a;
            state->pc+=2;
        }
        break;
        case 0x5e:{
            //MOV E,M
            uint16_t offset = (state-> h << 8) | (state->l);
            state->e = state->memory[offset];
        }
        break;
        case 0x77:{
            //MOV M,A
            //sets memory location defined in M (HL) to the value stored in A 
            uint16_t offset = (state-> h << 8) | state->l;
            state->memory[offset] = state->a;
        }
        break;
        case 0x7e:{
            //MOV A,M
            uint16_t offset = (state->h<<8) | (state->l);
            state->a = state->memory[offset];
        }
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
            // parity is handled by a subroutine    
            state->cc.p = parity( answer, 8 );    
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
        case 0xc2: {
            //JNZ 
            if(0 == state->cc.z)
                //set pc to the address in the next opcode
                state->pc = (opcode[2] << 8) | opcode[1];
            else 
                state->pc +=  2;
        }
        break;
        case 0xc3: 
            //JMP to memory address
            state->pc = (opcode[2] << 8 ) | opcode[1];
            break;
        case 0xc6: {
            //ADI byte
            //adds the next byte to A
            uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1];
            setFlagsArith(state, answer);
        }
        break;
        case 0xcd: {
            //CALL retrives memory address pushes it onto stack, and then jumps to it
            uint16_t ret = state-> pc +2;
            state->memory[state->sp-1] = (ret >> 8) & 0xff;
            state->memory[state->sp-2] = (ret & 0xff);
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
            state->cc.p = parity(x, 8);    
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
            state->cc.s = (0x80 == (x & 0x80));
            state->cc.p = parity(x,8);
            //if a is less than opcode we need to set the carry flag
            state->cc.cy = (state->a < opcode[1]);
            state->pc++;
        }
        break;
        case 0xc1:                      //POP B puts top two bytes into register pair
            {    
                state->c = state->memory[state->sp];    
                state->b = state->memory[state->sp+1];    
                state->sp += 2;    
            }    
        break;   
        case 0xc5:  //PUSH B puts register pair on top of the stack
            {
                state->memory[state->sp-1] = state->b;
                state->memory[state->sp-2] = state->c;
                state->sp = state->sp - 2;
            }
        break;
        case 0xf1:                      //POP PSW    PSW is a special register made of acumulator and register flags
            {    
                state->a = state->memory[state->sp+1];    
                uint8_t psw = state->memory[state->sp];    
                state->cc.z  = (0x01 == (psw & 0x01));    
                state->cc.s  = (0x02 == (psw & 0x02));    
                state->cc.p  = (0x04 == (psw & 0x04));    
                state->cc.cy = (0x05 == (psw & 0x08));    
                state->cc.ac = (0x10 == (psw & 0x10));    
                state->sp += 2;    
            }    
            break;    
        case 0xf5:                      //PUSH PSW    
            {    
            state->memory[state->sp-1] = state->a;    
            uint8_t psw = (state->cc.z |    
                            state->cc.s << 1 |    
                            state->cc.p << 2 |    
                            state->cc.cy << 3 |    
                            state->cc.ac << 4 );    
            state->memory[state->sp-2] = psw;    
            state->sp = state->sp - 2;    
            }    
            break;    
        default: {
            UnimplementedInstruction(state); 
            status = 1;
        }
        break;

    }

    return status;

}

void ReadFileIntoMemoryAt(State8080* state, char* filename, uint32_t offset)
{
    FILE *f= fopen(filename, "rb");
    if (f==NULL)
    {
        printf("error: Couldn't open %s\n", filename);
        exit(1);
    }
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    
    uint8_t *buffer = &state->memory[offset];
    fread(buffer, fsize, 1, f);
    fclose(f);
}
State8080* Init8080(void)
{
    State8080* state = calloc(1,sizeof(State8080));
    state->memory = malloc(0x10000);  //16K
    return state;
}   

int main (int argc, char**argv)
{
    printf("starting");
    int done = 0;
    int vblankcycles = 0;
    State8080* state = Init8080();
    
    ReadFileIntoMemoryAt(state, "SI-rom/invaders.h", 0);
    ReadFileIntoMemoryAt(state, "SI-rom/invaders.g", 0x800);
    ReadFileIntoMemoryAt(state, "SI-rom/invaders.f", 0x1000);
    ReadFileIntoMemoryAt(state, "SI-rom/invaders.e", 0x1800);
    
    while (done == 0)
    {
        done = Emulate8080Op(state);
    }
    return 0;
}