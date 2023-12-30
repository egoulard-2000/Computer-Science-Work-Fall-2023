/*
 * Author: Daniel Kopta
 * Updated by: Erin Parker
 * CS 4400, University of Utah
 *
 * Simulator handout
 * A simple x86-like processor simulator.
 * Read in a binary file that encodes instructions to execute.
 * Simulate a processor by executing instructions one at a time and appropriately 
 * updating register and memory contents.
 *
 * Some code and pseudo code has been provided as a starting point.
 *
 * Completed by: Emile Goulard
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "instruction.h"

// Forward declarations for helper functions
unsigned int get_file_size(int file_descriptor);
unsigned int* load_file(int file_descriptor, unsigned int size);
instruction_t* decode_instructions(unsigned int* bytes, unsigned int num_instructions);
unsigned int execute_instruction(unsigned int program_counter, instruction_t* instructions, 
				 int* registers, unsigned char* memory);
void print_instructions(instruction_t* instructions, unsigned int num_instructions);
void error_exit(const char* message);

// 17 registers
#define NUM_REGS 17
// 1024-byte stack
#define STACK_SIZE 1024
// Most-Significant Bit
#define MSB 0x80000000

int main(int argc, char** argv)
{
  // Make sure we have enough arguments
  if(argc < 2)
    error_exit("must provide an argument specifying a binary file to execute");

  // Open the binary file
  int file_descriptor = open(argv[1], O_RDONLY);
  if (file_descriptor == -1) 
    error_exit("unable to open input file");

  // Get the size of the file
  unsigned int file_size = get_file_size(file_descriptor);
  // Make sure the file size is a multiple of 4 bytes
  // since machine code instructions are 4 bytes each
  if(file_size % 4 != 0)
    error_exit("invalid input file");

  // Load the file into memory
  // We use an unsigned int array to represent the raw bytes
  // We could use any 4-byte integer type
  unsigned int* instruction_bytes = load_file(file_descriptor, file_size);
  close(file_descriptor);

  unsigned int num_instructions = file_size / 4;


  /****************************************/
  /**** Begin code to modify/implement ****/
  /****************************************/

  // Allocate and decode instructions (left for you to fill in)
  instruction_t* instructions = decode_instructions(instruction_bytes, num_instructions);

  // Optionally print the decoded instructions for debugging
  // Will not work until you implement decode_instructions
  // Do not call this function in your submitted final version
  // print_instructions(instructions, num_instructions);


  // Once you have completed Part 1 (decoding instructions), uncomment the below block

  // Allocate and initialize registers
  int* registers = (int*)malloc(sizeof(int) * NUM_REGS);
  for (int i = 0; i < NUM_REGS; i++)
    registers[i] = 0;

  // Initialize register values
  registers[6] = STACK_SIZE;

  // Stack memory is byte-addressed, so it must be a 1-byte type
  unsigned char* memory = (unsigned char*)malloc(sizeof(char) * STACK_SIZE);
  for (int i = 0; i < STACK_SIZE; i++)
    memory[i] = 0;

  // Run the simulation
  unsigned int program_counter = 0;

  // program_counter is a byte address, so we must multiply num_instructions by 4 
  // to get the address past the last instruction
  while(program_counter != num_instructions * 4)
  {
    program_counter = execute_instruction(program_counter, instructions, registers, memory);
  }
  
  return 0;
}

/*
 * Decodes the array of raw instruction bytes into an array of instruction_t
 * Each raw instruction is encoded as a 4-byte unsigned int
*/
instruction_t* decode_instructions(unsigned int* bytes, unsigned int num_instructions)
{
  // Initialize retval
  instruction_t* retval = (instruction_t*)malloc(sizeof(instruction_t) * num_instructions);

  // Fill in Fields
  for (int i = 0; i < num_instructions; i++)
  {
    // 0x1F mask is 31 bits
    retval[i].opcode = (bytes[i] >> 27) & 0x1F;
    retval[i].first_register = (bytes[i] >> 22) & 0x1F;
    retval[i].second_register = (bytes[i] >> 17) & 0x1F;
    retval[i].immediate = bytes[i] & 0x0000FFFF;
  }
    
  return retval;
}


/*
 * Executes a single instruction and returns the next program counter
*/
unsigned int execute_instruction(unsigned int program_counter, instruction_t* instructions, int* registers, unsigned char* memory)
{
  // program_counter is a byte address, but instructions are 4 bytes each
  // divide by 4 to get the index into the instructions array
  instruction_t instr = instructions[program_counter / 4];

  // Registers for comparisons
  long sreg1 = (long)registers[instr.first_register];
  long sreg2 = (long)registers[instr.second_register];
  unsigned long ureg1 = (unsigned long)registers[instr.first_register];
  unsigned long ureg2 = (unsigned long)registers[instr.second_register];

  // Comparison Vars
  long signedComp = sreg2 - sreg1;
  unsigned long unsignedComp = ureg2 - ureg1;

  // Sign-Extension Var
  unsigned int signEx = 0;
  
  switch(instr.opcode)
  {
  case subl: // 0
    registers[instr.first_register] = registers[instr.first_register] - instr.immediate;
    break;
  case addl_reg_reg: // 1
    registers[instr.second_register] = registers[instr.first_register] + registers[instr.second_register];
    break;
  case addl_imm_reg: // 2
    registers[instr.first_register] = registers[instr.first_register] + instr.immediate;
    break;
  case imull: // 3
    registers[instr.second_register] = registers[instr.first_register] * registers[instr.second_register];
    break;
  case shrl: // 4
    registers[instr.first_register] = (unsigned int)registers[instr.first_register] >> 1;
    break;
  case movl_reg_reg: // 5
    registers[instr.second_register] = registers[instr.first_register];
    break;
  case movl_deref_reg: // 6
    registers[instr.second_register] = *((int*)(&memory[registers[instr.first_register] + instr.immediate]));
    break;
  case movl_reg_deref: // 7
    *((int*)(&memory[registers[instr.second_register] + instr.immediate])) = registers[instr.first_register];
    break;
  case movl_imm_reg: // 8
    if (instr.immediate < 0)
      signEx = 0xFFFF0000 | instr.immediate;
    else
      signEx = 0x00000000 | instr.immediate;

    registers[instr.first_register] = signEx;
    break;
  case cmpl: // 9
    registers[16] = 0;
    if (ureg2 < ureg1) // Check for unsigned overflow
      registers[16] |= 0x1; // Set CF bit on for eflags

    if (signedComp == 0)
      registers[16] |= 0x40; // Set ZF bit on for eflags

    if (unsignedComp & MSB) // Check if Most-Significant Bit is 1
      registers[16] |= 0x80; // Set SF bit on for eflags

    if (signedComp > INT32_MAX || signedComp < INT32_MIN) // Check signed overflow
      registers[16] |= 0x800; // Set OF bit on for eflags

    break;
  case je: // 10
    if (registers[16] & 0x00000040) // ZF
      return program_counter + 4 + instr.immediate;
    break;
  case jl: // 11
    if ((registers[16] & 0x00000080) ^ (registers[16] & 0x00000800)) // SF xor OF
      return program_counter + 4 + instr.immediate;
    break;
  case jle: // 12
    if ((registers[16] & 0x00000080) ^ (registers[16] & 0x00000800) || registers[16] & 0x00000040) // (SF xor OF) or ZF
      return program_counter + 4 + instr.immediate;
    break;
  case jge: // 13
    if (!((registers[16] & 0x00000080) ^ (registers[16] & 0x00000800))) // !(SF xor ZF)
      return program_counter + 4 + instr.immediate;
    break;
  case jbe: // 14
    if ((registers[16] & 0x00000001) || (registers[16] & 0x00000040)) // CF or ZF
      return program_counter + 4 + instr.immediate;
    break;
  case jmp: // 15
    return program_counter + 4 + instr.immediate;
    break;
  case call: // 16
    registers[6] -= 4;
    *((int*)(&memory[registers[6]])) = program_counter + 4;
    return program_counter + 4 + instr.immediate;
    break;
  case ret: // 17
    if (registers[6] == 1024) {
      exit(0);
    }
    else {
      program_counter = *((int*)(&memory[registers[6]]));
      registers[6] += 4;
      return program_counter;
    }
    break;
  case pushl: // 18
    registers[6] -= 4;
    *((int*)(&memory[registers[6]])) = registers[instr.first_register];
    break;
  case popl: // 19
    registers[instr.first_register] = *((int*)(&memory[registers[6]]));
    registers[6] += 4;
    break;
  case printr: // 20
    printf("%d (0x%x)\n", registers[instr.first_register], registers[instr.first_register]);
    break;
  case readr: // 21
    scanf("%d", &(registers[instr.first_register]));
    break;
  }

  // program_counter + 4 represents the subsequent instruction
  return program_counter + 4;
}


/*********************************************/
/****  DO NOT MODIFY THE FUNCTIONS BELOW  ****/
/*********************************************/

/*
 * Returns the file size in bytes of the file referred to by the given descriptor
*/
unsigned int get_file_size(int file_descriptor)
{
  struct stat file_stat;
  fstat(file_descriptor, &file_stat);
  return file_stat.st_size;
}

/*
 * Loads the raw bytes of a file into an array of 4-byte units
*/
unsigned int* load_file(int file_descriptor, unsigned int size)
{
  unsigned int* raw_instruction_bytes = (unsigned int*)malloc(size);
  if(raw_instruction_bytes == NULL)
    error_exit("unable to allocate memory for instruction bytes (something went really wrong)");

  int num_read = read(file_descriptor, raw_instruction_bytes, size);

  if(num_read != size)
    error_exit("unable to read file (something went really wrong)");

  return raw_instruction_bytes;
}

/*
 * Prints the opcode, register IDs, and immediate of every instruction, 
 * assuming they have been decoded into the instructions array
*/
void print_instructions(instruction_t* instructions, unsigned int num_instructions)
{
  printf("instructions: \n");
  unsigned int i;
  for(i = 0; i < num_instructions; i++)
  {
    printf("op: %d, reg1: %d, reg2: %d, imm: %d\n", 
	   instructions[i].opcode,
	   instructions[i].first_register,
	   instructions[i].second_register,
	   instructions[i].immediate);
  }
  printf("--------------\n");
}

/*
 * Prints an error and then exits the program with status 1
*/
void error_exit(const char* message)
{
  printf("Error: %s\n", message);
  exit(1);
}
