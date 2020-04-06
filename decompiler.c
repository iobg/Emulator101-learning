#include <stdio.h>
#include <stdlib.h>


int Disassemble8080Op(unsigned char *codebuffer, int pc){
	unsigned char *code = &codebuffer[pc];
	int opbytes = 1; 
	// printf("%04x ", pc);
	switch (*code)
	{
		case 0x00: printf("NOP"); break;
	}
	return opbytes;
}

int main(void){

//open file, to pointer of fp
FILE *fp = fopen("./SI-rom/invaders.h", "r");

	if(fp == NULL){
		printf("file could not be opened");
		exit(1);
	}
	//seek to the end of the file to determine byte size
	fseek(fp, 0, SEEK_END);
	int fsize = ftell(fp);
	//seek back to the beginning of the file
	fseek(fp, 0L, SEEK_SET);

	//allocate a buffer equal to the size of the file
	unsigned char *buffer=malloc(fsize);

	//populate the buffer with the file contents
	fread(buffer,  fsize, 1, fp);
	fclose(fp);

	int pc = 0;

	//pretty sure you know what this does
	while(pc < fsize)
	{
		pc += Disassemble8080Op(buffer,pc);
	}
	
}





//starting at mem address B, set B and then C to their respective bytes (3,2) and then increment by 3 instead of 1 to skip the next two bytes
// case 0x01: printf("LXI    B,#$%02x%02x", code[2], code[1]); opbytes=3; break; 


