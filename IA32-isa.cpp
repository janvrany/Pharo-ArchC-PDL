/**
 * @file      IA32-isa.cpp
 * @author    Rodolfo Jardim de Azevedo
 *            Team 03 - MC723 - 2005, 1st period
 *              Eduardo Uemura Okada
 *              Andre Deiano Pansani
 *              Ricardo Andrade
 *            Modified by Rafael Madeira
 *              readRegister8();
 *              writeRegister8();
 *              void ac_behavior( Type_op1bi )
 *              DataManager
 *
 *            The ArchC Team
 *            http://www.archc.org/
 *
 *            Computer Systems Laboratory (LSC)
 *            IC-UNICAMP
 *            http://www.lsc.ic.unicamp.br
 *
 * @version   version?
 * @date      Mon, 19 Jun 2006 15:33:29 -0300
 * 
 * @brief     The ArchC x86 functional model.
 * 
 * @attention Copyright (C) 2002-2006 --- The ArchC Team
 *
 */

#include "IA32-isa.H"
#include "ac_isa_init.cpp"
#include <math.h>

// Register definitions to register bank GR:8
#define EAX 0
#define ECX 1
#define EDX 2
#define EBX 3
#define ESP 4
#define EBP 5
#define ESI 6
#define EDI 7 
#define AX 0
#define CX 1
#define DX 2
#define BX 3
#define SP 4
#define BP 5
#define SI 6
#define DI 7
#define AL 0
#define CL 1
#define DL 2
#define BL 3
#define AH 4
#define CH 5
#define DH 6
#define BH 7
#define STACK_BOTTOM 0x01000000
// Register definitions to register bank SR:6
#define CS 0
#define DS 1
#define SS 2
#define ES 3
#define FS 4
#define GS 5
// Register definitions to register bank SPR:2
#define EFLAGS 0
#define EIP 1
#define FLAG_CF (1)
#define FLAG_PF (1<<2)
#define FLAG_AF (1<<4)
#define FLAG_ZF (1<<6)
#define FLAG_SF (1<<7)
#define FLAG_TF (1<<8)
#define FLAG_IF (1<<9)
#define FLAG_DF (1<<10)
#define FLAG_OF (1<<11)
#define FLAG_IOPL (1<<12 | 1<<13)
#define FLAG_NT (1<<14)
#define FLAG_RF (1<<16)
#define FLAG_VM (1<<17)
#define FLAG_AC (1<<18)
#define FLAG_VIF (1<<19)
#define FLAG_VIP (1<<20)
#define FLAG_ID (1<<21)
#define FLAG_ALL 0xFFFFFFFF

// Auxiliar Flags
#define JUST_SET 0
#define RUNONCE 1
#define NOT_SET 2
#define OPER_ADD 0
#define OPER_SUB 1
#define OPER_MUL 2
#define OPER_NONE 4
#define MAXINT 0xFFFFFFFF
#define MASK_8BITS 0xFF
#define MASK_16BITS 0xFFFF
#define OPERAND_SIZE16JUST 0
#define OPERAND_SIZE16 1
#define OPERAND_SIZE32 2
#define ADDRESS_SIZE16JUST 0
#define ADDRESS_SIZE16 1
#define ADDRESS_SIZE32 2
#define INVALID_REGISTER 0xFFFFFFFF


// Output operation flags
#define VERBOSE 1
#define GENERICVERBOSE 0
#define OUTSTREAM stderr

// Auxiliar functions and data structures

static char *reg_str[8] = {"EAX","ECX","EDX","EBX","ESP","EBP","ESI","EDI"};
static char *reg_str16[8] = {"AX","CX","DX","BX","SP","BP","SI","DI"};
typedef unsigned long long uint64;
int OperandSize;
int AddressSize;

// ----------------------------------------------------------------------------
// Auxiliar output functions --------------------------------------------------
// ----------------------------------------------------------------------------

// Used to print instructions in generic format (ex."mov rm32, r32")
void acprintfg ( char *s )
{
  
  if (GENERICVERBOSE && !VERBOSE) fprintf(OUTSTREAM, "%s\n", s);
  else if (GENERICVERBOSE && VERBOSE) fprintf(OUTSTREAM, "%s\t\t", s);
}

// Used to print information
void acprintf ( char *s )
{
  if (VERBOSE) fprintf(OUTSTREAM, s);
}

// Used to print immediate data
void acprinti ( uint d )
{
  if (VERBOSE) fprintf(OUTSTREAM, "0x%08X", d);
}

void acprintfrr ( char *inst, uint reg1, uint reg2 )
{
  if (VERBOSE && (OperandSize==OPERAND_SIZE16))
    fprintf(OUTSTREAM, "%s %s, %s\n", inst, reg_str16[reg1], reg_str16[reg2] );
  else if (VERBOSE && (OperandSize==OPERAND_SIZE32))
    fprintf(OUTSTREAM, "%s %s, %s\n", inst, reg_str[reg1], reg_str[reg2] );
}

void acprintfmr ( char *inst, uint a, uint reg )
{
  if (VERBOSE && (OperandSize==OPERAND_SIZE16))
    fprintf(OUTSTREAM, "%s [0x%08X], %s\n", inst, a, reg_str16[reg] );
  else if (VERBOSE && (OperandSize==OPERAND_SIZE32))
    fprintf(OUTSTREAM, "%s [0x%08X], %s\n", inst, a, reg_str[reg] );
}

void acprintfrm ( char *inst, uint reg, uint a )
{
  if (VERBOSE && (OperandSize==OPERAND_SIZE16))
    fprintf(OUTSTREAM, "%s %s, [0x%08X]\n", inst, reg_str16[reg], a );
  else if (VERBOSE && (OperandSize==OPERAND_SIZE32))
    fprintf(OUTSTREAM, "%s %s, [0x%08X]\n", inst, reg_str[reg], a );
}

void acprintfi16 ( char *inst, ushort i )
{
  if (VERBOSE) fprintf(OUTSTREAM, "%s 0x%04X\n", inst, i);
}

void acprintfi32 ( char *inst, uint i )
{
  if (VERBOSE) fprintf(OUTSTREAM, "%s 0x%08X\n", inst, i);
}

void acprintfri16 ( char *inst, uint reg, ushort i )
{
  
  if (VERBOSE) fprintf(OUTSTREAM, "%s %s, 0x%04X\n", inst, reg_str16[reg], i);
}

void acprintfri32 ( char *inst, uint reg, uint i )
{
  if (VERBOSE) fprintf(OUTSTREAM, "%s %s, 0x%08X\n", inst, reg_str[reg], i );
}

void acprintfri ( char *inst, uint reg, uint i )
{
  if (VERBOSE) fprintf(OUTSTREAM, "%s %s, 0x%08X\n", inst, reg_str[reg], i);
}

void acprintfmi ( char *inst, uint a, uint i )
{
  if (VERBOSE) fprintf(OUTSTREAM, "%s [0x%08X], 0x%08X\n", inst, a, i );
}

void acprintfr16m1616 ( char *inst, uint reg, uint sel, uint offset )
{
  if (VERBOSE)
    fprintf(OUTSTREAM, "%s %s, 0x%04X:0x%04x\n",inst,reg_str16[reg],sel,offset);
}

void acprintfr32m1632 ( char *inst, uint reg, uint sel, uint offset )
{
  if (VERBOSE)
    fprintf(OUTSTREAM, "%s %s, 0x%04X:0x%08X\n", inst, reg_str[reg], sel, offset);
}

void acprintfr16m ( char *inst, uint reg, uint a )
{
  if (VERBOSE) fprintf(OUTSTREAM, "%s %s, 0x%08X\n", inst, reg_str16[reg], a);
}

void acprintfr32m ( char *inst, uint reg, uint a )
{
  if (VERBOSE) fprintf(OUTSTREAM, "%s %s, 0x%08X\n", inst, reg_str[reg], a);
}

// ----------------------------------------------------------------------------
// Auxiliar shift operations --------------------------------------------------
// ----------------------------------------------------------------------------

bool shiftLeft ( signed int *dest, signed char count )
{
  bool ret_val=false;
  if ( count > 0 )
    {
      (*dest) = (*dest) << count - 1;
      if ( (*dest) & (1<<31) ) ret_val = true;
      (*dest) = (*dest) << 1;
    }
  else if ( count < 0 )
    {
      (*dest) = (*dest) << count + 1;
      if ( (*dest) & (1) ) ret_val = true;
      (*dest) = (*dest) >> 1;
    }
  return ret_val;
}

bool shiftRight ( signed int *dest, unsigned char count )
{
  return shiftLeft(dest, ((~count)+0x01));
}

// ----------------------------------------------------------------------------
// Bit operations -------------------------------------------------------------
// ----------------------------------------------------------------------------

bool MSB32 ( uint b )
{
  return (b&(1<<31));
}

bool MSB16 ( uint b )
{
  return (b&(1<<15));
}

bool MSB8 ( uint b )
{
  return (b&(1<<7));
}

bool LSB ( uint b )
{
  return (b&(0x01));
}

bool testBit ( uint array, long index )
{
  long realIndex = index%32;
  uint mask = (uint)(1<<realIndex);
  if ( array & mask ) return true;
  return false;
}

bool testMemBit ( uint address, long index )
{
  uint realAddress = (uint)(address + (index/8));
  long realIndex = index%8;
  if ( realIndex < 0x00 ) realIndex = -realIndex;
  uint mask = (uint)(1<<realIndex);
  if ( MEM.read_byte(realAddress) & mask ) return true;
  return false;
}

void setBit ( uint *array, long index )
{
  long realIndex = index%32;
  uint mask = (uint)(1<<realIndex);
  (*array) = (*array)|mask;
}

void setMemBit ( uint address, long index )
{
  uint realAddress = (uint)(address + (index/8));
  long realIndex = index%8;
  if ( realIndex < 0x00 ) realIndex = -realIndex;
  uint mask = (uint)(1<<realIndex);
  unsigned char aux = MEM.read_byte(realAddress);
  MEM.write_byte(realAddress,aux|mask);
}

void resetBit ( uint *array, long index )
{
  long realIndex = index%32;
  uint mask = (~((uint)(1<<realIndex)));
  (*array) = (*array)&mask;
}

void resetMemBit ( uint address, long index )
{
  uint realAddress = (uint)(address + (index/8));
  long realIndex = index%8;
  if ( realIndex < 0x00 ) realIndex = -realIndex;
  uint mask = (~((uint)(1<<realIndex)));
  unsigned char aux = MEM.read_byte(realAddress);
  MEM.write_byte(realAddress,aux&mask);
}

void compBit ( uint *array, long index )
{
  if ( testBit((*array),index) ) resetBit(array,index);
  else setBit(array,index);
}

void compMemBit ( uint address, long index )
{
  if ( testMemBit(address,index) ) resetMemBit ( address,index );
  else setMemBit ( address, index );
}

// ----------------------------------------------------------------------------
// Stack functions ------------------------------------------------------------
// ----------------------------------------------------------------------------
void push ( uint d )
{
  uint temp;
  if ( OperandSize == OPERAND_SIZE16 )
    {
      temp = GR.read(ESP) - 2;
      GR.write(ESP,temp);
      MEM.write_half(temp, (unsigned short)d);
    }
  else if ( OperandSize == OPERAND_SIZE32 )
    {
      temp = GR.read(ESP) - 4;
      GR.write(ESP,temp);
      MEM.write(temp, d);
    }
}

uint pop()
{
  signed short r16;
  signed int aux;
  uint temp, ret;
  temp = GR.read(ESP);
  if ( OperandSize == OPERAND_SIZE16 )
    {
      r16 = MEM.read_half(temp);
      GR.write(ESP, temp+2);
      aux = r16;
      ret = aux;
    }
  else if ( OperandSize == OPERAND_SIZE32 )
    {
      if ( temp == STACK_BOTTOM )
	{
	  fprintf ( stderr, "Stack is empty!\n" );
	  return 0;
	}
      ret = MEM.read(temp);
      GR.write(ESP,temp+4);
      if ((GR.read(ESP) == STACK_BOTTOM) && (ret == 0xCC00))
	{
	  fprintf(stderr,"ArchC: Simulation terminated, found end of program.\n");
	  ac_stop(0);
	  fprintf ( stderr, "\n" );
	  PrintStat();
	  fprintf ( stderr, "\n" );
	  exit(1);
	}
    }
  return ret;
}

// ----------------------------------------------------------------------------
// Flag behavior functions ----------------------------------------------------
// ----------------------------------------------------------------------------
void setFlag ( uint flagId )
{
  uint temp = SPR.read(EFLAGS)|flagId;
  SPR.write(EFLAGS,temp);
}

void resetFlag ( uint flagId )
{
  uint temp = SPR.read(EFLAGS)&(~flagId);
  SPR.write(EFLAGS,temp);
}

void writeFlag ( uint flagId, bool val )
{
  if ( val ) setFlag(flagId);
  else resetFlag(flagId);
}

void compFlag ( uint flagId )
{
  uint temp = SPR.read(EFLAGS);
  if (temp&flagId) resetFlag(flagId);
  else setFlag(flagId);
}

bool testFlag ( uint flagId )
{
  uint temp = SPR.read(EFLAGS);
  if ( temp & flagId ) return true;
  return false;
}

bool checkParity ( uint res )
{
  unsigned char aux = (unsigned char)res;
  uint parity = 0;
  for ( int i=0; i<8; i++ )
    if ( aux&(1<<i) ) parity++;
  if ( (parity % 2) || (parity==0) )
    return false;
  return true;
}

bool checkCarry ( uint oper, uint op1, uint op2, uint res )
{
  unsigned long long lop1 = op1;
  unsigned long long lop2 = op2;
  unsigned long long aux = lop1+lop2;
  switch ( oper )
    {
    case OPER_MUL:
      if (res) return true;
      break;
    case OPER_ADD:
      if ( aux > 0xFFFFFFFF )
	return true;
      return false;
      break;
    case OPER_SUB:
      if ( op1 < op2 ) return true;
      return false;
      break;
    };
  return false;
}

bool checkCarry16 ( uint oper, uint op1, uint op2, uint res )
{
  unsigned short lop1 = op1;
  unsigned short lop2 = op2;
  unsigned long aux = lop1+lop2;
  switch ( oper )
    {
    case OPER_MUL:
      if (res) return true;
      break;
    case OPER_ADD:
      if ( aux > 0xFFFF )
	return true;
      return false;
      break;
    case OPER_SUB:
      if ( op1 < op2 ) return true;
      return false;
      break;
    };
  return false;
}

bool checkCarry8 ( uint oper, uint op1, uint op2, uint res )
{
  unsigned char lop1 = op1;
  unsigned char lop2 = op2;
  unsigned int aux = lop1+lop2;
  switch ( oper )
    {
    case OPER_MUL:
      if (res) return true;
      break;
    case OPER_ADD:
      if ( aux > 0xFF )
	return true;
      return false;
      break;
    case OPER_SUB:
      if ( op1 < op2 ) return true;
      return false;
      break;
    };
  return false;
}

bool checkAdjust ( uint oper, uint op1, uint op2, uint res )
{
  switch ( oper )
    {
    case OPER_ADD:
      if ( (op1&(1<<4)) && (op2&(1<<4)) && !(res&(1<<4)) ) return true;
      if ( !(op1&(1<<4)) && !(op2&(1<<4)) && (res&(1<<4)) ) return true;
      return false;
      break;
    case OPER_SUB:
      op2 = (~op2) + 1;
      if ( (op1&(1<<4)) && (op2&(1<<4)) && !(res&(1<<4)) ) return true;
      if ( !(op1&(1<<4)) && !(op2&(1<<4)) && (res&(1<<4)) ) return true;
      return false;
      break;
    };
  return false;
}

bool checkOverflow ( uint oper, uint op1, uint op2, uint res )
{
  signed int sop1 = op1;
  signed int sop2 = op2;
  signed int sres = res;
  switch ( oper )
    {
    case OPER_ADD:
      if ( (sop1<0) && (sop2<0) && (sres>0) ) return true;
      if ( (sop1>0) && (sop2>0) && (sres<0) ) return true;
      return false;
      break;
    case OPER_SUB:
      sop2 = -sop2;
      if ( (sop1<0) && (sop2<0) && (sres>0) ) return true;
      if ( (sop1>0) && (sop2>0) && (sres<0) ) return true;
      return false;
      break;
    case OPER_MUL:
      if ( res != 0 ) return true;
      return false;
      break;
    }
  return false;
}

bool checkOverflow16 ( uint oper, uint op1, uint op2, uint res )
{
  signed short sop1 = op1;
  signed short sop2 = op2;
  signed short sres = res;
  switch ( oper )
    {
    case OPER_ADD:
      if ( (sop1<0) && (sop2<0) && (sres>0) ) return true;
      if ( (sop1>0) && (sop2>0) && (sres<0) ) return true;
      return false;
      break;
    case OPER_SUB:
      sop2 = -sop2;
      if ( (sop1<0) && (sop2<0) && (sres>0) ) return true;
      if ( (sop1>0) && (sop2>0) && (sres<0) ) return true;
      return false;
      break;
    case OPER_MUL:
      if ( res != 0 ) return true;
      return false;
      break;
    }
  return false;
}

bool checkOverflow8 ( uint oper, uint op1, uint op2, uint res )
{
  signed char sop1 = op1;
  signed char sop2 = op2;
  signed char sres = res;
  switch ( oper )
    {
    case OPER_ADD:
      if ( (sop1<0) && (sop2<0) && (sres>0) ) return true;
      if ( (sop1>0) && (sop2>0) && (sres<0) ) return true;
      return false;
      break;
    case OPER_SUB:
      sop2 = -sop2;
      if ( (sop1<0) && (sop2<0) && (sres>0) ) return true;
      if ( (sop1>0) && (sop2>0) && (sres<0) ) return true;
      return false;
      break;
    case OPER_MUL:
      if ( res != 0 ) return true;
      return false;
      break;
    }
  return false;
}

void checkFlags8 ( uint oper, uint op1, uint op2, uint res, uint flags )
{
  if ( flags&FLAG_CF ) writeFlag(FLAG_CF, checkCarry8(oper,op1,op2,res));
  if ( flags&FLAG_PF ) writeFlag(FLAG_PF, checkParity(res));
  if ( flags&FLAG_AF ) writeFlag(FLAG_AF, checkAdjust(oper,op1,op2,res));
  if ( flags&FLAG_ZF ) writeFlag(FLAG_ZF, !res);
  if ( flags&FLAG_SF ) writeFlag(FLAG_SF, MSB8(res));
  if ( flags&FLAG_OF ) writeFlag(FLAG_OF, checkOverflow8(oper,op1,op2,res));
}

void checkFlags ( uint oper, uint op1, uint op2, uint res, uint flags )
{
  if ( flags&FLAG_CF )
    {
      if ( OperandSize == OPERAND_SIZE16 )
	writeFlag(FLAG_CF, checkCarry16(oper,op1,op2,res) );
      else if ( OperandSize == OPERAND_SIZE32 )
	writeFlag(FLAG_CF, checkCarry(oper,op1,op2,res) );
    }
  if ( flags&FLAG_PF ) writeFlag(FLAG_PF, checkParity(res));
  if ( flags&FLAG_AF ) writeFlag(FLAG_AF, checkAdjust(oper,op1,op2,res) );
  if ( flags&FLAG_ZF ) writeFlag(FLAG_ZF, !res);
  if ( flags&FLAG_SF )
    {
      if ( OperandSize == OPERAND_SIZE16 )
	writeFlag(FLAG_SF, MSB16(res));
      else if ( OperandSize == OPERAND_SIZE32 )
	writeFlag(FLAG_SF, MSB32(res));
    }
  if ( flags&FLAG_OF )
    {
      if ( OperandSize == OPERAND_SIZE16 )
	writeFlag(FLAG_OF, checkOverflow16(oper,op1,op2,res));
      else if ( OperandSize == OPERAND_SIZE32 )
	writeFlag(FLAG_OF, checkOverflow(oper,op1,op2,res));
    }
}


//-----------------------------------                                                                                                    
//Flags -- Auxiliar Functions -- Begin
//----------------------------------
                                                                                                          

void setFlags_af(uint flag,uint reg){

/*****
Function to set 'flag' according to the 
resulting binary value in the 'reg' register
*****/

  if ( flag&FLAG_SF )
    if ( reg >= 0 ) resetFlag( FLAG_CF );
    else setFlag( FLAG_CF );
  if ( flag&FLAG_ZF )
    if ( reg == 0 ) setFlag( FLAG_ZF );
    else resetFlag( FLAG_ZF );
  if ( flag&FLAG_PF )
    writeFlag(FLAG_PF,checkParity( reg ));
}//setFlag                                                                                                                               

//-----------------------------------                                                                                                    
//Flags -- Auxiliar Functions -- End                                                                                                    
//----------------------------------   
// ----------------------------------------------------------------------------
// Generic processed data to instructions abstraction -------------------------
// ----------------------------------------------------------------------------

uint readMemory(uint a)
{
  signed int aux;
  signed short aux16;
  if ( OperandSize == OPERAND_SIZE16 )
    {
      aux16 = (MEM.read_half(a) & MASK_16BITS);
      aux = aux16;
    }
  else if ( OperandSize == OPERAND_SIZE32 )
    {
      aux = MEM.read(a);
    }
  return aux;
}

uint readMemory16(uint a)
{
  signed int aux;
  signed short aux16;
  aux16 = (MEM.read_half(a) & MASK_16BITS);
  aux = aux16;
  return aux;
}

uint readMemory8(uint a)
{
  signed char aux8;
  signed int aux;
  aux8 = (MEM.read(a) & MASK_8BITS);
  aux = aux8;
  return aux;
}

void writeMemory(uint a, uint val)
{
  unsigned short aux16 = (val & MASK_16BITS);
  if ( OperandSize == OPERAND_SIZE16 )
    MEM.write_half(a, aux16);
  else if ( OperandSize == OPERAND_SIZE32 )
    MEM.write(a, val);
}

void writeMemory8(uint a, uint val)
{
  MEM.write_byte(a, (val & MASK_8BITS));
}

uint readSPRegister(uint reg)
{
  return (SPR.read(reg));
}

void writeSPRegister(uint reg, uint val)
{
  if ( OperandSize == OPERAND_SIZE16 )
    SPR.write(reg, (SPR.read(reg)&0xFFFF0000)|(val&MASK_16BITS));
  else if ( OperandSize == OPERAND_SIZE32 )
    SPR.write(reg, val);
}

uint readSRegister(uint reg)
{
  return (SR.read(reg) & MASK_16BITS);
}

void writeSRegister(uint reg, uint val)
{
  SR.write(reg, (val & MASK_16BITS));
}

uint readRegister8(uint reg)
{
  signed char aux8;
  signed int aux;
  switch(reg)
    {
      // 8bit access to registers
    case AL: case CL: case DL: case BL:
      aux8 = (GR.read(reg) & MASK_8BITS);
      aux = aux8;
      return aux;
      break;
      
    case AH: 
      reg = 0;// ler de EAX
      aux8 = ((GR.read(reg)>>8) & MASK_8BITS);
      aux = aux8;
      return aux;
      break;
    case CH: 
      reg = 1;// ler de ECX
      aux8 = ((GR.read(reg)>>8) & MASK_8BITS);
      aux = aux8;
      return aux;
      break;
    case DH: 
      reg = 2;// ler de EDX
      aux8 = ((GR.read(reg)>>8) & MASK_8BITS);
      aux = aux8;
      return aux;
      break;

    case BH:
      reg = 3;// ler de EDX
      aux8 = ((GR.read(reg)>>8) & MASK_8BITS);
      aux = aux8;
      return aux;
      break;
    };
  return INVALID_REGISTER;
}

uint readRegister(uint reg)
{
  signed int aux;
  signed short aux16;
  if ( OperandSize == OPERAND_SIZE16 )
    {
      switch(reg)
	{
	  // 16bit access to registers
	case AX: case CX: case DX: case BX:
	case SP: case BP: case SI: case DI:
	  aux16 = (GR.read(reg) & MASK_16BITS);
	  aux = aux16;
	  return aux;
	  break;
	};
    }
  else if ( OperandSize == OPERAND_SIZE32 )
    {
      switch(reg)
	{
	  // 32bit access to registers
	case EAX: case ECX: case EDX: case EBX:
	case ESP: case EBP: case ESI: case EDI:
	  return GR.read(reg);
	  break;
	};
    }
  return INVALID_REGISTER;
}

void writeRegister8(uint reg, uint val)
{

 
  switch(reg)
    {
      // 8bit access to registers
    case AL: case CL: case DL: case BL:
      GR.write(reg, ((GR.read(reg) & 0xFFFFFF00) | (val & MASK_8BITS)));
      break;
      
    case AH:
      reg = 0; //AH sera escrito em EAX 
      GR.write(reg, ( (GR.read(reg) & 0xFFFF00FF) | ((val & MASK_8BITS)<<8) ));
      break;
    case CH: 
      reg = 1; //CH sera escrito em ECX
      GR.write(reg, ( (GR.read(reg) & 0xFFFF00FF) | ((val & MASK_8BITS)<<8) ));
      break;
    case DH: 
      reg = 2; //DH sera escrito em EDX
      GR.write(reg, ( (GR.read(reg) & 0xFFFF00FF) | ((val & MASK_8BITS)<<8) ));
      break;
    case BH:
      reg = 3; //BH sera escrito em EBX
      GR.write(reg, ( (GR.read(reg) & 0xFFFF00FF) | ((val & MASK_8BITS)<<8) ));
      break;
    }
}

void writeRegister(uint reg, uint val)
{
  if ( OperandSize == OPERAND_SIZE16 )
    {
      printf("REG 16bits: %i\n",reg);
      switch(reg)
	{
	 
	  // 16bit access to registers
	case AX: case CX: case DX: case BX:
	case BP: case SP: case SI: case DI:
	  GR.write(reg, ((GR.read(reg) & 0xFFFF0000) | (val & MASK_16BITS)));
	  break;
	};
    }
  else if ( OperandSize == OPERAND_SIZE32 )
    {
      printf("REG 32bits: %i\n",reg);
      switch(reg)
	{
	  // 32bit access to registers
	case EAX: case ECX: case EDX: case EBX:
	case EBP: case ESP: case ESI: case EDI:
	  GR.write(reg,val);
	  break;
	};
    }
}

class Data
{
private:
  uint Register1;
  uint Register2;
  uint Address;
  unsigned char Address8;
  signed int Immediate;
  signed int Immediate8;
  signed int Immediate16;
  bool UsesMemory;
public:
  Data(){};
  ~Data(){};
  bool usesMemory() {return UsesMemory;};
  void usesMemory(bool aux) {UsesMemory=aux;};
  uint reg1() {return Register1;};
  void reg1(uint r) {Register1=r;};
  uint reg2() {return Register2;};
  void reg2(uint r) {Register2=r;};
  uint address() {return Address;};
  void address(uint a) {Address=a;};
  unsigned char address8() {return Address8;};
  void address8(unsigned char a) {Address8=a;};
  uint immediate() {
    //    ac_pc+=4; SPR.write(EIP,(int)ac_pc); 
    
    if ( OperandSize == OPERAND_SIZE16){
      ac_pc+=2; SPR.write(EIP,(int)ac_pc); 
      acprintf("Size 16\n");
     
    }
    else if (OperandSize == OPERAND_SIZE32){
      ac_pc+=4; SPR.write(EIP,(int)ac_pc); 
      acprintf("Size 32\n");
     
  }
  
    return Immediate;
  };
  void immediate(signed int i) {Immediate=i;};
  uint immediate8() {ac_pc+=1; SPR.write(EIP,(int)ac_pc); return Immediate8;};
  void immediate8(signed char i) {Immediate8=i;};
  uint immediate16() {ac_pc+=2; SPR.write(EIP,(int)ac_pc);return Immediate16;};
  void immediate16(signed char i) {Immediate16=i;};
  void immediates(signed int i)
  {
    immediate(i);
    immediate16(i);
    immediate8(i);
  };
  uint acpc_inc;
};



Data data;
int IFJustSet;

class DataManager
{
public:
  DataManager(){};
  ~DataManager(){};
  uint getMemOrReg1()
  {
    if ( data.usesMemory() ) return readMemory(data.address());
    return readRegister(data.reg1());
  }
  uint getMemOrReg2()
  {
    if ( data.usesMemory() ) return readMemory(data.address());
    return readRegister(data.reg2());
  }
  uint getReg1() { return readRegister(data.reg1()); };
  uint getReg2() { return readRegister(data.reg2()); };
  uint getImmediate()
  {
    //    if ( OperandSize == OPERAND_SIZE16 ) return data.immediate16();
    if ( OperandSize == OPERAND_SIZE16 ) return data.immediate();
    else if ( OperandSize == OPERAND_SIZE32 ) return data.immediate();
    return data.immediate();
  }
  void setMemOrReg1(uint val)
  {
    if ( data.usesMemory() ) writeMemory(data.address(),val);
    else writeRegister(data.reg1(),val);
  }
  void setMemOrReg2(uint val)
  {
    if ( data.usesMemory() ) writeMemory(data.address(),val);
    else writeRegister(data.reg2(),val);
  }
  void setMem(uint val) { writeMemory(data.address(),val);}
  void setReg1(uint val) { writeRegister(data.reg1(),val); }
  void setReg2(uint val) { writeRegister(data.reg2(),val); }
};

DataManager dataManager;

class PrintManager
{
public:
  PrintManager(){};
  ~PrintManager(){};
  
  // Instructions such as: INST REG1, ADDR
  void printReg1Addr ( char *g16, char *g32, char *i16, char *i32 )
  {
    if ( OperandSize == OPERAND_SIZE16 )
      {
	acprintfg(g16);
	acprintfr16m ( i16, data.reg1(), data.address() );
      }
    else if ( OperandSize == OPERAND_SIZE32 )
      {
	acprintfg(g32);
	acprintfr32m ( i32, data.reg1(), data.address() );
      }
  }
  
  // Instructions such as: INST REG1, SEL:OFFSET
  void printReg1Addr ( char *g16, char *g32, char *i16, char *i32, uint s, uint o)
  {
    if ( OperandSize == OPERAND_SIZE16 )
      {
	acprintfg(g16);
	acprintfr16m1616 ( i16, data.reg1(), s, o );
      }
    else if ( OperandSize == OPERAND_SIZE32 )
      {
	acprintfg(g32);
	acprintfr32m1632 ( i32, data.reg1(), s, o );
      }
  }
  
  // Instructions such as: INST REG2, MEM/REG1
  void printReg2MReg1(char *g16, char *g32, char *i16, char *i32)
  {
    if ( OperandSize == OPERAND_SIZE16 )
      {
	acprintfg(g16);
	if ( data.usesMemory() ) acprintfmr(i16,data.address(),data.reg1());
	else acprintfrr(i16,data.reg2(),data.reg1());
      }
    else if ( OperandSize == OPERAND_SIZE32 )
      {
	acprintfg(g32);
	if ( data.usesMemory() ) acprintfmr(i32,data.address(),data.reg1());
	else acprintfrr(i32,data.reg2(),data.reg1());
      }
  }
  
  void printReg1MReg2(char *g16, char *g32, char *i16, char *i32)
  {
    if ( OperandSize == OPERAND_SIZE16 )
      {
	acprintfg(g16);
	if ( data.usesMemory()) acprintfrm(i16,data.reg1(),data.address());
	else acprintfrr(i16,data.reg1(),data.reg2());
      }
    else if ( OperandSize == OPERAND_SIZE32 )
      {
	acprintfg(g32);
	if ( data.usesMemory()) acprintfrm(i32,data.reg1(),data.address());
	else acprintfrr(i32,data.reg1(),data.reg2());
      }
  }
  
  // Instructions such as: INST FIXED, IMMEDIATE32
  void printImm(char *g16, char *g32, char *i16, char *i32, uint i)
  {
    if ( OperandSize == OPERAND_SIZE16 )
      {
	acprintfg(g16);
	acprintfi16(i16,i);
      }
    else if ( OperandSize == OPERAND_SIZE32 )
      {
	acprintfg(g32);
	acprintfi32(i32,i);
      }
  }
  
  // Instructions such as: INST REG/MEM, IMM16/32
  void printReg2MImm(char *g16, char *g32, char *i16, char *i32, uint i )
  {
    if ( OperandSize == OPERAND_SIZE16 )
      {
        //printf("16 bits\n");
	acprintfg(g16);
	acprintfri16(i16, data.reg2(), i);
      }
    else if ( OperandSize == OPERAND_SIZE32 )
      {
	acprintfg(g32);
	acprintfri32(i32, data.reg2(), i);
      }
  }
  
  // Instructions that are independent from operand and address sizes
  void print(char *inst)
  {
    acprintfg(inst);
    acprintf(inst);acprintf("\n");
  }
  
  // Instructions with fix-argument type
  void print(char *inst16, char *inst32)
  {
    if ( OperandSize == OPERAND_SIZE16 )
      {
	acprintfg(inst16);
	acprintf(inst16);acprintf("\n");
      }
    else if ( OperandSize == OPERAND_SIZE32 )
      {
	acprintfg(inst32);
	acprintf(inst32);acprintf("\n");
      }
  }
};

PrintManager printManager;

// ----------------------------------------------------------------------------
// Addressing modes abstraction -----------------------------------------------
// ----------------------------------------------------------------------------

signed int signExtend8 ( unsigned int i )
{
  if ( MSB8(i) ) return (0xFFFFFF00)|(i&MASK_8BITS);
  return i;
}

uint processSib(uint mod, uint sib, uint disp, uint imm, bool *used)
{
  uint ss = (sib>>6);
  uint index = (sib>>3)&(0x07);
  uint base = sib&0x07;
  uint scaledIndex=0;
  uint baseValue=0;
  
  // Calculating Scaled Index
  if ( index == 0x04 ) scaledIndex = 0;
  else scaledIndex = GR.read(index)*((uint)pow((double)2,(double)ss));
  
  // Calculating Base
  if ( (base == 0x05) && (mod == 0x00) )
    {
      baseValue = disp;
      data.acpc_inc+= 4;
      ac_pc+= 4;
      data.immediates(imm);
      (*used)=true;
    }
  else
    {
      baseValue = GR.read(base);
    }
  
  return baseValue + scaledIndex;
}

void processData(uint mod, uint regop, uint rm, uint sib, uint disp, uint imm)
{
  bool used = false;
  uint disp8, disp32;
  data.usesMemory(true);
  data.reg1(regop);
  data.acpc_inc = 0;
  switch(mod)
    {
    case 0x00:
      disp32 = (disp<<8)|(sib&MASK_8BITS);
      switch(rm)
	{
	case 0x04:	// SIB
	  data.address(processSib(mod,sib,disp,imm,&used));
	  data.acpc_inc+=2;
	  ac_pc+= 2;
	  if ( !used ) data.immediates(disp);
	  break;
	case 0x05:	// DISP32
	  data.address(disp32);
	  data.acpc_inc+=5;
	  ac_pc+= 5;
	  data.immediates((disp>>24)+(imm<<8));
	  break;
	default:	// [E**]
	  data.address(GR.read(rm));
	  data.acpc_inc+=1;
	  ac_pc+= 1;
	  data.immediates(disp32);
	  break;
	};
      break;
      
    case 0x01:
      disp8 = signExtend8(disp&MASK_8BITS);
      switch(rm)
	{
	case 0x04:	// SIB+DISP8
	  data.address(processSib(mod,sib,disp,imm,&used) + (signed)disp8);
	  data.acpc_inc+=3;
	  ac_pc+= 3;
	  data.immediates((imm<<24)|((disp>>8)&MASK_8BITS));
	  break;
	default:	// [E**]+DISP8
	  data.address(GR.read(rm) + (signed)signExtend8(sib));
	  data.acpc_inc+=2;
	  ac_pc+= 2;
	  data.immediates(disp);
	  break;
	};
      break;
      
    case 0x02:
      disp32 = disp;
      switch(rm)
	{
	case 0x04:	// SIB+DISP32
	  data.address(processSib(mod,sib,disp,imm,&used) + (signed)disp32);
	  data.acpc_inc+=6;
	  ac_pc+= 6;
	  data.immediates(imm);
	  break;
	default:	// [E**]+DISP32
	  disp32 = (disp<<8)|(sib&MASK_8BITS);
	  data.address(GR.read(rm) + (signed)disp32);
	  data.acpc_inc+=5;
	  ac_pc+= 5;
	  data.immediates(((disp>>24)&MASK_8BITS)|(imm<<8));
	  break;
	};
      break;
      
    case 0x03:
      data.usesMemory(false);
      data.reg2(rm);
      data.acpc_inc+=1;
      ac_pc+=1;
      data.immediates((sib&MASK_8BITS)|(disp<<8));
      break;
    };
}




//!Behavior executed before simulation begins.
void ac_behavior( begin )
{
  // Segmented memory is not implemented... yet
  SR.write(CS,0x00000000);	// Code Segment
  SR.write(DS,0x00000000);	// Data Segment
  SR.write(ES,0x00000000);
  SR.write(SS,0x00000000);	// Stack Segment
  
  // Setting Operand size attribute behavior
  OperandSize = OPERAND_SIZE32;
  
  // Setting Address size attribute behavior
  AddressSize = ADDRESS_SIZE32;
  
  // Setting Stack Pointer
  GR.write(ESP,STACK_BOTTOM);
  
  // Setting IF behavior flag
  IFJustSet = NOT_SET;
  setFlag(FLAG_IF);
  
  // Setting OS return control (end of program)
  push(0xCC00);
};

//!Behavior executed after simulation ends.
void ac_behavior( end ){};

//!Generic instruction behavior method.
void ac_behavior( instruction )
{
  // IF flag behavior implementation
  if (IFJustSet == RUNONCE)
    {
      IFJustSet = NOT_SET;
      setFlag(FLAG_IF);
    }
  if (IFJustSet == JUST_SET) IFJustSet = RUNONCE;
  // OperandSize attribute behavior implementation
  if (OperandSize == OPERAND_SIZE16 ) OperandSize = OPERAND_SIZE32;
  if (OperandSize == OPERAND_SIZE16JUST ) OperandSize = OPERAND_SIZE16;
  // AddressSize attribute behavior implementation
  if (AddressSize == ADDRESS_SIZE16 ) AddressSize = ADDRESS_SIZE32;
  if (AddressSize == ADDRESS_SIZE16JUST ) AddressSize = ADDRESS_SIZE16;
};

//! Instruction Format behavior methods.
void ac_behavior( Type_op1b )
{
  ac_pc+= 1;
  SPR.write(EIP,(int)ac_pc);
}
void ac_behavior( Type_op2b )
{
  ac_pc+= 2;
  SPR.write(EIP,(int)ac_pc);
}
void ac_behavior( Type_op1bi )
{
  data.immediate(imme);
  ac_pc+= 1;
  SPR.write(EIP,(int)ac_pc);
}
void ac_behavior( Type_op1bi8 )
{
  data.immediate8(imm8);
  ac_pc+= 1;
  SPR.write(EIP,(int)ac_pc);
}
void ac_behavior( Type_op1bd8 )
{
  data.address8(disp8);
  ac_pc+= 2;
  SPR.write(EIP,(int)ac_pc);
}
void ac_behavior( Type_op1bd32 )
{
  data.address(imme);
  ac_pc+= 5;
  SPR.write(EIP,(int)ac_pc);
}
void ac_behavior( Type_op2bd32 )
{
  data.address(disp32);
  ac_pc+= 6;
  SPR.write(EIP,(int)ac_pc);
}
void ac_behavior( Type_op1b_rm32 )
{
  processData(mod,regop,rm,sib,disp,imm);
  ac_pc+=1;
  SPR.write(EIP,(int)ac_pc);
}
void ac_behavior( Type_op2b_rm32 )
{
  processData(mod2,regop2,rm2,sib2,disp2,imm2);
  ac_pc+=2;
  SPR.write(EIP,(int)ac_pc);
}
void ac_behavior( Type_op2b_debug )
{
  if ( op2b==0xAAAA )
    ac_pc+= 10;
  else if ( op2b==0xDD00 )
    ac_pc+= 2;
  else if ( op2b==0xCC00 )
    ac_pc+= 2;
  SPR.write(EIP,(int)ac_pc);
}

//!Instruction insd behavior method.
void ac_behavior( insd )
{
  printManager.print ( "INSD <Not implemented>" );
  // Input/Output not available
}

//!Instruction outsd behavior method.
void ac_behavior( outsd )
{
  printManager.print ( "OUTSD <Not implemented>" );
  // Input/Output not available
}

// Debug ok
//!Instruction stc behavior method.
void ac_behavior( stc )
{
  printManager.print ( "STC" );
  setFlag(FLAG_CF);
}

// Debug ok
//!Instruction clc behavior method.
void ac_behavior( clc )
{
  printManager.print ( "CLC" );
  resetFlag(FLAG_CF);
}

// Debug ok
//!Instruction cmc behavior method.
void ac_behavior( cmc )
{
  printManager.print ( "CMC" );
  compFlag(FLAG_CF);
}

// Debug ok
//!Instruction cld behavior method.
void ac_behavior( cld )
{
  printManager.print ( "CLD" );
  resetFlag(FLAG_DF);
}

// Debug ok
//!Instruction std behavior method.
void ac_behavior( std )
{
  printManager.print ( "STD" );
  setFlag(FLAG_DF);
}

//!Instruction lahf behavior method.
void ac_behavior( lahf )
{
  printManager.print ( "LAHF <Not implemented>" );
}

//!Instruction sahf behavior method.
void ac_behavior( sahf )
{
  printManager.print ( "SAHF <Not implemented>" );
}

// Debug 16/32 ok
//!Instruction pushfd behavior method.
void ac_behavior( pushfd )
{
  printManager.print("PUSHF", "PUSHFD");
  push(readSPRegister(EFLAGS));
}

// Debug 16/32 ok
//!Instruction popfd behavior method.
void ac_behavior( popfd )
{
	printManager.print("POPF", "POPFD");
	uint aux = pop();
	writeSPRegister(EFLAGS, aux);
}

// Debug ok
//!Instruction sti behavior method.
void ac_behavior( sti )
{
	printManager.print ( "STI" );
	IFJustSet = JUST_SET;
}

// Debug ok
//!Instruction cli behavior method.
void ac_behavior( cli )
{
	printManager.print ( "CLI" );
	IFJustSet = NOT_SET;
	resetFlag(FLAG_IF);
}

// Debug ok
//!Instruction nop behavior method.
void ac_behavior( nop )
{
	printManager.print ( "NOP" );
}

//!Instruction xlatb behavior method.
void ac_behavior( xlatb )
{
	printManager.print ( "XLATB <Not implemented>" );
}

// Debug ok
//!Instruction push_EAX behavior method.
void ac_behavior( push_EAX )
{
	printManager.print ( "PUSH AX", "PUSH EAX" );
	push ( readRegister(EAX) );
}

// Debug ok
//!Instruction push_ECX behavior method.
void ac_behavior( push_ECX )
{
	printManager.print ( "PUSH CX", "PUSH ECX" );
	push ( readRegister(ECX) );
}

// Debug ok
//!Instruction push_EDX behavior method.
void ac_behavior( push_EDX )
{
	printManager.print ( "PUSH DX", "PUSH EDX" );
	push ( readRegister(EDX) );
}

// Debug ok
//!Instruction push_EBX behavior method.
void ac_behavior( push_EBX )
{
	printManager.print ( "PUSH BX", "PUSH EBX" );
	push ( readRegister(EBX) );
}

// Debug ok
//!Instruction push_ESP behavior method.
void ac_behavior( push_ESP )
{
	printManager.print ( "PUSH SP", "PUSH ESP" );
	push ( readRegister(ESP) );
}

// Debug ok
//!Instruction push_EBP behavior method.
void ac_behavior( push_EBP )
{
	printManager.print ( "PUSH BP", "PUSH EBP" );
	push ( readRegister(EBP) );
}

// Debug ok
//!Instruction push_ESI behavior method.
void ac_behavior( push_ESI )
{
	printManager.print ( "PUSH SI", "PUSH ESI" );
	push ( readRegister(ESI) );
}

// Debug ok
//!Instruction push_EDI behavior method.
void ac_behavior( push_EDI )
{
	printManager.print ( "PUSH DI", "PUSH EDI" );
	push ( readRegister(EDI) );
}

// Debug ok
//!Instruction push_CS behavior method.
void ac_behavior( push_CS )
{
	printManager.print ( "PUSH CS" );
	OperandSize = OPERAND_SIZE16;
        push ( readSRegister(CS) );
}

// Debug ok
//!Instruction push_SS behavior method.
void ac_behavior( push_SS )
{
	printManager.print ( "PUSH SS" );
	OperandSize = OPERAND_SIZE16;
	push ( readSRegister(CS) );
}

// Debug ok
//!Instruction push_DS behavior method.
void ac_behavior( push_DS )
{
	printManager.print ( "PUSH DS" );
	OperandSize = OPERAND_SIZE16;
	push ( readSRegister(DS) );
}

// Debug ok
//!Instruction push_ES behavior method.
void ac_behavior( push_ES )
{
	printManager.print ( "PUSH ES" );
	OperandSize = OPERAND_SIZE16;
	push ( readSRegister(ES) );
}

// Debug ok
//!Instruction pop_EAX behavior method.
void ac_behavior( pop_EAX )
{
	printManager.print ( "POP AX", "POP EAX" );
	writeRegister(EAX,pop());
}

// Debug ok
//!Instruction pop_ECX behavior method.
void ac_behavior( pop_ECX )
{
	printManager.print ( "POP CX", "POP ECX" );
	writeRegister(ECX,pop());
}

// Debug ok
//!Instruction pop_EDX behavior method.
void ac_behavior( pop_EDX )
{
	printManager.print ( "POP DX", "POP EDX" );
	writeRegister(EDX,pop());
}

// Debug ok
//!Instruction pop_EBX behavior method.
void ac_behavior( pop_EBX )
{
	printManager.print ( "POP BX", "POP EBX" );
	writeRegister(EBX,pop());
}

// Debug ok
//!Instruction pop_ESP behavior method.
void ac_behavior( pop_ESP )
{
	printManager.print ( "POP SP", "POP ESP" );
	writeRegister(ESP,pop());
}

// Debug ok
//!Instruction pop_EBP behavior method.
void ac_behavior( pop_EBP )
{
	printManager.print ( "POP BP", "POP EBP" );
	writeRegister(EBP,pop());
}

// Debug ok
//!Instruction pop_ESI behavior method.
void ac_behavior( pop_ESI )
{
	printManager.print ( "POP SI", "POP ESI" );
	writeRegister(ESI,pop());
}

// Debug ok
//!Instruction pop_EDI behavior method.
void ac_behavior( pop_EDI )
{
	printManager.print ( "POP DI", "POP EDI" );
	writeRegister(EDI,pop());
}

// Debug ok
//!Instruction pop_DS behavior method.
void ac_behavior( pop_DS )
{
	printManager.print ( "POP DS" );
	writeSRegister(DS,pop());
}

// Debug ok
//!Instruction pop_ES behavior method.
void ac_behavior( pop_ES )
{
	printManager.print ( "POP ES" );
	writeSRegister(ES,pop());
}

// Debug ok
//!Instruction pop_SS behavior method.
void ac_behavior( pop_SS )
{
	printManager.print ( "POP SS" );
	writeSRegister(SS,pop());
}

// Debug ok
//!Instruction push_a_ad behavior method.
void ac_behavior( push_a_ad )
{
	printManager.print ( "PUSHA", "PUSHAD" );
	uint tempESP = readRegister(ESP);
	push(readRegister(EAX));
	push(readRegister(ECX));
	push(readRegister(EDX));
	push(readRegister(EBX));
	push(tempESP);
	push(readRegister(EBP));
	push(readRegister(ESI));
	push(readRegister(EDI));
}

// Debug ok
//!Instruction pop_a_ad behavior method.
void ac_behavior( pop_a_ad )
{
	printManager.print ( "POPA", "POPAD" );
	uint tempESP;
	writeRegister(EDI,pop());
	writeRegister(ESI,pop());
	writeRegister(EBP,pop());
	tempESP = pop();
	writeRegister(EBX,pop());
	writeRegister(EDX,pop());
	writeRegister(ECX,pop());
	writeRegister(EAX,pop());
	writeRegister(ESP,tempESP);
}

//!Instruction in_EAX_DX behavior method.
void ac_behavior( in_EAX_DX )
{
	acprintf ( "IN EAX DX <Not implemented>\n" );
}

//!Instruction out_DX_EAX behavior method.
void ac_behavior( out_DX_EAX )
{
	acprintf ( "OUT DX EAX <Not implemented>\n" );
}

//!Instruction cwd_cwq behavior method.
void ac_behavior( cwd_cwq )
{
  printf ( "CWD/CDQ\n" );
  if ( OperandSize == OPERAND_SIZE16 )
    {
      if ( MSB16(readRegister(EAX)) ) writeRegister(EDX,0xFFFF);
      else writeRegister(EDX,0);
    }
  else if ( OperandSize == OPERAND_SIZE32 )
    {
      if ( MSB32(readRegister(EAX)) ) writeRegister(EDX,0xFFFFFFFF);
      else writeRegister(EDX,0);
    }
}

// Debug ok
//!Instruction inc_EAX behavior method.
void ac_behavior( inc_EAX )
{
  printManager.print ( "INC AX", "INC EAX" );
  uint op1 = readRegister(EAX);
  uint op2 = 1;
  uint res = op1 + op2;
  writeRegister(EAX, res );
  checkFlags(OPER_ADD,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction inc_ECX behavior method.
void ac_behavior( inc_ECX )
{
	printManager.print ( "INC CX", "INC ECX" );
	uint op1 = readRegister(ECX);
	uint op2 = 1;
	uint res = op1 + op2;
	writeRegister(ECX,res);
	checkFlags(OPER_ADD,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction inc_EDX behavior method.
void ac_behavior( inc_EDX )
{
	printManager.print ( "INC DX", "INC EDX" );
	uint op1 = readRegister(EDX);
	uint op2 = 1;
	uint res = op1 + op2;
	writeRegister(EDX,res);
	checkFlags(OPER_ADD,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction inc_EBX behavior method.
void ac_behavior( inc_EBX )
{
	printManager.print ( "INC BX", "INC EBX" );
	uint op1 = readRegister(EBX);
	uint op2 = 1;
	uint res = op1 + op2;
	writeRegister(EBX,res);
	checkFlags(OPER_ADD,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction inc_ESP behavior method.
void ac_behavior( inc_ESP )
{
	printManager.print ( "INC SP", "INC ESP" );
	uint op1 = readRegister(ESP);
	uint op2 = 1;
	uint res = op1 + op2;
	writeRegister(ESP, res);
	checkFlags(OPER_ADD,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction inc_EBP behavior method.
void ac_behavior( inc_EBP )
{
	printManager.print ( "INC BP", "INC EBP" );
	uint op1 = readRegister(EBP);
	uint op2 = 1;
	uint res = op1 + op2;
	writeRegister(EBP, res);
	checkFlags(OPER_ADD,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction inc_ESI behavior method.
void ac_behavior( inc_ESI )
{
	printManager.print ( "INC SI", "INC ESI" );
	uint op1 = readRegister(ESI);
	uint op2 = 1;
	uint res = op1 + op2;
	writeRegister(ESI, res);
	checkFlags(OPER_ADD,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction inc_EDI behavior method.
void ac_behavior( inc_EDI )
{
	printManager.print ( "INC DI", "INC EDI" );
	uint op1 = readRegister(EDI);
	uint op2 = 1;
	uint res = op1 + op2;
	writeRegister(EDI, res);
	checkFlags(OPER_ADD,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction dec_EAX behavior method.
void ac_behavior( dec_EAX )
{
  printManager.print ( "DEC AX", "DEC EAX" );
  uint op1 = readRegister(EAX);
  uint op2 = 1;
  uint res = op1 - op2;
  writeRegister(EAX, res);
  checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction dec_ECX behavior method.
void ac_behavior( dec_ECX )
{
	printManager.print ( "DEC CX", "DEC ECX" );
	uint op1 = readRegister(ECX);
	uint op2 = 1;
	uint res = op1 - op2;
	writeRegister(ECX, res);
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction dec_EDX behavior method.
void ac_behavior( dec_EDX )
{
	printManager.print ( "DEC DX", "DEC EDX" );
	uint op1 = readRegister(EDX);
	uint op2 = 1;
	uint res = op1 - op2;
	writeRegister(EDX, res);
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction dec_EBX behavior method.
void ac_behavior( dec_EBX )
{
	printManager.print ( "DEC BX", "DEC EBX" );
	uint op1 = readRegister(EBX);
	uint op2 = 1;
	uint res = op1 - op2;
	writeRegister(EBX, res);
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction dec_ESP behavior method.
void ac_behavior( dec_ESP )
{
	printManager.print ( "DEC SP", "DEC ESP" );
	uint op1 = readRegister(ESP);
	uint op2 = 1;
	uint res = op1 - op2;
	writeRegister(ESP, res);
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction dec_EBP behavior method.
void ac_behavior( dec_EBP )
{
	printManager.print ( "DEC BP", "DEC EBP" );
	uint op1 = readRegister(EBP);
	uint op2 = 1;
	uint res = op1 - op2;
	writeRegister(EBP, res);
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction dec_ESI behavior method.
void ac_behavior( dec_ESI )
{
	printManager.print ( "DEC SI", "DEC ESI" );
	uint op1 = readRegister(ESI);
	uint op2 = 1;
	uint res = op1 - op2;
	writeRegister(ESI, res);
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

// Debug ok
//!Instruction dec_EDI behavior method.
void ac_behavior( dec_EDI )
{
	printManager.print ( "DEC DI", "DEC EDI" );
	uint op1 = readRegister(EDI);
	uint op2 = 1;
	uint res = op1 - op2;
	writeRegister(EDI, res);
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}





//!Instruction iret_iretd behavior method.
void ac_behavior( iret_iretd )
{
	acprintf ( " IRETD <To implement>\n" );
}

// Debug ok
//!Instruction leave behavior method.
void ac_behavior( leave )
{
	printManager.print("LEAVE");
	writeRegister(ESP, readRegister(EBP));
	writeRegister(EBP, pop());
}

//!Instruction movs_m32_m32 behavior method.
void ac_behavior( movs_m32_m32 ){
	acprintf ( " MOVS M32 M32\n" );

	uint add_src = SR.read(DS)+GR.read(ESI);
	uint add_dst = SR.read(ES)+GR.read(EDI);
	uint df = SPR.read(EFLAGS)&(~FLAG_DF);
	MEM.write(add_dst,MEM.read(add_src));
	if (df == 0) {
		GR.write(ESI,GR.read(ESI)+4);
		GR.write(EDI,GR.read(EDI)+4);
	} else {
		GR.write(ESI,GR.read(ESI)-4);
		GR.write(EDI,GR.read(EDI)-4);
	}
}

//!Instruction cmps_m32_m32 behavior method.
void ac_behavior( cmps_m32_m32 ){
  acprintf ( " CMPS M32 M32\n" );
  
  uint add_src1 = SR.read(DS)+GR.read(ESI);
  uint add_src2 = SR.read(ES)+GR.read(EDI);
  uint df = SPR.read(EFLAGS)&(~FLAG_DF);	
  
  uint op1 = MEM.read(add_src1);
  uint op2 = MEM.read(add_src2);
  uint temp = op1 - op2;
  checkFlags(OPER_SUB, op1, op2, temp,FLAG_ALL);
  
  if (df == 0) {
    GR.write(ESI,GR.read(ESI)+4);
    GR.write(EDI,GR.read(EDI)+4);
  } else {
    GR.write(ESI,GR.read(ESI)-4);
    GR.write(EDI,GR.read(EDI)-4);
  }
}

//!Instruction scas_m32 behavior method.
void ac_behavior( scas_m32 ){
	acprintf ( " SCAS M32\n" );

	uint add_src = SR.read(ES)+GR.read(EDI);
	uint df = SPR.read(EFLAGS)&(~FLAG_DF);	

	uint op1 = GR.read(EAX);
	uint op2 = MEM.read(add_src);
	uint temp = op1 - op2;
	checkFlags(OPER_SUB, op1, op2, temp, FLAG_ALL);
	
	if (df == 0) {
		GR.write(EDI,GR.read(EDI)+4);
	} else {
		GR.write(EDI,GR.read(EDI)-4);
	}
}

//!Instruction lods_m32 behavior method.
void ac_behavior( lods_m32 ){
	acprintf ( " LODS M32\n" );

	uint add_src = SR.read(DS)+GR.read(ESI);
	uint df = SPR.read(EFLAGS)&(~FLAG_DF);	
	
	GR.write(EAX,MEM.read(add_src));
	
	if (df == 0) {
		GR.write(ESI,GR.read(ESI)+4);
	} else {
		GR.write(ESI,GR.read(ESI)-4);
	}
}

//!Instruction stos_m32 behavior method.
void ac_behavior( stos_m32 ){
	acprintf ( " STOS M32\n" );

	uint add_src = SR.read(ES)+GR.read(EDI);
	uint df = SPR.read(EFLAGS)&(~FLAG_DF);	
	
	MEM.write(add_src,GR.read(EAX));
	
	if (df == 0) {
		GR.write(EDI,GR.read(EDI)+4);
	} else {
		GR.write(EDI,GR.read(EDI)-4);
	}
}

// Debug ok
//!Instruction ud2 behavior method.
void ac_behavior( ud2 )
{
	printManager.print( "UD2 <Not implemented>" );
}

// Debug ok
//!Instruction cpuid behavior method.
void ac_behavior( cpuid )
{
	printManager.print ( "CPUID <Fake implementation>" );
	writeRegister(EAX,0x00002005);
	writeRegister(ECX,0x00002005);
	writeRegister(EDX,0x00002005);
	writeRegister(EBX,0x00002005);
}

// Debug ok
//!Instruction bswap_EAX_r32 behavior method.
void ac_behavior( bswap_EAX_r32 )
{
	printManager.print ( "BSWAP AX", "BSWAP EAX" );
	uint op = readRegister(EAX);
	uint res = (op<<24)|((op<<8)&0xFF0000)|((op>>8)&0xFF00)|((op>>24)&0xFF);
	writeRegister(EAX,res);
}

// Debug ok
//!Instruction bswap_ECX_r32 behavior method.
void ac_behavior( bswap_ECX_r32 )
{
	printManager.print ( "BSWAP CX", "BSWAP ECX" );
	uint op = readRegister(ECX);
	uint res = (op<<24)|((op<<8)&0xFF0000)|((op>>8)&0xFF00)|((op>>24)&0xFF);
	writeRegister(ECX,res);
}

// Debug ok
//!Instruction bswap_EDX_r32 behavior method.
void ac_behavior( bswap_EDX_r32 )
{
	printManager.print ( "BSWAP DX", "BSWAP EDX" );
	uint op = readRegister(EDX);
	uint res = (op<<24)|((op<<8)&0xFF0000)|((op>>8)&0xFF00)|((op>>24)&0xFF);
	writeRegister(EDX,res);
}

// Debug ok
//!Instruction bswap_EBX_r32 behavior method.
void ac_behavior( bswap_EBX_r32 )
{
	printManager.print ( "BSWAP BX", "BSWAP EBX" );
	uint op = readRegister(EBX);
	uint res = (op<<24)|((op<<8)&0xFF0000)|((op>>8)&0xFF00)|((op>>24)&0xFF);
	writeRegister(EBX,res);
}

// Debug ok
//!Instruction bswap_ESP_r32 behavior method.
void ac_behavior( bswap_ESP_r32 )
{
	printManager.print ( "BSWAP SP", "BSWAP ESP" );
	uint op = readRegister(ESP);
	uint res = (op<<24)|((op<<8)&0xFF0000)|((op>>8)&0xFF00)|((op>>24)&0xFF);
	writeRegister(ESP,res);
}

// Debug ok
//!Instruction bswap_EBP_r32 behavior method.
void ac_behavior( bswap_EBP_r32 )
{
	printManager.print ( "BSWAP BP", "BSWAP EBP" );
	uint op = readRegister(EBP);
	uint res = (op<<24)|((op<<8)&0xFF0000)|((op>>8)&0xFF00)|((op>>24)&0xFF);
	writeRegister(EBP,res);
}

// Debug ok
//!Instruction bswap_ESI_r32 behavior method.
void ac_behavior( bswap_ESI_r32 )
{
	printManager.print ( "BSWAP SI", "BSWAP ESI" );
	uint op = readRegister(ESI);
	uint res = (op<<24)|((op<<8)&0xFF0000)|((op>>8)&0xFF00)|((op>>24)&0xFF);
	writeRegister(ESI,res);
}

// Debug ok
//!Instruction bswap_EDI_r32 behavior method.
void ac_behavior( bswap_EDI_r32 )
{
	printManager.print ( "BSWAP DI", "BSWAP EDI" );
	uint op = readRegister(EDI);
	uint res = (op<<24)|((op<<8)&0xFF0000)|((op>>8)&0xFF00)|((op>>24)&0xFF);
	writeRegister(EDI,res);
}

// Debug ok
//!Instruction push_FS behavior method.
void ac_behavior( push_FS )
{
	printManager.print ( "PUSH FS" );
	OperandSize = OPERAND_SIZE16;
	push(readSRegister(FS));
}

// Debug ok
//!Instruction push_GS behavior method.
void ac_behavior( push_GS )
{
	printManager.print ( "PUSH GS" );
	OperandSize = OPERAND_SIZE16;
	push(readSRegister(GS));
}

// Debug ok
//!Instruction pop_FS behavior method.
void ac_behavior( pop_FS )
{
	printManager.print ( "POP FS" );
	OperandSize = OPERAND_SIZE16;
	writeSRegister(FS,pop());
}

// Debug ok
//!Instruction pop_GS behavior method.
void ac_behavior( pop_GS )
{
	printManager.print ( "POP GS" );
	OperandSize = OPERAND_SIZE16;
	writeRegister(GS,pop());
}

//!Instruction lds_r32_m16_32 behavior method.
void ac_behavior( lds_r32_m16_32 )
{
	uint sel = readMemory16(data.address());
	uint offset = readMemory(data.address()+2);
	printManager.printReg1Addr("LDS R16, M16:16","LDS R32, M16:32","LDS","LDS",sel,offset);
	writeSRegister(DS, sel);
	writeRegister(data.reg1(), offset);
}

//!Instruction les_r32_m16_32 behavior method.
void ac_behavior( les_r32_m16_32 )
{
  uint sel = readMemory16(data.address());
  uint offset = readMemory(data.address()+2);
  printManager.printReg1Addr("LES R16, M16:16","LES R32, M16:32","LES","LES",sel,offset);
  writeSRegister(ES, sel);
  writeRegister(data.reg1(), offset);
}

//!Instruction lea_r32_m behavior method.
void ac_behavior( lea_r32_m )
{
  printManager.printReg1Addr ( "LEA R16, M", "LEA R32, M", "LEA", "LEA" );
  dataManager.setReg1(data.address());
}

//!Instruction mov_rm32_r32 behavior method.
void ac_behavior( mov_rm32_r32 )
{
  printManager.printReg2MReg1("MOV RM16, R16","MOV RM32, R32","MOV","MOV");
  dataManager.setMemOrReg2(dataManager.getReg1());
}


//!Instruction mov_r32_rm32 behavior method.
void ac_behavior( mov_r32_rm32 )
{
  printManager.printReg1MReg2("MOV R16, RM16","MOV R32, RM32","MOV","MOV");
  dataManager.setReg1(dataManager.getMemOrReg2());
}

//!Instruction mov_EAX_imm32 behavior method.
void ac_behavior( mov_EAX_imm32 )
{
  uint i = dataManager.getImmediate();
  printf("Teste EAX: %i\n",i);
  printManager.printImm("MOV AX, IMM16","MOV EAX, IMM32","MOV AX,","MOV EAX,",i);
  writeRegister(EAX, i);
}

//!Instruction mov_ECX_imm32 behavior method.
void ac_behavior( mov_ECX_imm32 )
{
  uint i = dataManager.getImmediate();
  printf("Teste ECX: %i\n",i);
  printManager.printImm("MOV CX, IMM16","MOV ECX, IMM32","MOV CX,","MOV ECX,",i);
  writeRegister(ECX, i);
}

//!Instruction mov_EDX_imm32 behavior method.
void ac_behavior( mov_EDX_imm32 )
{
  uint i = dataManager.getImmediate();
  printf("Teste EDX: %i\n",i);
  printManager.printImm("MOV DX, IMM16","MOV EDX, IMM32","MOV DX,","MOV EDX,",i);
  writeRegister(EDX, i);
}

//!Instruction mov_EBX_imm32 behavior method.
void ac_behavior( mov_EBX_imm32 )
{
  uint i = dataManager.getImmediate();
  printf("Teste EBX: %i\n",i);
  printManager.printImm("MOV BX, IMM16","MOV EBX, IMM32","MOV BX,","MOV EBX,",i);
  writeRegister(EBX, i);
}

//!Instruction mov_ESP_imm32 behavior method.
void ac_behavior( mov_ESP_imm32 )
{
  uint i = dataManager.getImmediate();
  printManager.printImm("MOV SP, IMM16","MOV ESP, IMM32","MOV SP,","MOV ESP,",i);
  writeRegister(ESP, i);
}

//!Instruction mov_EBP_imm32 behavior method.
void ac_behavior( mov_EBP_imm32 )
{
  uint i = dataManager.getImmediate();
  printManager.printImm("MOV BP, IMM16","MOV EBP, IMM32","MOV BP,","MOV EBP,",i);
  writeRegister(EBP, i);
}

//!Instruction mov_ESI_imm32 behavior method.
void ac_behavior( mov_ESI_imm32 )
{
  uint i = dataManager.getImmediate();
  printManager.printImm("MOV SI, IMM16","MOV ESI, IMM32","MOV SI,","MOV ESI,",i);
  writeRegister(ESI, i);
}

//!Instruction mov_EDI_imm32 behavior method.
void ac_behavior( mov_EDI_imm32 )
{
  uint i = dataManager.getImmediate();
  printManager.printImm("MOV DI, IMM16","MOV EDI, IMM32","MOV DI,","MOV EDI,",i);
  writeRegister(EDI, i);
}

//!Instruction mov_rm32_imm32 behavior method.
void ac_behavior( mov_rm32_imm32 )
{

  printf("�aqui!!!!!\n");
  uint i = dataManager.getImmediate();
  printManager.printReg2MImm("MOV RM16, IMM16","MOV RM32, IMM32","MOV","MOV",i);
  dataManager.setMemOrReg2(i);
}

//!Instruction xchg_EAX_r32 behavior method.
// void ac_behavior( xchg_EAX_EAX ){}

//!Instruction xchg_ECX_r32 behavior method.
void ac_behavior( xchg_EAX_ECX )
{
  acprintf ( "XCHG EAX, ECX\n" );
  uint aux = readRegister(EAX);
  writeRegister(EAX, readRegister(ECX));
  writeRegister(ECX, aux);
}

//!Instruction xchg_EDX_r32 behavior method.
void ac_behavior( xchg_EAX_EDX )
{
  acprintf ( "XCHG EAX, EDX\n" );
  uint aux = readRegister(EAX);
  writeRegister(EAX, readRegister(EDX));
  writeRegister(EDX, aux);
}

//!Instruction xchg_EBX_r32 behavior method.
void ac_behavior( xchg_EAX_EBX )
{
	acprintf ( "XCHG EAX, EBX\n" );
        uint aux = readRegister(EAX);
        writeRegister(EAX, readRegister(EBX));
        writeRegister(EBX, aux);
}

//!Instruction xchg_ESP_r32 behavior method.
void ac_behavior( xchg_EAX_ESP )
{
	acprintf ( "XCHG EAX, ESP\n" );
        uint aux = readRegister(EAX);
        writeRegister(EAX, readRegister(ESP));
        writeRegister(ESP, aux);
}

//!Instruction xchg_EBP_r32 behavior method.
void ac_behavior( xchg_EAX_EBP )
{
	acprintf ( "XCHG EAX, EBP\n" );
        uint aux = readRegister(EAX);
        writeRegister(EAX, readRegister(EBP));
        writeRegister(EBP, aux);
}

//!Instruction xchg_ESI_r32 behavior method.
void ac_behavior( xchg_EAX_ESI )
{
	acprintf ( "XCHG EAX, ESI\n" );
        uint aux = readRegister(EAX);
        writeRegister(EAX, readRegister(ESI));
        writeRegister(ESI, aux);
}

//!Instruction xchg_EDI_r32 behavior method.
void ac_behavior( xchg_EAX_EDI )
{
	acprintf ( "XCHG EAX, EDI\n" );
        uint aux = readRegister(EAX);
        writeRegister(EAX, readRegister(EDI));
        writeRegister(EDI, aux);
}

//!Instruction xchg_rm32_r32 behavior method.
void ac_behavior( xchg_rm32_r32 )
{
	acprintf ( "XCHG RM32, R32\n" );
	uint aux;
	if ( data.usesMemory() )
	{
		aux = readRegister(data.reg1());
		writeRegister(data.reg1(), readMemory(data.address()));
		writeMemory(data.address(), aux);
	}
	else
	{
		aux = readRegister(data.reg1());
		writeRegister(data.reg1(), readRegister(data.reg2()));
		writeRegister(data.reg2(), aux);
	}
}

//!Instruction push_rm32 behavior method.
void ac_behavior( push_rm32 )
{
	acprintf ( "PUSH RM32\n" );
	if ( data.usesMemory() ) push(readMemory(data.address()));
	else push(readRegister(data.reg2()));
}

//!Instruction push_imm32 behavior method.
void ac_behavior( push_imm32 )
{
	acprintf ( "PUSH IMM32\n" );
	push(dataManager.getImmediate());
}

//!Instruction pop_rm32 behavior method.
void ac_behavior( pop_rm32 )
{
	acprintf ( "POP RM32\n" );
	if ( data.usesMemory() ) writeMemory(data.address(),pop());
	else writeRegister(data.reg2(),pop());
}

//!Instruction in_EAX_imm8 behavior method.
void ac_behavior( in_EAX_imm8 )
{
	acprintf ( "IN EAX, IMM8 <Not implemented>\n" );
}

//!Instruction out_imm8_EAX behavior method.
void ac_behavior( out_imm8_EAX )
{
	acprintf ( "OUT IMM8, EAX <Not implemented>\n" );
}

//!Instruction add_EAX_imm32 behavior method.
void ac_behavior( add_EAX_imm32 )
{
	acprintf ( "ADD EAX, IMM32\n" );
	uint op1 = readRegister(EAX);
	uint op2 = dataManager.getImmediate();
	uint aux = op1 + op2;
	writeRegister(EAX,aux);
	checkFlags(OPER_ADD,op1,op2,aux,FLAG_ALL); 
}

//!Instruction add_rm32_imm32 behavior method.
void ac_behavior( add_rm32_imm32 )
{
	uint op1, op2, aux;
	acprintf ( "ADD RM32, IMM32\n" );
	if ( data.usesMemory() )
	{
		op1 = readMemory(data.address());
		op2 = dataManager.getImmediate();
		aux = op1 + op2;
		writeMemory(data.address(),aux);
	}
	else
	{
		op1 = readRegister(data.reg2());
		op2 = dataManager.getImmediate();
		aux = op1 + op2;
		writeRegister(data.reg2(), aux);
	}
	checkFlags(OPER_ADD,op1,op2,aux,FLAG_ALL);
}

//!Instruction add_rm32_imm8 behavior method.
void ac_behavior( add_rm32_imm8 )
{
	// acprintf ( "ADD RM32, IMM8\t\t" );
	signed char i = data.immediate8();
	signed int ii = i;
	uint op1, op2, aux;
	if ( data.usesMemory() )
	{
		op1 = readMemory(data.address());
		op2 = ii;
		if ( op2 & (1<<7) ) op2 = op2|0xFFFFFF00;
		aux = op1 + op2;
		writeMemory(data.address(), aux);
	}
	else
	{
		op1 = readRegister(data.reg2());
		op2 = ii;
		if ( op2 & (1<<7) ) op2 = op2|0xFFFFFF00;
		aux = op1 + op2;
		writeRegister(data.reg2(),aux);
	}
	if ( data.usesMemory() ) acprintfmi ( "ADD", data.address(), op2 );
	else acprintfri ( "ADD", data.reg2(), op2 );
	checkFlags(OPER_ADD,op1,op2,aux,FLAG_ALL);
}

//!Instruction add_rm32_r32 behavior method.
void ac_behavior( add_rm32_r32 )
{
	uint op1, op2, aux;
	// acprintf ( "ADD RM32, R32\n" );
	printf ( "ADD " );
	if ( data.usesMemory() )
	{
		printf ( "[0x%08X], ", data.address() );
		op1 = readMemory(data.address());
		op2 = readRegister(data.reg1());
		aux = op1 + op2;
		writeMemory(data.address(), aux);
	}
	else
	{
		if ( OperandSize == OPERAND_SIZE16 ) printf ( "%s, ", reg_str16[data.reg1()] );
		else printf ( "%s, ", reg_str[data.reg1()] );
		op1 = readRegister(data.reg1());
		op2 = readRegister(data.reg2());
		aux = op1 + op2;
		writeRegister(data.reg2(), aux);
	}
	if ( OperandSize == OPERAND_SIZE16 ) printf ( "%s\n", reg_str16[data.reg2()] );
	else printf ( "%s\n", reg_str[data.reg2()] );
	checkFlags(OPER_ADD,op1,op2,aux,FLAG_ALL);
}

//!Instruction add_r32_rm32 behavior method.
void ac_behavior( add_r32_rm32 )
{
	uint op1, op2, aux;
	acprintf ( "ADD R32, RM32\n" );
	if ( data.usesMemory() )
	{
		op1 = readRegister(data.reg1());
		op2 = readMemory(data.address());
		aux = op1 + op2;
		writeRegister(data.reg1(),aux);
	}
	else
	{
		op1 = readRegister(data.reg1());
		op2 = readRegister(data.reg2());
		aux = op1 + op2;
		writeRegister(data.reg1(),aux);
	}
	checkFlags(OPER_ADD,op1,op2,aux,FLAG_ALL);
}

//!Instruction adc_EAX_imm32 behavior method.
void ac_behavior( adc_EAX_imm32 )
{
	acprintf ( "ADC EAX, IMM32\n" );
	uint op1, op2, aux, carry=0;
	if ( testFlag ( FLAG_CF ) ) carry = 1;
	op1 = readRegister(EAX);
	op2 = dataManager.getImmediate() + carry;
	aux = op1 + op2;
	writeRegister(EAX, aux);
	checkFlags(OPER_ADD,op1,op2,aux,FLAG_ALL);
}

//!Instruction adc_rm32_r32 behavior method.
void ac_behavior( adc_rm32_r32 )
{
	acprintf ( "ADC RM32, R32\n" );
	uint op1, op2, res;
	op1 = dataManager.getMemOrReg2();
	op2 = dataManager.getReg1() + testFlag(FLAG_CF);
	res = op1 + op2;
	dataManager.setMemOrReg2(res);
	checkFlags(OPER_ADD,op1,op2,res,FLAG_ALL);
}

//!Instruction adc_r32_rm32 behavior method.
void ac_behavior( adc_r32_rm32 )
{
        acprintf ( "ADC R32, RM32\n" );
        uint op1, op2, res;
        if ( data.usesMemory() )
        {
                op1 = readMemory(data.address());
                op2 = readRegister(data.reg1())+testFlag(FLAG_CF);
                res = op1 + op2;
		writeRegister(data.reg1(),res);
        }
        else
        {
                op1 = readRegister(data.reg1());
                op2 = readRegister(data.reg2())+testFlag(FLAG_CF);
                res = op1 + op2;
                GR.write(data.reg1(),res);
        }
        checkFlags(OPER_ADD,op1,op2,res,FLAG_ALL);
}

//!Instruction sub_EAX_imm32 behavior method.
void ac_behavior( sub_EAX_imm32 )
{
	acprintf ( "SUB EAX, IMM32\n" );
	uint op1, op2, res;
	op1 = readRegister(EAX);
	op2 = dataManager.getImmediate();
	res = op1 - op2;
	writeRegister(EAX, res);
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction sub_rm32_r32 behavior method.
void ac_behavior( sub_rm32_r32 )
{
	uint op1, op2, res;
	//acprintf ( "SUB RM32, R32\n" );
	printf ( "SUB " );
	if ( data.usesMemory() ) printf ( "[0x%08X], ", data.address() );
	else
	{
	  if ( OperandSize == OPERAND_SIZE16 ) printf ( "%s, ", reg_str16[data.reg2()] );
	  else printf ( "%s, ", reg_str[data.reg2()] );
	}
	op1 = dataManager.getMemOrReg2();
	op2 = dataManager.getReg1();
	if ( OperandSize == OPERAND_SIZE16 ) printf ( "%s\n", reg_str16[data.reg1()] );
	else printf ( "%s\n", reg_str[data.reg1()] );
	res = op1 - op2;
	dataManager.setMemOrReg2(res);
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction sub_r32_rm32 behavior method.
void ac_behavior( sub_r32_rm32 )
{
        acprintf ( "SUB R32, RM32\n" );
        uint op1, op2, res;
	op1 = dataManager.getReg1();
	op2 = dataManager.getMemOrReg2();
	res = op1 - op2;
	dataManager.setReg1(res);
        checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction sbb_EAX_imm32 behavior method.
void ac_behavior( sbb_EAX_imm32 )
{
	acprintf ( "SBB EAX, IMM32\n" );
	uint op1, op2, res, carry=0;
	if ( testFlag ( FLAG_CF ) ) carry=1;
	op1 = readRegister(EAX);
	op2 = dataManager.getImmediate() + carry;
	res = op1 - op2;
	writeRegister(EAX,res);
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction sbb_rm32_r32 behavior method.
void ac_behavior( sbb_rm32_r32 )
{
	acprintf ( "SBB RM32, R32\n" );
	uint op1, op2, res, carry=0;
	if ( testFlag ( FLAG_CF ) ) carry = 1;
	op1 = dataManager.getMemOrReg2();
	op2 = dataManager.getReg1() + carry;
	res = op1 - op2;
	dataManager.setMemOrReg2(res);
	checkFlags ( OPER_SUB,op1,op2,res,FLAG_ALL );
}

//!Instruction sbb_r32_rm32 behavior method.
void ac_behavior( sbb_r32_rm32 )
{
        acprintf ( "SBB R32, RM32\n" );
        uint op1, op2, res, carry=0;
        if ( testFlag ( FLAG_CF ) ) carry = 1;
	op1 = dataManager.getReg1();
	op2 = dataManager.getMemOrReg2() + carry;
	res = op1 - op2;
	dataManager.setReg1(res);
        checkFlags ( OPER_SUB,op1,op2,res,FLAG_ALL );
}

//!Instruction mul_rm32 behavior method.
void ac_behavior( mul_rm32 )
{
	acprintf ( "MUL RM32\n" );
	uint op1 = readRegister(EAX);
	uint op2 = dataManager.getMemOrReg2();
	unsigned long long lop1 = op1;
	unsigned long long lop2 = op2;
	unsigned long long res = lop1 * lop2;
	if (	((OperandSize==OPERAND_SIZE32)&&(res>0xFFFFFFFF))||
		((OperandSize==OPERAND_SIZE16)&&(res>0xFFFF)) )
	{
		setFlag(FLAG_OF);
		setFlag(FLAG_CF);
	}
	else
	{
		resetFlag(FLAG_OF);
		resetFlag(FLAG_CF);
	}
	uint mask = 0xFFFFFFFF;
	uint disp = 32;
	if ( OperandSize == OPERAND_SIZE16 )
	{
		mask = 0xFFFF;
		disp = 16;
	}
	writeRegister(EDX, res >> disp );
	writeRegister(EAX, res & mask );
}

//!Instruction imul_r32_rm32_imm8 behavior method.
void ac_behavior( imul_r32_rm32_imm8 )
{
	acprintf ( " IMUL R32, RM32, IMM8\n" );
	signed int op1, op2;
	signed long long aux;
	uint64 mask = 0xFFFFFFFF;
	mask = mask<<32;
	if ( data.usesMemory() )
	{
		op1 = data.immediate8();
		op2 = MEM.read(data.address());
		aux = op1*op2;
		GR.write(data.reg1(),aux);
	}
	else
	{
		op1 = data.immediate8();
		op2 = GR.read(data.reg1());
		aux = op1*op2;
		GR.write(data.reg2(),aux);
	}
	if ( aux & mask )
	{
		setFlag(FLAG_OF);
		setFlag(FLAG_CF);
	}
	else
	{
		resetFlag(FLAG_OF);
		resetFlag(FLAG_CF);
	}
}

//!Instruction imul_r32_rm32_imm32 behavior method.
void ac_behavior( imul_r32_rm32_imm32 )
{
	acprintf ( " IMUL R32, RM32, IMM32\n" );
	signed int op1, op2;
	signed long long aux;
	uint64 mask = 0xFFFFFFFF;
	mask = mask<<32;
	if ( data.usesMemory() )
	{
		op1 = data.immediate();
		op2 = MEM.read(data.address());
		aux = op1*op2;
		GR.write(data.reg1(),aux);
	}
	else
	{
		op1 = data.immediate();
		op2 = GR.read(data.reg1());
		aux = op1*op2;
		GR.write(data.reg2(),aux);
	}
	if ( aux & mask )
	{
		setFlag(FLAG_OF);
		setFlag(FLAG_CF);
	}
	else
	{
		resetFlag(FLAG_OF);
		resetFlag(FLAG_CF);
	}
}

//!Instruction cmp_EAX_imm32 behavior method.
void ac_behavior( cmp_EAX_imm32 )
{
	acprintf ( "CMP EAX, IMM32\n" );
	uint op1, op2, aux;
	op1 = readRegister(EAX);
	op2 = dataManager.getImmediate();
	aux = op1 + (~op2) + 0x01;
	checkFlags(OPER_SUB,op1,op2,aux,FLAG_ALL);
}

//!Instruction cmp_rm32_r32 behavior method.
void ac_behavior( cmp_rm32_r32 )
{
	acprintf ( "CMP RM32, R32\n" );
	uint op1, op2, aux;
	if ( data.usesMemory() )
	{
		op1 = readMemory(data.address());
		op2 = readRegister(data.reg1());
	}
	else
	{
		op2 = readRegister(data.reg1());
		op1 = readRegister(data.reg2());
	}
	aux = op1 + (~op2) + 0x01;
	checkFlags(OPER_SUB,op1,op2,aux,FLAG_ALL);
}

//!Instruction cmp_r32_rm32 behavior method.
void ac_behavior( cmp_r32_rm32 )
{
	acprintf ( "CMP R32, RM32\n" );
	uint op1, op2, res;
	op1 = dataManager.getReg1();
	op2 = dataManager.getMemOrReg2();
	res = op1 - op2;
	checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction and_EAX_imm32 behavior method.
void ac_behavior( and_EAX_imm32 )
{
	acprintf ( "AND EAX, IMM32\n" );
	uint op1, op2, res;
	op1 = readRegister(EAX);
	op2 = dataManager.getImmediate();
	res = op1 & op2;
	writeRegister(EAX, res);
	resetFlag ( FLAG_OF );
	resetFlag ( FLAG_CF );
	checkFlags( OPER_NONE,op1,op2,res,FLAG_SF|FLAG_ZF|FLAG_PF );
}

//!Instruction and_rm32_r32 behavior method.
void ac_behavior( and_rm32_r32 )
{
	acprintf ( "AND RM32, R32\n" );
	uint op1, op2, res;
	op1 = dataManager.getMemOrReg2();
	op2 = dataManager.getReg1();
	res = op1 & op2;
	dataManager.setMemOrReg2(res);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_ALL);
	resetFlag(FLAG_CF); resetFlag(FLAG_OF);
}

//!Instruction and_r32_rm32 behavior method.
void ac_behavior( and_r32_rm32 )
{
        acprintf ( "AND R32, RM32\n" );
        uint op1, op2, res;
	op1 = dataManager.getMemOrReg2();
	op2 = dataManager.getReg1();
	res = op1 & op2;
	dataManager.setReg1(res);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_ALL);
	resetFlag(FLAG_CF); resetFlag(FLAG_OF);
}

//!Instruction or_EAX_imm32 behavior method.
void ac_behavior( or_EAX_imm32 )
{
        acprintf ( "OR EAX, IMM32\n" );
        uint op1, op2, res;
        op1 = readRegister(EAX);
        op2 = dataManager.getImmediate();
        res = op1 | op2;
	writeRegister(EAX, res);
        resetFlag ( FLAG_OF );
        resetFlag ( FLAG_CF );
        checkFlags( OPER_NONE,op1,op2,res,FLAG_SF|FLAG_ZF|FLAG_PF );
}

//!Instruction or_rm32_r32 behavior method.
void ac_behavior( or_rm32_r32 )
{
        acprintf ( "OR RM32, R32\n" );
        uint op1, op2, res;
	op1 = dataManager.getMemOrReg2();
	op2 = dataManager.getReg1();
	res = op1 | op2;
	dataManager.setMemOrReg2(res);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_ALL);
	resetFlag(FLAG_OF);resetFlag(FLAG_CF);
}

//!Instruction or_r32_rm32 behavior method.
void ac_behavior( or_r32_rm32 )
{
        acprintf ( "OR R32, RM32\n" );
        uint op1, op2, res;
	op1 = dataManager.getMemOrReg2();
	op2 = dataManager.getReg1();
	res = op1|op2;
	dataManager.setReg1(res);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_ALL);
	resetFlag(FLAG_OF); resetFlag(FLAG_CF);
}

//!Instruction xor_EAX_imm32 behavior method.
void ac_behavior( xor_EAX_imm32 )
{
        acprintf ( "XOR EAX, IMM32\n" );
        uint op1, op2, res;
        op1 = readRegister(EAX);
        op2 = dataManager.getImmediate();
        res = op1 ^ op2;
	writeRegister(EAX, res);
        resetFlag ( FLAG_OF );
        resetFlag ( FLAG_CF );
        checkFlags( OPER_NONE,op1,op2,res,FLAG_SF|FLAG_ZF|FLAG_PF );
}

//!Instruction xor_rm32_r32 behavior method.
void ac_behavior( xor_rm32_r32 )
{
        acprintf ( "XOR RM32, R32\n" );
        uint op1, op2, res;
	op1 = dataManager.getMemOrReg2();
	op2 = dataManager.getReg1();
	res = op1 ^ op2;
	dataManager.setMemOrReg2(res);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_ALL);
	resetFlag(FLAG_OF); resetFlag(FLAG_CF);
}

//!Instruction xor_r32_rm32 behavior method.
void ac_behavior( xor_r32_rm32 )
{
        acprintf ( "XOR R32, RM32\n" );
        uint op1, op2, res;
	op1 = dataManager.getMemOrReg2();
	op2 = dataManager.getReg1();
	res = op1^op2;
	dataManager.setReg1(res);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_ALL);
	resetFlag(FLAG_OF); resetFlag(FLAG_CF);
}

//!Instruction sal_shl_rm32 behavior method.
void ac_behavior( sal_shl_rm32 )
{
	// acprintf ( "SAL RM32\n" );
	printf ( "SAL " );
	unsigned int op = dataManager.getMemOrReg2();
	unsigned int res = op << 1;
	dataManager.setMemOrReg2(res);
	if ( data.usesMemory() ) printf ( "[0x%08X]\n", data.address() );
	else
	{
		if ( OperandSize == OPERAND_SIZE16 ) printf ( "%s\n", reg_str16[data.reg2()] );
		else printf ( "%s\n", reg_str[data.reg2()] );
	}
	if ( OperandSize == OPERAND_SIZE16 ) writeFlag(FLAG_CF, MSB16(op));
	else if ( OperandSize == OPERAND_SIZE32 ) writeFlag(FLAG_CF, MSB32(op));
	bool of16 = (testFlag(FLAG_CF)&&!MSB16(res))||(!testFlag(FLAG_CF)&&MSB16(res));
	bool of32 = (testFlag(FLAG_CF)&&!MSB32(res))||(!testFlag(FLAG_CF)&&MSB32(res));
	if ( OperandSize == OPERAND_SIZE16 ) writeFlag(FLAG_OF, of16);
	else if ( OperandSize == OPERAND_SIZE32 ) writeFlag(FLAG_OF, of32);
}

//!Instruction sar_rm32 behavior method
void ac_behavior( sar_rm32 )
{
	//acprintf ( "SAR RM32\n" );
	printf ( "SAR " );
	if ( data.usesMemory() ) printf ( "[0x%08X]\n", data.address() );
	else
	{
		if ( OperandSize == OPERAND_SIZE16 ) printf ( "%s\n", reg_str16[data.reg2()] );
		else printf ( "%s\n", reg_str[data.reg2()] );
	}
	signed int op = dataManager.getMemOrReg2();
	signed int res = op >> 1;
	dataManager.setMemOrReg2(res);
	checkFlags(OPER_NONE,0,0,res,FLAG_ALL);
	resetFlag(FLAG_OF);
	writeFlag(FLAG_CF, op&0x01);
}

//!Instruction shr_rm32 behavior method
void ac_behavior( shr_rm32 )
{
	acprintf ( "SHR RM32\n" );
	uint op = dataManager.getMemOrReg2();
	if ( OperandSize == OPERAND_SIZE16 ) op = op&MASK_16BITS;
	uint res = op >> 1;
	dataManager.setMemOrReg2(res);
	checkFlags(OPER_NONE,0,0,res,FLAG_ALL);
	writeFlag(FLAG_CF, op&0x01);
	if ( OperandSize == OPERAND_SIZE16 ) writeFlag(FLAG_OF,MSB16(op));
	else if ( OperandSize == OPERAND_SIZE32 ) writeFlag(FLAG_OF,MSB32(op));
}

//!Instruction sal_shl_rm32_imm8 behavior method
void ac_behavior( sal_shl_rm32_imm8 )
{
	//acprintf( "SAL RM32, IMM8\n" );
	acprintf("SAL ");
	if ( data.usesMemory() ) printf ( "[0x%08X], ", data.address() );
	else
	{
		if ( OperandSize == OPERAND_SIZE16 ) printf ( "%s, ", reg_str16[data.reg2()] );
		else printf ( "%s, ", reg_str[data.reg2()] );
	}
	unsigned int op = dataManager.getMemOrReg2();
	signed char count = data.immediate8();
	unsigned int res = op << count;
	printf ( "0x%02X\n", count );
	dataManager.setMemOrReg2(res);
	checkFlags(OPER_NONE,0,0,res,FLAG_ALL);
	if ( OperandSize == OPERAND_SIZE16 ) writeFlag(FLAG_CF,MSB16(op));
	else if ( OperandSize == OPERAND_SIZE32 ) writeFlag(FLAG_CF,MSB32(op));
	bool of16 = (!testFlag(FLAG_CF)&&MSB16(res))||(testFlag(FLAG_CF)&&!MSB16(res));
	bool of32 = (!testFlag(FLAG_CF)&&MSB32(res))||(testFlag(FLAG_CF)&&!MSB32(res));
	if ( OperandSize == OPERAND_SIZE16 ) writeFlag(FLAG_OF, of16);
	else if ( OperandSize == OPERAND_SIZE32 ) writeFlag(FLAG_OF, of32 );
}

//!Instruction sar_rm32_imm8 behavior method
void ac_behavior( sar_rm32_imm8 )
{
	//acprintf ( "SAR RM32, IMM8\n" );
	printf ( "SAR " );
	if ( data.usesMemory() ) printf ( "[0x%08X], ", data.address() );
	else
	{
		if ( OperandSize == OPERAND_SIZE16 ) printf ( "%s, ", reg_str16[data.reg2()] );
		else printf ( "%s, ", reg_str[data.reg2()] );
	}
	signed int op = dataManager.getMemOrReg2();
	signed char count = data.immediate8();
	printf ( "0x%02X\n", count );
	signed int res = op >> count;
	dataManager.setMemOrReg2(res);
	checkFlags(OPER_NONE,0,0,res,FLAG_ALL);
	if ( count > 0 ) writeFlag(FLAG_CF, (op>>(count-1))&0x01);
}

//!Instruction shr_rm32_imm8 behavior method
void ac_behavior( shr_rm32_imm8 )
{
	printf ( "SHR RM32, IMM8\n" );
	signed char count;
	unsigned int op, res;
	op = dataManager.getMemOrReg2();
	if ( OperandSize == OPERAND_SIZE16 ) op = op & MASK_16BITS;
	count = data.immediate8();
	res = op >> count;
	dataManager.setMemOrReg2(res);
	checkFlags(OPER_NONE,0,0,res,FLAG_ALL);
	if ( OperandSize == OPERAND_SIZE16 ) writeFlag(FLAG_OF, MSB16(op));
	else if ( OperandSize == OPERAND_SIZE32 ) writeFlag(FLAG_OF, MSB32(op));
	if ( count > 0 ) writeFlag(FLAG_CF, (op>>(count-1))&0x01);
}

//!Instruction test_eax_imm32 behavior method.
void ac_behavior( test_eax_imm32 )
{
	acprintf ( " TEST EAX, IMM32\n" );
	uint op1, op2, aux;
	op1 = GR.read(EAX);
	op2 = data.immediate();
	aux = op1 & op2;
	resetFlag ( FLAG_OF );
	resetFlag ( FLAG_CF );
	checkFlags ( OPER_NONE,op1,op2,aux,FLAG_SF|FLAG_ZF|FLAG_PF );
}

//!Instruction test_rm32_r32 behavior method.
void ac_behavior( test_rm32_r32 )
{
	acprintf ( "TEST RM32, R32\n" );
	uint op1, op2, res;
	op1 = dataManager.getMemOrReg2();
	op2 = dataManager.getReg1();
	res = op1 & op2;
	resetFlag ( FLAG_OF );
	resetFlag ( FLAG_CF );
	checkFlags ( OPER_NONE,op1,op2,res,FLAG_SF|FLAG_ZF|FLAG_PF );
}

//!Instruction ja_jnbe_rel8 behavior method.
void ac_behavior( ja_jnbe_rel8 )
{
	acprintf ( " JA/JNBE REL8\n" );
	if ( testFlag(FLAG_CF)==false && testFlag(FLAG_ZF)==false )
	{
		if ( data.address8()&(1<<7) ) ac_pc-= ((~data.address8())+0x01);
		else ac_pc+= data.address8();
	}
}

//!Instruction jae_jnb_jnc_rel8 behavior method.
void ac_behavior( jae_jnb_jnc_rel8 )
{
        acprintf ( " JAE/JNB/JNC REL8\n" );
        if ( testFlag(FLAG_CF)==false )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jb_jc_jnae_rel8 behavior method.
void ac_behavior( jb_jc_jnae_rel8 )
{
        acprintf ( " JB/JC/JNAE REL8\n" );
        if ( testFlag(FLAG_CF)==true )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jbe_jna_rel8 behavior method.
void ac_behavior( jbe_jna_rel8 )
{
        acprintf ( " JBE/JNA REL8\n" );
        if ( (testFlag(FLAG_CF)==true) || (testFlag(FLAG_ZF)==true) )
        {
		printf ( "Branch taken\n" );
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction je_jz_rel8 behavior method.
void ac_behavior( je_jz_rel8 )
{
        acprintf ( " JE/JZ REL8\n" );
        if ( testFlag(FLAG_ZF)==true )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jg_jnle_rel8 behavior method.
void ac_behavior( jg_jnle_rel8 )
{
        acprintf ( " JG/JNLE REL8\n" );
        if ( testFlag(FLAG_ZF)==false && (testFlag(FLAG_SF)==testFlag(FLAG_OF)) )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jge_jnl_rel8 behavior method.
void ac_behavior( jge_jnl_rel8 )
{
        acprintf ( "JGE/JNL REL8\n" );
        if ( testFlag(FLAG_SF)==testFlag(FLAG_OF) )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jl_jnge_rel8 behavior method.
void ac_behavior( jl_jnge_rel8 )
{
        acprintf ( " JL/JNGE REL8\n" );
        if ( testFlag(FLAG_SF)!=testFlag(FLAG_OF) )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jle_jng_rel8 behavior method.
void ac_behavior( jle_jng_rel8 )
{
        acprintf ( "JLE/JNG REL8\n" );
        if ( (testFlag(FLAG_ZF)==true) || ((testFlag(FLAG_SF)!=testFlag(FLAG_OF))) )
        {
		printf ( "Branch taken\n" );
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jne_jnz_rel8 behavior method.
void ac_behavior( jne_jnz_rel8 )
{
        acprintf ( " JNE/JNZ REL8\n" );
        if ( testFlag(FLAG_ZF)==false )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jno_rel8 behavior method.
void ac_behavior( jno_rel8 )
{
        acprintf ( " JNO REL8\n" );
        if ( testFlag(FLAG_OF)==false )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jnp_jpo_rel8 behavior method.
void ac_behavior( jnp_jpo_rel8 )
{
        acprintf ( " JNP/JPO REL8\n" );
        if ( testFlag(FLAG_PF)==false )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jns_rel8 behavior method.
void ac_behavior( jns_rel8 )
{
        acprintf ( " JNS REL8\n" );
        if ( testFlag(FLAG_SF)==false )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jo_rel8 behavior method.
void ac_behavior( jo_rel8 )
{
        acprintf ( " JO REL8\n" );
        if ( testFlag(FLAG_OF)==true )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jp_jpe_rel8 behavior method.
void ac_behavior( jp_jpe_rel8 )
{
        acprintf ( " JP/JPE REL8\n" );
        if ( testFlag(FLAG_PF)==true )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction js_rel8 behavior method.
void ac_behavior( js_rel8 )
{
        acprintf ( " JS REL8\n" );
        if ( testFlag(FLAG_SF)==true )
        {
                if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
                else ac_pc+= data.address8();
        }
}

//!Instruction jmp_rel8 behavior method.
void ac_behavior( jmp_rel8 )
{
	printf ( "JMP REL8\n" );
	signed char d = data.address8();
	ac_pc+=d;
}

//!Instruction jcxz_jecxz_rel8 behavior method.
void ac_behavior( jcxz_jecxz_rel8 )
{
	acprintf ( " JECXZ REL8\n" );
	if ( GR.read(ECX)==0x00000000 )
	{
		if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
		else ac_pc+= data.address8();
	}
}

//!Instruction jmp_rel32 behavior method.
void ac_behavior( jmp_rel32 )
{
	// acprintf ( " JMP REL32\t\t" );
	acprintf("JMP "); acprinti(ac_pc+data.address()); acprintf("\n");
	signed int rel = data.address();
	ac_pc+= rel;
}

//!Instruction jmp_ptr16_32 behavior method.
void ac_behavior( jmp_ptr16_32 )
{
	acprintf ( " JMP PTR16:32 <Not implement>\n" );
}

//!Instruction loop_rel8 behavior method.
void ac_behavior( loop_rel8 )
{
	acprintf ( " LOOP REL8\n" );
	uint aux = GR.read(ECX);
	aux--;
	if ( aux > 0x00000000 )
	{
		if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
		else ac_pc+= data.address8();
		GR.write(ECX,aux);
	}
}

//!Instruction loope_loopz_rel8 behavior method.
void ac_behavior( loope_loopz_rel8 )
{
	acprintf ( " LOOPE/LOOPZ REL8\n" );
	uint aux = GR.read(ECX);
	aux--;
	if ( (aux > 0x00000000) && testFlag(FLAG_ZF)==true )
	{
		if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
		else ac_pc+= data.address8();
		GR.write(ECX,aux);
	}
}

//!Instruction loopne_loopnz_rel8 behavior method.
void ac_behavior( loopne_loopnz_rel8 )
{
	acprintf ( " LOOPNE/LOOPNZ REL8\n" );
	uint aux = GR.read(ECX);
	aux--;
	if ( (aux > 0x00000000) && testFlag(FLAG_ZF)==false )
	{
		if ( data.address8()&(1<<7) ) ac_pc-=((~data.address8())+0x01);
		else ac_pc+= data.address8();
		GR.write(ECX,aux);
	}
}

//!Instruction call_rel32 behavior method.
void ac_behavior( call_rel32 )
{
	// acprintf ( "CALL REL32\t\t" );
	push((int)ac_pc);
	signed int a = data.address();
	acprintf("CALL "); acprinti(ac_pc+a); acprintf("\n");
	ac_pc+= a;
}

//!Instruction call_ptr16_32 behavior method.
void ac_behavior( call_ptr16_32 )
{
	acprintf ( "CALL PTR16:32 <Not implement>\n" );
}

//!Instruction ret_near behavior method.
void ac_behavior( ret_near )
{
	acprintf ( "RET\n" );
	ac_pc = pop();
	SPR.write(EIP,(int)ac_pc);
}

//!Instruction ret_far behavior method.
void ac_behavior( ret_far )
{
	acprintf ( "RET\n" );
	ac_pc = pop();
	SPR.write(EIP,(int)ac_pc);
}

//!Instruction ret_near_imm16 behavior method.
void ac_behavior( ret_near_imm16 )
{
	acprintf ( "RET IMM16 <Not implemented>\n" );
}

//!Instruction ret_far_imm16 behavior method.
void ac_behavior( ret_far_imm16 )
{
	acprintf ( "RET IMM16 <Not implemented>\n" );
}

//!Instruction int_3 behavior method.
void ac_behavior( int_3 )
{
	acprintf ( "INT 3 <Not implemented>\n" );
}

//!Instruction int_imm8 behavior method.
void ac_behavior( int_imm8 )
{
	acprintf ( "INT IMM8 <Not implemented>\n" );
}

//!Instruction into behavior method.
void ac_behavior( into )
{
	acprintf ( "INTO <Not implemented>\n" );
}

//!Instruction bound_r32_m32_32 behavior method.
void ac_behavior( bound_r32_m32_32 )
{
	acprintf ( "BOUND R32, M32&32 <Partially implemented>\n" );
	signed int r = GR.read(data.reg1());
	signed int lb = MEM.read(data.address());
	signed int ub = MEM.read(data.address()+4);
	if ( (r<lb) || (r>ub) )
		;	// Exception #BR
}

//!Instruction enter_imm16_0_1_imm8 behavior method.
void ac_behavior( enter_imm16_0_1_imm8 )
{
	acprintf ( "ENTER IMM16, IMM8 <Not implemented>\n" );
}

//!Instruction lss_r32_m16_32 behavior method.
void ac_behavior( lss_r32_m16_32 )
{
  acprintf ( "LSS R32, M16:32\n" );
  unsigned char b0 = MEM.read_byte(data.address());
  unsigned char b1 = MEM.read_byte(data.address()+1);
  uint offset = readMemory(data.address()+2);
  uint ds_data = (b1<<8) + b0;
  writeSRegister(SS, ds_data);
  writeRegister(data.reg1(), offset);
}

//!Instruction lfs_r32_m16_32 behavior method.
void ac_behavior( lfs_r32_m16_32 )
{
  acprintf ( "LFS R32,M16:32\n" );
  unsigned char b0 = MEM.read_byte(data.address());
  unsigned char b1 = MEM.read_byte(data.address()+1);
  uint offset = readMemory(data.address()+2);
  uint ds_data = (b1<<8) + b0;
  writeSRegister(FS, ds_data);
  writeRegister(data.reg1(), offset);
}

//!Instruction lgs_r32_m16_32 behavior method.
void ac_behavior( lgs_r32_m16_32 )
{
  acprintf ( "LGS R32,M16:32\n" );
  unsigned char b0 = MEM.read_byte(data.address());
  unsigned char b1 = MEM.read_byte(data.address()+1);
  uint offset = readMemory(data.address()+2);
  uint ds_data = (b1<<8) + b0;
  writeSRegister(GS, ds_data);
  writeRegister(data.reg1(), offset);
}

//!Instruction cmov_a_nbe_r32_rm32 behavior method.
void ac_behavior( cmov_a_nbe_r32_rm32 )
{
  acprintf ( "CMOVA/CMOVNBE R32, RM32\n" );
  if ( (testFlag(FLAG_CF)==false) && (testFlag(FLAG_ZF)==false) )
    {
      if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
      else writeRegister(data.reg1(), readRegister(data.reg2()));
    }
}

//!Instruction cmov_ae_nb_nc_r32_rm32 behavior method.
void ac_behavior( cmov_ae_nb_nc_r32_rm32 )
{
  acprintf ( "CMOVAE/CMOVNB/CMOVNC R32, RM32\n" );
  if ( testFlag(FLAG_CF)==false )
    {
      if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
      else writeRegister(data.reg1(), readRegister(data.reg2()));
    }
}

//!Instruction cmov_b_c_nae_r32_rm32 behavior method.
void ac_behavior( cmov_b_c_nae_r32_rm32 )
{
  acprintf ( "CMOVB/CMOVC/CMOVNAE R32, RM32\n" );
  if ( testFlag(FLAG_CF)==true )
    {
      if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
      else writeRegister(data.reg1(), readRegister(data.reg2()));
    }
}

//!Instruction cmov_be_na_r32_rm32 behavior method.
void ac_behavior( cmov_be_na_r32_rm32 )
{
  acprintf ( "CMOVBE/CMOVNA R32, RM32\n" );
  if ( testFlag(FLAG_CF)==true && testFlag(FLAG_ZF)==true )
    {
      if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
      else writeRegister(data.reg1(), readRegister(data.reg2()));
    }
}

//!Instruction cmov_e_z_r32_rm32 behavior method.
void ac_behavior( cmov_e_z_r32_rm32 )
{
  acprintf ( "CMOVE/CMOVZ R32, RM32\n" );
  if ( testFlag(FLAG_ZF)==true )
    {
      if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
      else writeRegister(data.reg1(), readRegister(data.reg2()));
    }
}

//!Instruction cmov_g_nle_r32_rm32 behavior method.
void ac_behavior( cmov_g_nle_r32_rm32 )
{
  acprintf ( "CMOVG/CMOVNLE R32, RM32\n" );
  if ( testFlag(FLAG_ZF)==false && (testFlag(FLAG_SF)==testFlag(FLAG_OF)) )
    {
      if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
      else writeRegister(data.reg1(), readRegister(data.reg2()));
    }
}

//!Instruction cmov_ge_nl_r32_rm32 behavior method.
void ac_behavior( cmov_ge_nl_r32_rm32 )
{
  acprintf ( "CMOVGE/CMOVNL R32, RM32\n" );
  if ( testFlag(FLAG_SF)==testFlag(FLAG_OF) )
    {
      if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
      else writeRegister(data.reg1(), readRegister(data.reg2()));
    }
}

//!Instruction cmov_l_nge_r32_rm32 behavior method.
void ac_behavior( cmov_l_nge_r32_rm32 )
{
  acprintf ( "CMOVL/CMOVNGE R32, RM32\n" );
  if ( testFlag(FLAG_SF)!=testFlag(FLAG_OF) )
    {
      if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
      else writeRegister(data.reg1(), readRegister(data.reg2()));
    }
}

//!Instruction cmov_le_ng_r32_rm32 behavior method.
void ac_behavior( cmov_le_ng_r32_rm32 )
{
	acprintf ( "CMOVLE/CMOVNG R32, RM32\n" );
	if ( testFlag(FLAG_ZF)==true || (testFlag(FLAG_SF)!=testFlag(FLAG_OF)) )
	{
                if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
                else writeRegister(data.reg1(), readRegister(data.reg2()));
	}
}

//!Instruction cmov_ne_nz_r32_rm32 behavior method.
void ac_behavior( cmov_ne_nz_r32_rm32 )
{
	acprintf ( "CMOVNE/CMOVNZ R32, RM32\n" );
	if ( testFlag(FLAG_ZF)==false )
	{
                if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
                else writeRegister(data.reg1(), readRegister(data.reg2()));
	}
}

//!Instruction cmov_no_r32_rm32 behavior method.
void ac_behavior( cmov_no_r32_rm32 )
{
	acprintf ( "CMOVNO R32, RM32\n" );
	if ( testFlag(FLAG_OF)==false )
	{
                if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
                else writeRegister(data.reg1(), readRegister(data.reg2()));
	}
}

//!Instruction cmov_np_po_r32_rm32 behavior method.
void ac_behavior( cmov_np_po_r32_rm32 )
{
	acprintf ( "CMOVNP/CMOVPO R32, RM32\n" );
	if ( testFlag(FLAG_PF)==false )
	{
                if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
                else writeRegister(data.reg1(), readRegister(data.reg2()));
	}
}

//!Instruction cmov_ns_r32_rm32 behavior method.
void ac_behavior( cmov_ns_r32_rm32 )
{
	acprintf ( "CMOVNS R32, RM32\n" );
	if ( testFlag(FLAG_SF)==false )
	{
                if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
                else writeRegister(data.reg1(), readRegister(data.reg2()));
	}
}

//!Instruction cmov_o_r32_rm32 behavior method.
void ac_behavior( cmov_o_r32_rm32 )
{
	acprintf ( "CMOVO R32, RM32\n" );
	if ( testFlag(FLAG_OF)==true )
	{
                if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
                else writeRegister(data.reg1(), readRegister(data.reg2()));
	}
}

//!Instruction cmov_p_pe_r32_rm32 behavior method.
void ac_behavior( cmov_p_pe_r32_rm32 )
{
	acprintf ( "CMOVP/CMOVPE R32, RM32\n" );
	if ( testFlag(FLAG_PF)==true )
	{
                if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
                else writeRegister(data.reg1(), readRegister(data.reg2()));
	}
}

//!Instruction cmov_s_r32_rm32 behavior method.
void ac_behavior( cmov_s_r32_rm32 )
{
	acprintf ( "CMOVS R32, RM32\n" );
	if ( testFlag(FLAG_SF)==true )
	{
                if ( data.usesMemory() ) writeRegister(data.reg1(),readMemory(data.address()));
                else writeRegister(data.reg1(), readRegister(data.reg2()));
	}
}

//!Instruction xadd_rm32_r32 behavior method.
void ac_behavior( xadd_rm32_r32 )
{
	acprintf ( "XADD RM32,R32\n" );
	uint op1, op2, aux;
	if ( data.usesMemory() )
	{
		op1 = readMemory(data.address());
		op2 = readRegister(data.reg1());
		writeRegister(data.reg1(),op1);
		aux = op1 + op2;
		writeMemory(data.address(),aux);
	}
	else
	{
		op1 = readRegister(data.reg2());
		op2 = readRegister(data.reg1());
		writeRegister(data.reg1(),op1);
		aux = op1 + op2;
		writeRegister(data.reg2(),aux);
	}
	checkFlags(OPER_ADD,op1,op2,aux,FLAG_ALL);
}

//!Instruction cmpxchg_rm32_r32 behavior method.
void ac_behavior( cmpxchg_rm32_r32 )
{
	acprintf ( " CMPXCHG RM32, R32\n" );
	uint op1, op2, aux;
	if ( data.usesMemory() )
	{
		op1 = readMemory(data.address());
		op2 = readRegister(data.reg1());
		aux = op1 + (~readRegister(EAX)) + 0x01;
		if ( readRegister(EAX)==op1 )
			writeMemory(data.address(), op2);
		else
			writeRegister(EAX,op1);
	}
	else
	{
		op1 = readRegister(data.reg2());
		op2 = readRegister(data.reg1());
		aux = op1 + (~readRegister(EAX)) + 0x01;
		if ( readRegister(EAX)==op1 )
			writeRegister(data.reg2(),op2);
		else
			writeRegister(EAX,op1);
	}
	checkFlags(OPER_SUB,op1,GR.read(EAX),aux,FLAG_ALL);
}

//!Instruction cmpxchg8b_m64 behavior method.
void ac_behavior( cmpxchg8b_m64 )
{
	acprintf ( "CMPXCHG8B M64\n" );
	unsigned long long dest, edxeax, aux;
	dest = (((unsigned long long)readMemory(data.address()))<<32)+readMemory(data.address()+4);
	edxeax = (((unsigned long long)readRegister(EDX))<<32) + readRegister(EAX);
	aux = dest + (~edxeax) + 0x01;
	if ( edxeax == dest )
	{
		writeMemory(data.address(), readRegister(EBX));
		writeMemory(data.address()+4, readRegister(ECX));
		setFlag(FLAG_ZF);
	}
	else
	{
		writeRegister(EDX, (dest>>32));
		writeRegister(EAX, dest&0xFFFFFFFF);
		resetFlag(FLAG_ZF);
	}
}

//!Instruction movsx_r32_rm8 behavior method.
void ac_behavior( movsx_r32_rm8 )
{
	acprintf ( "MOVSX R32, RM8\n" );
	signed char op;
	signed int res;
	if ( data.usesMemory() )
	{
		op = MEM.read_byte(data.address());
		res = op;
	}
	else
	{
		op = readRegister8(data.reg2());
		res = op;
	}
	writeRegister(data.reg1(),res);
}

//!Instruction movsx_r32_rm16 behavior method.
void ac_behavior( movsx_r32_rm16 )
{
	// acprintf ( "MOVSX R32, RM16\n" );
	printf ( "MOVSX " );
	if ( OperandSize == OPERAND_SIZE16 ) printf ( "%s, ", reg_str16[data.reg1()] );
	else printf ( "%s, ", reg_str[data.reg1()] );
	signed short op;
	signed int res;
	if ( data.usesMemory() ) op = MEM.read_half(data.address());
	else op = GR.read(data.reg2());
	if ( data.usesMemory() ) printf ( "[0x%08X]\n", data.address() );
	else
	{
		if ( OperandSize == OPERAND_SIZE16 ) printf ( "%s\n", reg_str16[data.reg2()] );
		else printf ( "%s\n", reg_str[data.reg2()] );
	}
	res = op;
	writeRegister(data.reg1(),res);
}

//!Instruction movzx_r32_rm8 behavior method.
void ac_behavior( movzx_r32_rm8 )
{
	acprintf ( "MOVZX R32, RM8\n" );
	unsigned char op;
	if ( data.usesMemory() ) op = MEM.read_byte(data.address());
	else op = readRegister8(data.reg2());
	writeRegister(data.reg1(),op);
}

//!Instruction movzx_r32_rm16 behavior method.
void ac_behavior( movzx_r32_rm16 )
{
	acprintf ( "MOVZX R32, RM16\n" );
	unsigned short op;
	if ( data.usesMemory() ) op = MEM.read_half(data.address());
	else op = (readRegister(data.reg2())&MASK_16BITS);
	writeRegister(data.reg1(),op);
}

//!Instruction shrd_rm32_r32_imm8 behavior method.
void ac_behavior( shrd_rm32_r32_imm8 )
{
	printf ( "SHRD RM32, R32, IMM8\n" );
	if ( OperandSize == OPERAND_SIZE16 )
	{
		printf ( "NOT IMPLEMENTED!\n" );
	}
	else if ( OperandSize == OPERAND_SIZE32 )
	{
		signed char count = data.immediate8();
		unsigned int op1 = dataManager.getMemOrReg2();
		unsigned long long op2 = dataManager.getReg1();
		unsigned long long op = (op2<<32)|(op1);
		unsigned long long res = op >> count;
		dataManager.setMemOrReg2(res);
		dataManager.setReg1(res>>31);
		if ( count > 0 ) checkFlags(OPER_NONE,0,0,res,FLAG_ALL);
		if ( count > 0 ) writeFlag(FLAG_CF, (op>>(count-1))&0x01);
		if ( count == 1 ) writeFlag(FLAG_OF, MSB32(op1) != MSB32(res));
	}
}

//!Instruction shrd_rm32_r32_CL behavior method.
void ac_behavior( shrd_rm32_r32_CL )
{
	acprintf ( "SHRD RM32, R32, CL <Not implemented>\n" );
}

//!Instruction shld_rm32_r32_imm8 behavior method.
void ac_behavior( shld_rm32_r32_imm8 )
{
	printf ( "SHLD RM32, R32, IMM8\n" );
	if ( OperandSize == OPERAND_SIZE16 )
	{
		printf ( "NOT IMPLEMENTED!\n" );
	}
	else if ( OperandSize == OPERAND_SIZE32 )
	{
		signed char count = data.immediate8();
		unsigned long long op1 =dataManager.getMemOrReg2();
		unsigned int op2 = dataManager.getReg1();
		unsigned long long op = (op1<<32)|(op2);
		unsigned long long res = op << count;
		dataManager.setMemOrReg2(res>>32);
		dataManager.setReg1(res>>1);
		if ( count > 0 ) checkFlags(OPER_NONE,0,0,res,FLAG_ALL);
		if ( count > 0 ) writeFlag(FLAG_CF, MSB32(op<<(count-1)) );
		if ( count == 1 ) writeFlag(FLAG_OF, MSB32(op1) != MSB32(res));
	}
}

//!Instruction shld_rm32_r32_CL behavior method.
void ac_behavior( shld_rm32_r32_CL )
{
	acprintf ( "SHLD RM32, R32, CL <Not implemented>\n" );
}

//!Instruction bt_rm32_r32 behavior method.
void ac_behavior( bt_rm32_r32 )
{
	acprintf ( " BT RM32,R32\n" );
	if ( data.usesMemory() )
	{
		if ( testMemBit(data.address(), data.reg1()) ) setFlag(FLAG_CF);
		else resetFlag ( FLAG_CF );
	}
	else
	{
		if ( testBit ( readRegister(data.reg2()), readRegister(data.reg1()) ) ) setFlag(FLAG_CF);
		else resetFlag ( FLAG_CF );
	}
}

//!Instruction bt_rm32_imm8 behavior method.
void ac_behavior( bt_rm32_imm8 )
{
	acprintf ( "BT RM32, IMM8\n" );
	signed char i = data.immediate8();
	signed int op = i;
	if ( data.usesMemory() )
	{
		if ( testMemBit(data.address(), op) ) setFlag(FLAG_CF);
		else resetFlag ( FLAG_CF );
	}
	else
	{
		if ( testBit ( readRegister(data.reg2()), op ) ) setFlag(FLAG_CF);
		else resetFlag ( FLAG_CF );
	}
}

//!Instruction bts_rm32_r32 behavior method.
void ac_behavior( bts_rm32_r32 )
{
	uint target;
        acprintf ( "BTS RM32,R32\n" );
        if ( data.usesMemory() )
        {
		if ( testMemBit(data.address(),GR.read(data.reg1())) ) setFlag ( FLAG_CF );
		else resetFlag ( FLAG_CF );
		setMemBit ( data.address(), GR.read(data.reg1()) );
        }
        else
        {
		target = GR.read(data.reg2());
		if ( testBit ( target, GR.read(data.reg1()) ) ) setFlag(FLAG_CF);
		else resetFlag ( FLAG_CF );
		setBit ( &target, GR.read(data.reg1()) );
		GR.write(data.reg2(),target);
        }
}

//!Instruction btr_rm32_r32 behavior method.
void ac_behavior( btr_rm32_r32 )
{
        uint target;
        acprintf ( "BTR RM32,R32\n" );
        if ( data.usesMemory() )
        {
                if ( testMemBit(data.address(),GR.read(data.reg1())) ) setFlag ( FLAG_CF );
                else resetFlag ( FLAG_CF );
                resetMemBit ( data.address(), GR.read(data.reg1()) );
        }
        else
        {
                target = GR.read(data.reg2());
                if ( testBit ( target, GR.read(data.reg1()) ) ) setFlag(FLAG_CF);
                else resetFlag ( FLAG_CF );
                resetBit ( &target, GR.read(data.reg1()) );
                GR.write(data.reg2(),target);
        }
}

//!Instruction btc_rm32_r32 behavior method.
void ac_behavior( btc_rm32_r32 )
{
        uint target;
        acprintf ( "BTC RM32,R32\n" );
        if ( data.usesMemory() )
        {
                if ( testMemBit(data.address(),GR.read(data.reg1())) ) setFlag ( FLAG_CF );
                else resetFlag ( FLAG_CF );
                compMemBit ( data.address(), GR.read(data.reg1()) );
        }
        else
        {
                target = GR.read(data.reg2());
                if ( testBit ( target, GR.read(data.reg1()) ) ) setFlag(FLAG_CF);
                else resetFlag ( FLAG_CF );
                compBit ( &target, GR.read(data.reg1()) );
                GR.write(data.reg2(),target);
        }
}

//!Instruction bsf_r32_rm32 behavior method.
void ac_behavior( bsf_r32_rm32 )
{
	acprintf ( "BSF R32, RM32\n" );
	uint source, index=32;
	if ( data.usesMemory() ) source = readMemory(data.address());
	else source = readRegister(data.reg2());
	for ( uint i=0; i<32; i++ )
		if (testBit(source,i)) index = i;
	if ( index > 31 ) setFlag ( FLAG_ZF );
	else resetFlag ( FLAG_ZF );
	writeRegister(data.reg1(),index);
}

//!Instruction bsr_r32_rm32 behavior method.
void ac_behavior( bsr_r32_rm32 )
{
	acprintf ( "BSR R32, RM32\n" );
	uint source, index = 32;
	if ( data.usesMemory() ) source = readMemory(data.address());
	else source = readRegister(data.reg2());
	for ( uint i=31; i>=0; i-- )
		if ( testBit(source,i)) index = i;
	if ( index > 31 ) setFlag ( FLAG_ZF );
	else resetFlag ( FLAG_ZF );
	writeRegister(data.reg1(),index);
}

//!Instruction seta_setnbe_rm8 behavior method.
void ac_behavior( seta_setnbe_rm8 )
{
	acprintf ( " SETA/SETNBE RM8 <Not implemented>\n" );
}

//!Instruction setae_setnb_setnc_rm8 behavior method.
void ac_behavior( setae_setnb_setnc_rm8 )
{
  acprintf ( "SETAE/SETNB/SETNC RM8\n" );
  if ( testFlag(FLAG_CF)==false )
    {
      if ( data.usesMemory() ) MEM.write_byte(data.address(),1);
      else writeRegister8(data.reg2(),1);
    }
  else
    {
      if ( data.usesMemory() ) MEM.write_byte(data.address(),0);
      else writeRegister8(data.reg2(),0);
    }
}

//!Instruction setb_setc_setnae_rm8 behavior method.
void ac_behavior( setb_setc_setnae_rm8 )
{
  acprintf ( " SETB/SETC/SETNAE RM8 <Not implemented>\n" );
}

//!Instruction setbe_setna_rm8 behavior method.
void ac_behavior( setbe_setna_rm8 )
{
  acprintf ( " SETBE/SETNA RM8 <Not implemented>\n" );
}

//!Instruction sete_setz_rm8 behavior method.
void ac_behavior( sete_setz_rm8 )
{
  acprintf ( " SETE/SETZ RM8 <Not implemented>\n" );
}

//!Instruction setg_setnle_rm8 behavior method.
void ac_behavior( setg_setnle_rm8 )
{
  acprintf ( " SETG/SETNLE RM8 <Not implemented>\n" );
}

//!Instruction setge_setnl_rm8 behavior method.
void ac_behavior( setge_setnl_rm8 )
{
  acprintf ( " SETGE/SETNL RM8 <Not implemented>\n" );
}

//!Instruction setl_setnge_rm8 behavior method.
void ac_behavior( setl_setnge_rm8 )
{
  acprintf ( " SETL/SETNGE RM8 <Not implemented>\n" );
}

//!Instruction setle_setng_rm8 behavior method.
void ac_behavior( setle_setng_rm8 )
{
  acprintf ( " SETLE/SETNG RM8 <Not implemented>\n" );
}

//!Instruction setne_setnz_rm8 behavior method.
void ac_behavior( setne_setnz_rm8 )
{
  acprintf ( " SETNE/SETNZ RM8 <Not implemented>\n" );
}

//!Instruction setno_rm8 behavior method.
void ac_behavior( setno_rm8 )
{
  acprintf ( " SETNO RM8 <Not implemented>\n" );
}

//!Instruction setnp_setpo_rm8 behavior method.
void ac_behavior( setnp_setpo_rm8 )
{
  acprintf ( " SETNP/SETPO RM8 <Not implemented>\n" );
}

//!Instruction setns_rm8 behavior method.
void ac_behavior( setns_rm8 )
{
  acprintf ( " SETNS RM8 <Not implemented>\n" );
}

//!Instruction seto_rm8 behavior method.
void ac_behavior( seto_rm8 )
{
  acprintf ( " SETO RM8 <Not implemented>\n" );
}

//!Instruction setp_setpe_rm8 behavior method.
void ac_behavior( setp_setpe_rm8 )
{
  acprintf ( " SETP/SETPE RM8 <Not implemented>\n" );
}

//!Instruction sets_rm8 behavior method.
void ac_behavior( sets_rm8 )
{
  acprintf ( " SETS RM8 <Not implemented>\n" );
}

//!Instruction ja_jnbe_rel16_32 behavior method.
void ac_behavior( ja_jnbe_rel16_32 )
{
  acprintf ( " JA/JNBE REL32\n" );
  if ( testFlag(FLAG_CF)==false && testFlag(FLAG_ZF)==false )
    {
      if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
      else ac_pc+= data.address();
    }
}

//!Instruction jae_jnb_jnc_rel16_32 behavior method.
void ac_behavior( jae_jnb_jnc_rel16_32 )
{
  acprintf ( " JAE/JNB/JNC REL32\n" );
  if ( testFlag(FLAG_CF)==false )
    {
      if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
      else ac_pc+= data.address();
    }
}

	//!Instruction jb_jc_jnae_rel16_32 behavior method.
	void ac_behavior( jb_jc_jnae_rel16_32 )
	{
		acprintf ( " JB/JC/JNAE REL32\n" );
		if ( testFlag(FLAG_CF)==true )
		{
			if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
			else ac_pc+= data.address();
		}
	}

	//!Instruction jbe_jna_rel16_32 behavior method.
	void ac_behavior( jbe_jna_rel16_32 )
	{
		acprintf ( " JBE/JNA REL32\n" );
		if ( testFlag(FLAG_CF)==true || testFlag(FLAG_ZF)==true )
		{
			if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
			else ac_pc+= data.address();
		}
	}

	//!Instruction je_jz_rel16_32 behavior method.
	void ac_behavior( je_jz_rel16_32 )
	{
		acprintf ( " JE/JZ REL32\n" );
		if ( testFlag(FLAG_ZF)==true )
		{
			if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
			else ac_pc+= data.address();
		}
	}

	//!Instruction jg_jnle_rel16_32 behavior method.
	void ac_behavior( jg_jnle_rel16_32 )
	{
		acprintf ( " JG/JNLE REL32\n" );
		if ( testFlag(FLAG_ZF)==false && (testFlag(FLAG_SF)==testFlag(FLAG_OF)) )
		{
			if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
			else ac_pc+= data.address();
		}
	}

	//!Instruction jge_jnl_rel16_32 behavior method.
	void ac_behavior( jge_jnl_rel16_32 )
	{
		acprintf ( " JGE/JNL REL32\n" );
		if ( testFlag(FLAG_SF)==testFlag(FLAG_OF) )
		{
			if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
			else ac_pc+= data.address();
		}
	}

	//!Instruction jl_jnge_rel16_32 behavior method.
	void ac_behavior( jl_jnge_rel16_32 )
	{
		acprintf ( " JL/JNGE REL32\n" );
		if ( testFlag(FLAG_SF)!=testFlag(FLAG_OF) )
		{
			if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
			else ac_pc+= data.address();
		}
	}

	//!Instruction jle_jng_rel16_32 behavior method.
	void ac_behavior( jle_jng_rel16_32 )
	{
		acprintf ( " JLE/JNG REL32\n" );
		if ( testFlag(FLAG_ZF)==true || (testFlag(FLAG_SF)!=testFlag(FLAG_OF)) )
		{
			if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
			else ac_pc+= data.address();
		}
	}

	//!Instruction jne_jnz_rel16_32 behavior method.
	void ac_behavior( jne_jnz_rel16_32 )
	{
		acprintf ( " JNE/JNZ REL32\n" );
		if ( testFlag(FLAG_ZF)==false )
		{
			if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
			else ac_pc+= data.address();
		}
	}

	//!Instruction jno_rel16_32 behavior method.
	void ac_behavior( jno_rel16_32 )
	{
		acprintf ( " JNO REL32\n" );
		if ( testFlag(FLAG_OF)==false )
		{
			if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
			else ac_pc+= data.address();
		}
	}

	//!Instruction jnp_jpo_rel16_32 behavior method.
	void ac_behavior( jnp_jpo_rel16_32 )
	{
		acprintf ( " JNP/JPO REL32\n" );
		if ( testFlag(FLAG_PF)==false )
		{
			if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
			else ac_pc+= data.address();
		}
	}

	//!Instruction jns_rel16_32 behavior method.
void ac_behavior( jns_rel16_32 )
{
  acprintf ( " JNS REL32\n" );
  if ( testFlag(FLAG_SF)==false )
    {
      if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
      else ac_pc+= data.address();
    }
}

//!Instruction jo_rel16_32 behavior method.
void ac_behavior( jo_rel16_32 )
{
  acprintf ( " JO REL32\n" );
  if ( testFlag(FLAG_OF)==true )
    {
      if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
      else ac_pc+= data.address();
    }
}

//!Instruction jp_jpe_rel16_32 behavior method.
void ac_behavior( jp_jpe_rel16_32 )
{
  acprintf ( " JP/JPE REL32\n" );
  if ( testFlag(FLAG_PF)==true )
    {
      if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
      else ac_pc+= data.address();
    }
}

//!Instruction js_rel16_32 behavior method.
void ac_behavior( js_rel16_32 )
{
  acprintf ( " JS REL32\n" );
  if ( testFlag(FLAG_SF)==true )
    {
      if ( data.address()&(1<<31) ) ac_pc-=((~data.address())+0x01);
      else ac_pc+= data.address();
    }
}

//!Instruction bts_rm32_imm8 behavior method
void ac_behavior( bts_rm32_imm8 )
{
  acprintf ( " BTS RM32, IMM8\n" );
  signed char i = data.immediate8();
  int op = i;
  uint target;
  acprintf ( " BTS RM32,R32\n" );
  if ( data.usesMemory() )
    {
      if ( testMemBit(data.address(),op) ) setFlag ( FLAG_CF );
      else resetFlag ( FLAG_CF );
      setMemBit ( data.address(), op );
    }
  else
    {
      target = GR.read(data.reg2());
      if ( testBit ( target, op ) ) setFlag(FLAG_CF);
      else resetFlag ( FLAG_CF );
      setBit ( &target, op );
      GR.write(data.reg2(),target);
    }
}

//!Instruction btr_rm32_imm8 behavior method
void ac_behavior( btr_rm32_imm8 )
{
  acprintf ( " BTR RM32, IMM8\n" );
  signed char i = data.immediate8();
  int op = i;
  uint target;
  if ( data.usesMemory() )
    {
      if ( testMemBit(data.address(),op) ) setFlag ( FLAG_CF );
      else resetFlag ( FLAG_CF );
      resetMemBit ( data.address(), op );
    }
  else
    {
      target = GR.read(data.reg2());
      if ( testBit ( target, op ) ) setFlag(FLAG_CF);
      else resetFlag ( FLAG_CF );
      resetBit ( &target, op );
      GR.write(data.reg2(),target);
    }
}

//!Instruction btc_rm32_imm8 behavior method
void ac_behavior( btc_rm32_imm8 )
{
  acprintf ( " BTC RM32, IMM8\n" );
  signed char i =data.immediate8();
  int op = i;
  uint target;
  if ( data.usesMemory() )
    {
      if ( testMemBit(data.address(),op) ) setFlag ( FLAG_CF );
      else resetFlag ( FLAG_CF );
      compMemBit ( data.address(), op );
    }
  else
    {
      target = GR.read(data.reg2());
      if ( testBit ( target, op ) ) setFlag(FLAG_CF);
      else resetFlag ( FLAG_CF );
      compBit ( &target, op );
      GR.write(data.reg2(),target);
    }
}

//!Instruction inc_rm32 behavior method
void ac_behavior( inc_rm32 )
{
  acprintf ( "INC RM32\n" );
  uint op, res;
  op = dataManager.getMemOrReg2();
  res = op + 1;
  dataManager.setMemOrReg2(res);
  checkFlags(OPER_ADD,op,1,res,FLAG_ALL&(~FLAG_CF));
}

//!Instruction dec_rm32 behavior method
void ac_behavior( dec_rm32 )
{
  acprintf ( "DEC RM32\n" );
  if ( OperandSize == OPERAND_SIZE16 )
    {
      ushort op1, op2, res;
      op1 = dataManager.getMemOrReg2();
      op2 = 1;
      res = op1 - op2;
      dataManager.setMemOrReg2(res);
      checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
    }
  else if ( OperandSize == OPERAND_SIZE32 )
    {
      uint op1, op2, res;
      op1 = dataManager.getMemOrReg2();
      op2 = 1;
      res = op1 - op2;
      dataManager.setMemOrReg2(res);
      checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
    }
}

//!Instruction jmp_rm32 behavior method
void ac_behavior( jmp_rm32 )
{
  acprintf ( " JMP RM32\n" );
  if (data.usesMemory()) ac_pc=MEM.read(data.address());
  else ac_pc=GR.read(data.reg2());
}

//!Instruction jmp_m16_32 behavior method
void ac_behavior( jmp_m16_32 )
{
  acprintf ( " JMP M16:32 <Not implemented>\n" );
}

//!Instruction call_rm32 behavior method
void ac_behavior( call_rm32 )
{
  acprintf ( " CALL RM32\n" );
  push(ac_pc);
  if ( data.usesMemory() ) ac_pc=MEM.read(data.address());
  else ac_pc=GR.read(data.reg2());
}

//!Instruction call_m16_32 behavior method
void ac_behavior( call_m16_32 )
{
  acprintf ( " CALL M16:32 <Not implemented>\n" );
}

//!Instruction sub_rm32_imm32 behavior method
void ac_behavior( sub_rm32_imm32 )
{
  // acprintf ( "SUB RM32, IMM32\t\t" );
  uint op1, op2, aux;
  op1 = data.immediate();
  if ( data.usesMemory() ) op2 = MEM.read(data.address());
  else op2 = GR.read(data.reg2());
  aux = op2 - op1;
  if ( data.usesMemory() ) MEM.write(data.address(),aux);
  else GR.write(data.reg2(),aux);
  if ( data.usesMemory() ) acprintfmi ( "SUB", data.address(), op1 );
  else acprintfri ( "SUB", data.reg2(), op1 );
  checkFlags(OPER_SUB,op2,op1,aux,FLAG_ALL);
}

//!Instruction sub_rm32_imm8 behavior method
void ac_behavior( sub_rm32_imm8 )
{
  // acprintf ( "SUB RM32, IMM8\t\t" );
  signed char i = data.immediate8();
  signed int j = i;
  uint op1, op2, aux;
  if ( data.usesMemory() ) acprintfmi ( "SUB", data.address(), j );
  else acprintfri ( "SUB", data.reg2(), j );
  op1 = (unsigned)j;
  if ( data.usesMemory() ) op2 = MEM.read(data.address());
  else op2 = GR.read(data.reg2());
  aux = op2 - op1;
  if ( data.usesMemory() ) MEM.write(data.address(),aux);
  else GR.write(data.reg2(),aux);
  checkFlags(OPER_SUB,op2,op1,aux,FLAG_ALL);
}

//!Instruction sbb_rm32_imm32 behavior method
void ac_behavior( sbb_rm32_imm32 )
{
  acprintf ( " SBB RM32, IMM32\n" );
  uint op1, op2, aux;
  op1 = data.immediate() + testFlag(FLAG_CF);
  if ( data.usesMemory() ) op2 = MEM.read(data.address());
  else op2 = GR.read(data.reg2());
  aux = op2 - op1;
  if ( data.usesMemory() ) MEM.write(data.address(),aux);
  else GR.write(data.reg2(),aux);
  checkFlags(OPER_SUB,op2,op1,aux,FLAG_ALL);
}

//!Instruction sbb_rm32_imm8 behavior method
void ac_behavior( sbb_rm32_imm8 )
{
  acprintf ( " SBB RM32, IMM8\n" );
  signed char i = data.immediate8();
  signed int j = i;
  uint op1, op2, aux;
  op1 = (unsigned)j;
  if ( data.usesMemory() ) op2 = MEM.read(data.address());
  else op2 = GR.read(data.reg2());
  op1+= testFlag(FLAG_CF);
  aux = op2 - op1;
  if ( data.usesMemory() ) MEM.write(data.address(),aux);
  else GR.write(data.reg2(),aux);
  checkFlags(OPER_SUB,op2,op1,aux,FLAG_ALL);
}

//!Instruction adc_rm32_imm32 behavior method
void ac_behavior( adc_rm32_imm32 )
{
  acprintf ( " ADC RM32, IMM32\n" );
  uint op1, op2, aux;
  op1 = data.immediate() + testFlag(FLAG_CF);
  if ( data.usesMemory() ) op2 = MEM.read(data.address());
  else op2 = GR.read(data.reg2());
  aux = op1 + op2;
  if ( data.usesMemory() ) MEM.write(data.address(),aux);
  else GR.write(data.reg2(),aux);
  checkFlags(OPER_ADD,op1,op2,aux,FLAG_ALL);
}

	//!Instruction adc_rm32_imm8 behavior method
	void ac_behavior( adc_rm32_imm8 )
	{
		acprintf ( "ADC RM32, IMM8\n" );
		signed char i = data.immediate8();
		signed int aux = i;
		uint op1, op2, res;
		op1 = aux + testFlag(FLAG_CF);
		op2 = dataManager.getMemOrReg2();
		res = op1 + op2;
		dataManager.setMemOrReg2(res);
		checkFlags(OPER_ADD,op1,op2,aux,FLAG_ALL);
	}

//!Instruction cmp_rm32_imm32 behavior method
void ac_behavior( cmp_rm32_imm32 )
{
  acprintf ( " CMP RM32, IMM32\n" );
  signed int op1, op2, aux;
  op1 = data.immediate();
  if ( data.usesMemory() ) op2 = MEM.read(data.address());
  else op2 = GR.read(data.reg2());
  aux = op1-op2;
  checkFlags(OPER_SUB,op1,op2,aux,FLAG_ALL);
}

//!Instruction cmp_rm32_imm8 behavior method
void ac_behavior( cmp_rm32_imm8 )
{
  acprintf ( "CMP RM32, IMM8\n" );
  signed char i = data.immediate8();
  signed int op1 = dataManager.getMemOrReg2();
  signed int op2 = i;
  signed int res = op1 - op2;
  checkFlags(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction and_rm32_imm32 behavior method
void ac_behavior( and_rm32_imm32 )
{
	acprintf ( "AND RM32, IMM32\n" );
	uint op1, op2, res;
	op1 = dataManager.getImmediate();
	op2 = dataManager.getMemOrReg2();
	res = op1&op2;
	dataManager.setMemOrReg2(res);
	resetFlag(FLAG_CF);
	resetFlag(FLAG_OF);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_SF|FLAG_ZF|FLAG_PF);
}

//!Instruction and_rm32_imm8 behavior method
void ac_behavior( and_rm32_imm8 )
{
	acprintf ( "AND RM32, IMM8\n" );
	signed char i = data.immediate8();
	signed int aux = i;
	uint op1, op2, res;
	op1 = aux;
	op2 = dataManager.getMemOrReg2();
	res = op1&op2;
	dataManager.setMemOrReg2(res);
	resetFlag(FLAG_CF);
	resetFlag(FLAG_OF);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_SF|FLAG_ZF|FLAG_PF);
}

//!Instruction or_rm32_imm32 behavior method
void ac_behavior( or_rm32_imm32 )
{
	acprintf ( "OR RM32, IMM32\n" );
	uint op1, op2, res;
	op1 = dataManager.getImmediate();
	op2 = dataManager.getMemOrReg2();
	res = op1|op2;
	dataManager.setMemOrReg2(res);
	resetFlag(FLAG_CF);
	resetFlag(FLAG_OF);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_SF|FLAG_ZF|FLAG_PF);
}

//!Instruction or_rm32_imm8 behavior method
void ac_behavior( or_rm32_imm8 )
{
	acprintf ( "OR RM32, IMM8\n" );
	signed char i = data.immediate8();
	signed int aux = i;
	uint op1, op2, res;
	op1 = aux;
	op2 = dataManager.getMemOrReg2();
	res = op1|op2;
	dataManager.setMemOrReg2(res);
	resetFlag(FLAG_CF);
	resetFlag(FLAG_OF);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_SF|FLAG_ZF|FLAG_PF);
}

//!Instruction xor_rm32_imm32 behavior method
void ac_behavior( xor_rm32_imm32 )
{
	acprintf ( "XOR RM32, IMM32\n" );
	uint op1, op2, res;
	op1 = dataManager.getImmediate();
	op2 = dataManager.getMemOrReg2();
	res = op1^op2;
	dataManager.setMemOrReg2(res);
	resetFlag(FLAG_CF);
	resetFlag(FLAG_OF);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_SF|FLAG_ZF|FLAG_PF);
}

//!Instruction xor_rm32_imm8 behavior method
void ac_behavior( xor_rm32_imm8 )
{
	acprintf ( "XOR RM32, IMM8\n" );
	signed char i = data.immediate8();
	signed int aux = i;
	uint op1, op2, res;
	op1 = aux;
	op2 = dataManager.getMemOrReg2();
	res = op1^op2;
	dataManager.setMemOrReg2(res);
	resetFlag(FLAG_CF);
	resetFlag(FLAG_OF);
	checkFlags(OPER_NONE,op1,op2,res,FLAG_SF|FLAG_ZF|FLAG_PF);
}

//!Instruction div_rm32 behavior method
void ac_behavior( div_rm32 )
{
	acprintf ( "DIV RM32\n" );
	if ( OperandSize == OPERAND_SIZE16 )
	{
		unsigned int op1 = (readRegister(DX)<<16)|((readRegister(AX)&MASK_16BITS));
		unsigned short op2 = dataManager.getMemOrReg2();
		writeRegister(AX, op1/op2);
		writeRegister(DX, op1%op2);
	}
	else if ( OperandSize == OPERAND_SIZE32 )
	{
		unsigned long long edx = readRegister(EDX);
		unsigned long long op1 = (edx<<32)|(readRegister(EAX));
		unsigned int op2 = dataManager.getMemOrReg2();
		writeRegister(EAX, op1/op2);
		writeRegister(EDX, op1%op2);
	}
}

//!Instruction idiv_rm32 behavior method
void ac_behavior( idiv_rm32 )
{
	acprintf ( "IDIV RM32\n" );
	if ( OperandSize == OPERAND_SIZE16 )
	{
		signed int op1 = (readRegister(DX)<<16)|((readRegister(AX)&MASK_16BITS));
		signed short op2 = dataManager.getMemOrReg2();
		writeRegister(AX, op1/op2);
		writeRegister(DX, op1%op2);
	}
	else if ( OperandSize == OPERAND_SIZE32 )
	{
		signed long long edx = readRegister(EDX);
		signed long long op1 = (edx<<32)|(readRegister(EAX));
		signed int op2 = dataManager.getMemOrReg2();
		writeRegister(EAX, op1/op2);
		writeRegister(EDX, op1%op2);
	}
}

//!Instruction imul_rm32 behavior method
void ac_behavior( imul_rm32 )
{
	acprintf ( "IMUL RM32\n" );
	signed int op1 = readRegister(EAX);
	signed int op2 = dataManager.getMemOrReg2();
	signed long long lop1 = op1;
	signed long long lop2 = op2;
	signed long long res = lop1 * lop2;
	if ( (res>0xFFFF&&(OperandSize==OPERAND_SIZE16))||(res>0xFFFFFFFF&&(OperandSize==OPERAND_SIZE32)) )
	{
		setFlag(FLAG_CF);
		setFlag(FLAG_OF);
	}
	else
	{
		resetFlag(FLAG_CF);
		resetFlag(FLAG_OF);
	}
	uint mask = 0xFFFFFFFF;
	uint disp = 32;
	if ( OperandSize == OPERAND_SIZE16 )
	{
		mask = 0xFFFF;
		disp = 16;
	}
	writeRegister(EDX, res >> disp );
	writeRegister(EAX, res & mask);
}

//!Instruction neg_rm32 behavior method
void ac_behavior( neg_rm32 )
{
	acprintf ( " NEG RM32\n" );
	uint op, aux;
	if ( data.usesMemory() )
	{
		op = MEM.read(data.address());
		aux = (~op)+0x01;
		MEM.write(data.address(),aux);
	}
	else
	{
		op = GR.read(data.reg2());	
		aux = (~op)+0x01;
		GR.write(data.reg2(),aux);
	}
	if ( !op ) resetFlag(FLAG_CF);
	else setFlag(FLAG_CF);
	checkFlags(OPER_ADD,(~op),1,aux,FLAG_OF|FLAG_SF|FLAG_ZF|FLAG_AF|FLAG_PF);
}

//!Instruction not_rm32 behavior method
void ac_behavior( not_rm32 )
{
	acprintf ( "NOT RM32\n" );
	uint op = dataManager.getMemOrReg2();
	dataManager.setMemOrReg2(~op);
}

//!Instruction test_rm32_imm32 behavior method
void ac_behavior( test_rm32_imm32 )
{
	acprintf ( " TEST RM32, IMM32\n" );
	uint op1, op2, aux;
	if ( data.usesMemory() ) op1 = MEM.read(data.address());
	else op1 = GR.read(data.reg2());
	op2 = data.immediate();
	aux = op1 & op2;
	resetFlag(FLAG_OF);
	resetFlag(FLAG_CF);
	checkFlags(OPER_NONE,op1,op2,aux,FLAG_SF|FLAG_ZF|FLAG_PF);
}

//!Instruction rcl_rm32 behavior method
void ac_behavior( rcl_rm32 )
{
	acprintf ( " RCL RM32\n" );
        unsigned char count = 1;
        unsigned char tempCount = count & 0x1F;
        uint destination;
        bool tempCF;
        if ( data.usesMemory() ) destination = MEM.read(data.address());
        else destination = GR.read(data.reg2());
        while ( tempCount != 0 )
        {
                tempCF = (destination&(1<<31));
                destination = (destination<<1) + testFlag(FLAG_CF);
                if ( tempCF ) setFlag(FLAG_CF);
                else resetFlag(FLAG_CF);
                tempCount--;
        }
        if ( count==1 )
        {
                if ((destination&(1<<31))^(testFlag(FLAG_CF))) setFlag(FLAG_OF);
                else resetFlag(FLAG_OF);
        }
        if ( data.usesMemory() ) MEM.write(data.address(),destination);
        else GR.write(data.reg2(),destination);
}

//!Instruction rcl_rm32_imm8 behavior method
void ac_behavior( rcl_rm32_imm8 )
{
	acprintf ( " RCL RM32, IMM8\n" );
	unsigned char count = data.immediate8();
	unsigned char tempCount = count & 0x1F;
	uint destination;
	bool tempCF;
	if ( data.usesMemory() ) destination = MEM.read(data.address());
	else destination = GR.read(data.reg2());
	while ( tempCount != 0 )
	{
		tempCF = (destination&(1<<31));
		destination = (destination<<1) + testFlag(FLAG_CF);
		if ( tempCF ) setFlag(FLAG_CF);
		else resetFlag(FLAG_CF);
		tempCount--;
	}
	if ( count==1 )
	{
		if ((destination&(1<<31))^(testFlag(FLAG_CF))) setFlag(FLAG_OF);
		else resetFlag(FLAG_OF);
	}
	if ( data.usesMemory() ) MEM.write(data.address(),destination);
	else GR.write(data.reg2(),destination);
}

//!Instruction rcr_rm32 behavior method
void ac_behavior( rcr_rm32 )
{
	acprintf ( " RCR RM32\n" );
        unsigned char count = 1;
        unsigned char tempCount = count & 0x1F;
        uint destination;
        bool tempCF;
        if ( data.usesMemory() ) destination = MEM.read(data.address());
        else destination = GR.read(data.reg2());
        if ( count == 1 )
        {
                if ((destination&(1<<31))^(testFlag(FLAG_CF))) setFlag(FLAG_OF);
                else resetFlag(FLAG_OF);
        }
        while ( tempCount != 0 )
        {
                tempCF = (destination&0x01);
                destination = (destination>>1)+(testFlag(FLAG_CF)*(1<<31));
                if ( tempCF ) setFlag(FLAG_CF);
                else resetFlag(FLAG_CF);
                tempCount--;
        }
        if ( data.usesMemory() ) MEM.write(data.address(),destination);
        else GR.write(data.reg2(),destination);
}

//!Instruction rcr_rm32_imm8 behavior method
void ac_behavior( rcr_rm32_imm8 )
{
	acprintf ( " RCR RM32, IMM8\n" );
	unsigned char count = data.immediate8();
	unsigned char tempCount = count & 0x1F;
	uint destination;
	bool tempCF;
	if ( data.usesMemory() ) destination = MEM.read(data.address());
	else destination = GR.read(data.reg2());
	if ( count == 1 )
	{
		if ((destination&(1<<31))^(testFlag(FLAG_CF))) setFlag(FLAG_OF);
		else resetFlag(FLAG_OF);
	}
	while ( tempCount != 0 )
	{
		tempCF = (destination&0x01);
		destination = (destination>>1)+(testFlag(FLAG_CF)*(1<<31));
		if ( tempCF ) setFlag(FLAG_CF);
		else resetFlag(FLAG_CF);
		tempCount--;
	}
	if ( data.usesMemory() ) MEM.write(data.address(),destination);
	else GR.write(data.reg2(),destination);
}

//!Instruction rol_rm32 behavior method
void ac_behavior( rol_rm32 )
{
	acprintf ( " ROL RM32\n" );
        unsigned char count = 1;
        unsigned char tempCount = count % 32;
        bool tempCF;
        uint destination;
        if ( data.usesMemory() ) destination = MEM.read(data.address());
        else destination = GR.read(data.reg2());
        while ( tempCount != 0 )
        {
                tempCF = (destination & (1<<31));
                destination = (destination<<1) + tempCF;
                tempCount--;
        }
        if ( destination & 0x01 ) setFlag(FLAG_CF);
        else resetFlag(FLAG_CF);
	if ( data.usesMemory() ) MEM.write(data.address(),destination);
	else GR.write(data.reg2(),destination);
        if ( count == 1 )
        {
                if ((destination&(1<<31))^(testFlag(FLAG_CF))) setFlag(FLAG_OF);
                else resetFlag(FLAG_OF);
        }
}

//!Instruction rol_rm32_imm8 behavior method
void ac_behavior( rol_rm32_imm8 )
{
	acprintf ( " ROL RM32, IMM8\n" );
	unsigned char count = data.immediate8();
	unsigned char tempCount = count % 32;
	bool tempCF;
	uint destination;
	if ( data.usesMemory() ) destination = MEM.read(data.address());
	else destination = GR.read(data.reg2());
	while ( tempCount != 0 )
	{
		tempCF = (destination & (1<<31));
		destination = (destination<<1) + tempCF;
		tempCount--;
	}
	if ( destination & 0x01 ) setFlag(FLAG_CF);
	else resetFlag(FLAG_CF);
	if ( data.usesMemory() ) MEM.write(data.address(),destination);
	else GR.write(data.reg2(),destination);
	if ( count == 1 )
	{
		if ((destination&(1<<31))^(testFlag(FLAG_CF))) setFlag(FLAG_OF);
		else resetFlag(FLAG_OF);
	}
}

//!Instruction ror_rm32 behavior method
void ac_behavior( ror_rm32 )
{
	acprintf ( " ROR RM32\n" );
        unsigned char count = 1;
        unsigned char tempCount = count % 32;
        bool tempCF;
        uint destination;
        if ( data.usesMemory() ) destination = MEM.read(data.address());
        else destination = GR.read(data.reg2());
        while ( tempCount != 0 )
        {
                tempCF = destination & (0x01);
                destination = (destination>>1)+(tempCF*(1<<31));
                tempCount--;
        }
        if ( data.usesMemory() ) MEM.write(data.address(),destination);
        else GR.write(data.reg2(),destination);
        if ( destination&(1<<31) ) setFlag(FLAG_CF);
        else resetFlag(FLAG_CF);
        if ( count == 1 )
        {
                if ((destination&(1<<31))^(destination&(1<<30))) setFlag(FLAG_OF);
                else resetFlag(FLAG_OF);
        }
}

//!Instruction ror_rm32_imm8 behavior method
void ac_behavior( ror_rm32_imm8 )
{
	acprintf ( " ROR RM32, IMM8\n" );
	unsigned char count = (data.immediate8()&0x1F);
	unsigned char tempCount = count % 32;
	bool tempCF;
	uint destination;
	if ( data.usesMemory() ) destination = MEM.read(data.address());
	else destination = GR.read(data.reg2());
	while ( tempCount != 0 )
	{
		tempCF = destination & (0x01);
		destination = (destination>>1)+(tempCF*(1<<31));
		tempCount--;
	}
	if ( data.usesMemory() ) MEM.write(data.address(),destination);
	else GR.write(data.reg2(),destination);
	if ( destination&(1<<31) ) setFlag(FLAG_CF);
	else resetFlag(FLAG_CF);
	if ( count == 1 )
	{
		if ((destination&(1<<31))^(destination&(1<<30))) setFlag(FLAG_OF);
		else resetFlag(FLAG_OF);
	}
}

//!Instruction P_LOCK behavior method
void ac_behavior( P_LOCK )
{
	acprintf ( " LOCK prefix <Not implemented>\n" );
}

//!Instruction P_REPNE_REPNZ behavior method
void ac_behavior( P_REPNE_REPNZ )
{
	acprintf ( " REPNE/REPNZ prefix <Not implemented>\n" );
}

//!Instruction P_REP_REPE_REPZ behavior method
void ac_behavior( P_REP_REPE_REPZ )
{
	acprintf ( " REP/REPE/REPZ prefix <Not implemented>\n" );
}

//!Instruction P_CS behavior method
void ac_behavior( P_CS )
{
	acprintf ( " CS override prefix <Not implemented>\n" );
}

//!Instruction P_SS behavior method
void ac_behavior( P_SS )
{
        acprintf ( " SS override prefix <Not implemented>\n" );
}

//!Instruction P_DS behavior method
void ac_behavior( P_DS )
{
        acprintf ( " DS override prefix <Not implemented>\n" );
}

//!Instruction P_ES behavior method
void ac_behavior( P_ES )
{
        acprintf ( " ES override prefix <Not implemented>\n" );
}

//!Instruction P_FS behavior method
void ac_behavior( P_FS )
{
        acprintf ( " FS override prefix <Not implemented>\n" );
}

//!Instruction P_GS behavior method
void ac_behavior( P_GS )
{
        acprintf ( " GS override prefix <Not implemented>\n" );
}

//!Instruction P_BTAKEN behavior method
void ac_behavior( P_BTAKEN )
{
	acprintf ( " BRANCH TAKEN prefix <Not implemented>\n" );
}

//!Instruction P_BNTAKEN behavior method
void ac_behavior( P_BNTAKEN )
{
	acprintf ( " BRANCH NOT TAKEN prefix <Not implemented>\n" );
}

//!Instruction P_OPSIZE behavior method
void ac_behavior( P_OPSIZE )
{
  acprintf ( "OPERAND SIZE is 16Bits for next instruction\n" );
  OperandSize = OPERAND_SIZE16JUST;
  //  OperandSize = OPERAND_SIZE32;
}

//!Instruction P_ADSIZE behavior method
void ac_behavior( P_ADSIZE )
{
  acprintf ( " ADDRESS SIZE if 16Bits for next instruction\n" );
  AddressSize = ADDRESS_SIZE16JUST;
}

//!Instruction movb_rm32_imm8 behavior method
void ac_behavior( mov_rm8_imm8 )
{
  acprintf ( "CALLED MOV RM8, IMM8\n" );
  if ( data.usesMemory() ) MEM.write_byte(data.address(),data.immediate8());
  else GR.write(data.reg2(),data.immediate8());
}

//!Instruction mov_r8_rm8 behavior method
void ac_behavior( mov_r8_rm8 )
{
  acprintf ( "CALLED MOV R8, RM8\n" );
  unsigned char op;
  if ( data.usesMemory() ) op = MEM.read_byte(data.address());
  else op = readRegister8(data.reg2());
  writeRegister8(data.reg1(),op);
}

//!Instruction mov_rm8_r8 behavior method
void ac_behavior( mov_rm8_r8 )
{
  acprintf ( "CALLED MOV RM8, R8\n" );
  unsigned char op = readRegister8(data.reg1());
  if ( data.usesMemory() ) MEM.write_byte(data.address(),op);
  else writeRegister8(data.reg2(),op);
}

//!Instruction mov_AL_imm8 behavior method
void ac_behavior( mov_AL_imm8 )
{
  acprintf ( "CALLED MOV AL, IMM8\n" );
 writeRegister8(AL,data.immediate8());
}

//!Instruction mov_CL_imm8 behavior method
void ac_behavior( mov_CL_imm8 )
{
  acprintf ( "CALLED MOV CL, IMM8\n" );
  writeRegister8(CL,data.immediate8());
}

//!Instruction mov_DL_imm8 behavior method
void ac_behavior( mov_DL_imm8 )
{
  acprintf ( "CALLED MOV DL, IMM8\n" );
  writeRegister8(DL,data.immediate8());
}

//!Instruction mov_BL_imm8 behavior method
void ac_behavior( mov_BL_imm8 )
{
  acprintf ( "CALLED MOV BL, IMM8\n" );
  writeRegister8(BL,data.immediate8());
}

//!Instruction mov_AH_imm8 behavior method
void ac_behavior( mov_AH_imm8 )
{
  acprintf ( "CALLED MOV AH, IMM8\n" );
   writeRegister8(AH,data.immediate8());
}

//!Instruction mov_CH_imm8 behavior method
void ac_behavior( mov_CH_imm8 )
{
  acprintf ( "CALLED MOV CH, IMM8\n" );
  writeRegister8(CH,data.immediate8());
}

//!Instruction mov_DH_imm8 behavior method
void ac_behavior( mov_DH_imm8 )
{
  acprintf ( "CALLED MOV DH, IMM8\n" );
  writeRegister8(DH,data.immediate8());
}

//!Instruction mov_BH_imm8 behavior method
void ac_behavior( mov_BH_imm8 )
{
	acprintf ( "CALLED MOV BH, IMM8\n" );
	writeRegister8(BH,data.immediate8());
}

//!Instruction add_r8_rm8 behavior method
void ac_behavior( add_r8_rm8 )
{
	acprintf ( "CALLED ADD R8, RM8\n" );
	unsigned char op1;
	if ( data.usesMemory() ) op1 = MEM.read_byte(data.address());
	else op1 = readRegister8(data.reg2());
	unsigned char op2 = readRegister8(data.reg1());
	unsigned char res = op1 + op2;
	writeRegister8(data.reg1(),res);
	checkFlags8(OPER_ADD,op1,op2,res,FLAG_ALL);
}

//!Instruction add_rm8_r8 behavior method
void ac_behavior( add_rm8_r8 )
{
	acprintf ( "CALLED ADD RM8, R8\n" );
	unsigned char op1;
	if ( data.usesMemory() ) op1 = MEM.read_byte(data.address());
	else op1 = readRegister8(data.reg2());
	unsigned char op2 = readRegister8(data.reg1());
	unsigned char res = op1 + op2;
	if ( data.usesMemory() ) MEM.write_byte(data.address(), res );
	else writeRegister8(data.reg2(),res);
	checkFlags8(OPER_ADD,op1,op2,res,FLAG_ALL);
}

//!Instruction add_rm8_imm8 behavior method
void ac_behavior( add_rm8_imm8 )
{
	acprintf ( "CALLED ADD RM8, IMM8\n" );
	unsigned char op1;
	if ( data.usesMemory() ) op1 = MEM.read_byte(data.address());
	else op1 = readRegister8(data.reg2());
	unsigned char op2 = data.immediate8();
	unsigned char res = op1 + op2;
	if ( data.usesMemory() ) MEM.write_byte(data.address(), res);
        else writeRegister8(data.reg2(), res);
	checkFlags8(OPER_ADD,op1,op2,res,FLAG_ALL);
}

//!Instruction sub_rm8_r8 behavior method
void ac_behavior( sub_rm8_r8 )
{
	acprintf ( "CALLED SUB RM8, R8\n" );
	unsigned char op1, op2, res;
	if ( data.usesMemory() ) op1 = MEM.read_byte(data.address());
	else op1 = readRegister8(data.reg2());
	op2 = readRegister8(data.reg1());
	res = op1 - op2;
	if ( data.usesMemory() ) MEM.write_byte(data.address(),res);
	else writeRegister8(data.reg2(),res);
	checkFlags8(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction sub_rm8_imm8 behavior method
void ac_behavior( sub_rm8_imm8 )
{
	acprintf ( "CALLED SUB RM8, IMM8\n" );
	unsigned char op1;
	if ( data.usesMemory() ) op1 = MEM.read_byte(data.address());
	else op1 = readRegister8(data.reg2());
	unsigned char op2 = data.immediate8();
	unsigned char res = op1 - op2;
	if ( data.usesMemory() ) MEM.write_byte(data.address(), res);
	else writeRegister8(data.reg2(), res);
	checkFlags8(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction inc_rm8 behavior method
void ac_behavior( inc_rm8 )
{
  acprintf ( "CALLED INC RM8\n" );
  unsigned char op1;
  if ( data.usesMemory() ) op1 = MEM.read_byte(data.address());
  else op1 = readRegister8(data.reg2());
  unsigned char op2 = 1;
  unsigned char res = op1 + op2;
  if ( data.usesMemory() ) MEM.write_byte(data.address(),res);
  else writeRegister8(data.reg2(),res);
  checkFlags8(OPER_ADD,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

//!Instruction dec_rm8 behavior method
void ac_behavior( dec_rm8 )
{
	acprintf ( "CALLED DEC RM8\n" );
	unsigned char op1;
	if ( data.usesMemory() ) op1 = MEM.read_byte(data.address());
	else op1 = readRegister8(data.reg2());
	unsigned char op2 = 1;
	unsigned char res = op1 - op2;
	if ( data.usesMemory() ) MEM.write_byte(data.address(), res);
	else writeRegister8(data.reg2(), res);
	checkFlags8(OPER_SUB,op1,op2,res,FLAG_ALL&(~FLAG_CF));
}

//!Instruction imul_rm8 behavior method
void ac_behavior( imul_rm8 )
{
	printf ( "IMUL RM8\n" );
	signed char op1 = readRegister8(AL);
	signed char op2;
	signed short res;
	if ( data.usesMemory() ) op2 = MEM.read_byte(data.address());
	else op2 = readRegister8(data.reg2());
	res = op1 * op2;
	if ( res & 0xFF00 )
	{
		setFlag(FLAG_CF);
		setFlag(FLAG_OF);
	}
	else
	{
		resetFlag(FLAG_CF);
		resetFlag(FLAG_OF);
	}
	OperandSize = OPERAND_SIZE16;
	writeRegister(AX, res);
}

//!Instruction imul_r32_rm32 behavior method
void ac_behavior( imul_r32_rm32 )
{
	printf ( "IMUL R32, RM32\n" );
	signed int op1 = dataManager.getReg1();
	signed int op2 = dataManager.getMemOrReg2();
	signed long long res = op1 * op2;
	if ( res > 0xFFFFFFFF )
	{
		setFlag(FLAG_CF);
		setFlag(FLAG_OF);
	}
	else
	{
		resetFlag(FLAG_CF);
		resetFlag(FLAG_OF);
	}
	dataManager.setReg1(res);
}

//!Instruction mul_rm8 behavior method
void ac_behavior( mul_rm8 )
{
	printf ( "MUL RM8\n" );
	unsigned char op1 = readRegister8(AL);
	unsigned char op2;
	unsigned short res;
	if ( data.usesMemory() ) op2 = MEM.read_byte(data.address());
	else op2 = readRegister8(data.reg2());
	res = op1 * op2;
	if ( res & 0xFF00 )
	{
		setFlag(FLAG_CF);
		setFlag(FLAG_OF);
	}
	else
	{
		resetFlag(FLAG_CF);
		resetFlag(FLAG_OF);
	}
	OperandSize = OPERAND_SIZE16;
	writeRegister(AX, res);
}

//!Instruction sal_rm8 behavior method
void ac_behavior( sal_rm8 )
{
	printf ( "SAL RM8\n" );
	unsigned char op, res;
	if ( data.usesMemory() ) op = MEM.read_byte(data.address());
	else op = readRegister8(data.reg2());
	res = op << 1;
	if ( data.usesMemory() ) MEM.write_byte(data.address(),res);
	else writeRegister8(data.reg2(),res);
	checkFlags8(OPER_NONE,0,0,res,FLAG_ALL);
	writeFlag(FLAG_CF, op&0x80);
	bool ofcond = (!MSB8(res)&&testFlag(FLAG_CF))||(MSB8(res)&&!testFlag(FLAG_CF));
	writeFlag(FLAG_OF, ofcond);
}

//!Instruction sal_rm8_imm8 behavior method
void ac_behavior( sal_rm8_imm8 )
{
	printf ( "SAL RM8, IMM8\n" );
	unsigned char op, res;
	signed char count = data.immediate8();
	if ( data.usesMemory() ) op = MEM.read_byte(data.address());
	else op = readRegister8(data.reg2());
	res = op << count;
	if ( data.usesMemory() ) MEM.write_byte(data.address(),res);
	else writeRegister(data.reg2(),res);
	checkFlags8(OPER_NONE,0,0,res,FLAG_ALL);
	if ( count > 0 ) writeFlag(FLAG_CF, (op<<(count-1))&0x80);
	bool ofcond = (!MSB8(res)&&testFlag(FLAG_CF))||(MSB8(res)&&!testFlag(FLAG_CF));
	if ( count == 1 ) writeFlag(FLAG_OF, ofcond);
}

//!Instruction shr_rm8 behavior method
void ac_behavior( shr_rm8 )
{
	printf ( "SHR RM8\n" );
	unsigned char op, res;
	if ( data.usesMemory() ) op = MEM.read_byte(data.address());
	else op = readRegister8(data.reg2());
	res = op >> 1;
	if ( data.usesMemory() ) MEM.write_byte(data.address(),res);
	else writeRegister8(data.reg2(),res);
	checkFlags8(OPER_NONE,0,0,res,FLAG_ALL);
	writeFlag(FLAG_CF, op&0x01);
	writeFlag(FLAG_OF, op&0x80);
}

//!Instruction sar_rm8 behavior method
void ac_behavior( sar_rm8 )
{
	printf ( "SAR RM8\n" );
	signed char op, res;
	if ( data.usesMemory() ) op = MEM.read_byte(data.address());
	else op = readRegister8(data.reg2());
	res = op>>1;
	if ( data.usesMemory() ) MEM.write_byte(data.address(),res);
	else writeRegister8(data.reg2(),res);
	checkFlags8(OPER_NONE,0,0,res,FLAG_ALL);
	writeFlag(FLAG_CF, op&0x01);
	resetFlag(FLAG_OF);
}

//!Instruction sar_rm8_imm8 behavior method
void ac_behavior( sar_rm8_imm8 )
{
	printf ( "SAR RM8, IMM8\n" );
	signed char count;
	signed char op, res;
	if ( data.usesMemory() ) op = MEM.read_byte(data.address());
	else op = readRegister8(data.reg2());
	count = data.immediate8();
	res = op >> count;
	if ( data.usesMemory() ) MEM.write_byte(data.address(),res);
	else writeRegister8(data.reg2(),res);
	checkFlags8(OPER_NONE,0,0,res,FLAG_ALL);
	if ( count > 0 ) writeFlag(FLAG_CF, (op>>(count-1))&0x01);
	if ( count == 1 ) resetFlag(FLAG_OF);
}

//!Instruction cmp_rm8_imm8 behavior method
void ac_behavior( cmp_rm8_imm8 )
{
	printf ( "CMP RM8, IMM8\n" );
	unsigned char op1, res;
	signed char op2;
	if ( data.usesMemory() ) op1 = MEM.read_byte(data.address());
	else op1 = readRegister8(data.reg2());
	op2 = data.immediate8();
	res = op1 - op2;
	checkFlags8(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction cmp_AL_imm8 behavior method
void ac_behavior( cmp_AL_imm8 )
{
	printf ( "CMP AL, IMM8\n" );
	unsigned char op1, res;
	signed char op2;
	op1 = readRegister8(AL);
	op2 = data.immediate8();
	res = op1 - op2;
	checkFlags8(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction shr_rm8_imm8 behavior method
void ac_behavior( shr_rm8_imm8 )
{
	printf ( "SHR RM8, IMM8\n" );
	signed char count;
	unsigned char op, res;
	if ( data.usesMemory() ) op = MEM.read_byte(data.address());
	else op = readRegister8(data.reg2());
	count = data.immediate8();
	res = op >> count;
	if ( data.usesMemory() ) MEM.write_byte(data.address(),res);
	else writeRegister8(data.reg2(),res);
	checkFlags8(OPER_NONE,0,0,res,FLAG_ALL);
	if ( count == 1 ) writeFlag(FLAG_OF, op&0x80);
	if ( count > 0 ) writeFlag(FLAG_CF, (op>>(count-1))&0x01);
}

//!Instruction div_rm8 behavior method
void ac_behavior( div_rm8 )
{
	printf ( "DIV RM8\n" );
	OperandSize = OPERAND_SIZE16;
	unsigned short op1 = readRegister(AX);
	OperandSize = OPERAND_SIZE32;
	unsigned char op2;
	if ( data.usesMemory() ) op2 = MEM.read_byte(data.address());
	else op2 = readRegister8(data.reg2());
	writeRegister8(AH, op1/op2);
	writeRegister8(AL, op1%op2);
}

//!Instruction or_r8_rm8 behavior method
void ac_behavior( or_r8_rm8 )
{
	printf ( "OR R8, RM8\n" );
	unsigned char op1 = readRegister8(data.reg1());
	unsigned char op2, res;
	if ( data.usesMemory() ) op2 = MEM.read_byte(data.address());
	else op2 = readRegister8(data.reg2());
	res = op1|op2;
	writeRegister8(data.reg1(),res);
	checkFlags8(OPER_NONE,op1,op2,res,FLAG_ALL);
	resetFlag(FLAG_OF);
	resetFlag(FLAG_CF);
}

//!Instruction and_r8_rm8 behavior method
void ac_behavior( and_r8_rm8 )
{
	printf ( "AND R8, RM8\n" );
	unsigned char op1 = readRegister8(data.reg1());
	unsigned char op2, res;
	if ( data.usesMemory() ) op2 = MEM.read_byte(data.address());
	else op2 = readRegister8(data.reg2());
	res = op1&op2;
	writeRegister8(data.reg1(),res);
	checkFlags8(OPER_NONE,op1,op2,res,FLAG_ALL);
	resetFlag(FLAG_OF);
	resetFlag(FLAG_CF);
}

//!Instruction xor_r8_rm8 behavior method
void ac_behavior( xor_r8_rm8 )
{
	printf ( "XOR R8, RM8\n" );
	unsigned char op1 = readRegister8(data.reg1());
	unsigned char op2, res;
	if ( data.usesMemory() ) op2 = MEM.read_byte(data.address());
	else op2 = readRegister8(data.reg2());
	res = op1^op2;
	writeRegister8(data.reg1(),res);
	checkFlags8(OPER_NONE,op1,op2,res,FLAG_ALL);
	resetFlag(FLAG_OF);
	resetFlag(FLAG_CF);
}

//!Instruction cmp_r8_rm8 behavior method.
void ac_behavior( cmp_r8_rm8 )
{
	printf ( "CMP R8, RM8\n" );
	unsigned char op1, op2, res;
	op1 = readRegister8(data.reg1());
	if ( data.usesMemory() ) op2 = MEM.read_byte(data.address());
	else op2 = readRegister8(data.reg2());
	res = op1 - op2;
	checkFlags8(OPER_SUB,op1,op2,res,FLAG_ALL);
}

//!Instruction push_imm8 behavior method.
void ac_behavior( push_imm8 )
{
	printf ( "PUSH IMM8\n" );
	signed char i = data.immediate8();
	signed int aux = i;
	push(aux);
}

//!Instruction dump_registers behavior method.
void ac_behavior( dump_registers )
{
	if ( VERBOSE ) {
	printf ( "------------------ Register dumping begin --------------------\n" );
	printf ( "- General Purpose --------------------------------------------\n" );
	printf ( "EAX = %08X\tECX = %08X\t", GR.read(EAX), GR.read(ECX) );
	printf ( "EDX = %08X\tEBX = %08X\n", GR.read(EDX), GR.read(EBX) );
	printf ( "ESP = %08X\tEBP = %08X\t", GR.read(ESP), GR.read(EBP) );
	printf ( "ESI = %08X\tEDI = %08X\n", GR.read(ESI), GR.read(EDI) );
	printf ( "- Segment ----------------------------------------------------\n" );
	printf ( "CS = %04X\tDS = %04X\tSS = %04X\n", SR.read(CS), SR.read(DS), SR.read(SS) );
	printf ( "ES = %04X\tFS = %04X\tGS = %04X\n", SR.read(ES), SR.read(FS), SR.read(GS) );
	printf ( "- Flags ------------------------------------------------------\n" );
	printf ( "EFLAGS = %08X\n", SPR.read(EFLAGS) );
	printf ( "OF FLAG = %d\n", testFlag(FLAG_OF) );
	printf ( "SF FLAG = %d\n", testFlag(FLAG_SF) );
	printf ( "ZF FLAG = %d\n", testFlag(FLAG_ZF) );
	printf ( "AF FLAG = %d\n", testFlag(FLAG_AF) );
	printf ( "CF FLAG = %d\n", testFlag(FLAG_CF) );
	printf ( "PF FLAG = %d\n", testFlag(FLAG_PF) );
	printf ( "- Program counter --------------------------------------------\n" );
	printf ( "EIP = %08X\n", SPR.read(EIP) );
	printf ( "------------------ Register dumping end ----------------------\n\n" );
	}
}

//!Instruction dump_memory behavior method.
void ac_behavior( dump_memory )
{
  if ( VERBOSE ) {
    printf ( "------------------ Memory dump begin --------------------\n" );
    printf ( "From : %08X\tTo : %08X\n", iadd, eadd );
    printf ( "- Data --------------------------------------------------\n" );
    unsigned int numBytes = (eadd-iadd);
    unsigned int counter = 0;
    unsigned char d;
    if ( eadd < iadd ) numBytes = 0;
    while ( counter <= numBytes )
      {
	if ( !(counter%8) ) printf ( "%08X\t", iadd+counter );
	d = MEM.read(iadd+counter);
	printf ( "%02X", d );
	counter++;
	if ( !(counter%8) ) printf ( "\n" );
	else printf ( " " );
      }
    if ( counter%8 ) printf ( "\n" );
    printf ( "------------------ Memory dump end ----------------------\n\n" );
  }
}

//!Instruction dumpt_stack behavior method.
void ac_behavior( dump_stack )
{
	if ( VERBOSE ) {
	printf ( "------------------ Stack dump begin ---------------------\n" );
	printf ( "Top : %08X\tBottom : %08X\n", GR.read(ESP), STACK_BOTTOM );
	printf ( "- Data --------------------------------------------------\n" );
	for ( unsigned int i=0; i<(STACK_BOTTOM-GR.read(ESP)); i+=4 )
		printf ( "%08X\t %08X\n", GR.read(ESP)+i, MEM.read(GR.read(ESP)+i) );
	printf ( "------------------ Stack dump end -----------------------\n\n" );
	}
}

//!Instruction sahf behavior method.
//void ac_behavior( sahf )
//{
//printManager.print ( "SAHF <Not implemented>" );
//}


//!Instruction aaa description

/* *********************************************               
Description:                                                                                                                                
Adjusts the sum of two unpacked BCD values to create an unpacked BCD result. The AL                                                          
register is the implied source and destination operand for this instruction. The AAA instruction                                         
is only useful when it follows an ADD instruction that adds (binary addition) two unpacked                                                  
BCD values and stores a byte result in the AL register. The AAA instruction then adjusts the                                                 
contents of the AL register to contain the correct 1-digit unpacked BCD result.                                                              
If the addition produces a decimal carry, the AH register increments by 1, and the CF and AF                                                 
flags are set. If there was no decimal carry, the CF and AF flags are cleared and the AH register                                            
is unchanged. In either case, bits 4 through 7 of the AL register are set to 0.                                                              
********************************************** */



//!Instruction aaa behavior method.
void ac_behavior( aaa ){
  signed int aux;
  acprintf( "AAA - not tested\n" );
  
  aux = GR[AL];
  if ( ( (aux & 0x0F) > 9 ) or ( testFlag(FLAG_AF) ) ){ 
    aux+=6;
    GR[AL] = aux;
    //    aux = GR[AH];
    aux = readRegister8(AH);
    aux+=1;
    writeRegister8(AH,aux); 
    writeFlag( FLAG_AF, 1);
    writeFlag( FLAG_CF, 1);
    aux = GR[AL];
    aux = ( aux & 0x0F );
    GR[AL] = aux;
  }//if
  else{
    resetFlag(FLAG_AF);
    resetFlag(FLAG_CF);
    aux = GR[AL];
    aux = ( aux & 0x0F );
    GR[AL] = aux;
  }//else
}//ac_behavior(aaa)


//!Instruction aad description
/**************************************************                                            
Description:                                                                                       
Adjusts two unpacked BCD digits (the least-significant digit in the AL register and the mostsignificant                                  
digit in the AH register) so that a division operation performed on the result will yield                                                
a correct unpacked BCD value. The AAD instruction is only useful when it precedes a DIV                                                  
instruction that divides (binary division) the adjusted value in the AX register by an unpacked                                          
BCD value.                                                                                                                               
The AAD instruction sets the value in the AL register to (AL + (10 * AH)), and then clears the                                           
AH register to 00H. The value in the AX register is then equal to the binary equivalent of the                   
original unpacked two-digit (base 10) number in registers AH and AL.                                                                  
****************************************************/
//!Instruction aad behavior method.
void ac_behavior( aad ){
  signed int tempAL,tempAH;
  acprintf ( " AAD - not tested\n" );
  tempAL = GR[AL];
  //tempAH = GR[AH];
  tempAH = readRegister8(AH);
  GR[AL] =  (( tempAL + ( tempAH*0x0A)  )  & 0xFF);
  writeRegister8(AH,0);
  //GR[AH] = 0;
  setFlags_af( FLAG_SF, GR[AL] );
  setFlags_af( FLAG_ZF, GR[AL] );
  setFlags_af( FLAG_PF, GR[AL] );
}//ac_behavior( aad )


//!Instruction aam description
/* ************************************     
Description:
Adjusts the result of the multiplication of two unpacked BCD values to create a pair of unpacked
(base 10) BCD values. The AX register is the implied source and destination operand for this
instruction. The AAM instruction is only useful when it follows an MUL instruction that multiplies
(binary multiplication) two unpacked BCD values and stores a word result in the AX
register. The AAM instruction then adjusts the contents of the AX register to contain the correct
2-digit unpacked (base 10) BCD result.
************************************ */
//!Instruction aam behavior method.
void ac_behavior( aam ){
  uint tempAL;
  acprintf ( " AAM - not tested\n" );
  tempAL = GR[AL];
  //GR[AH] = ( tempAL / 0x0A );
  writeRegister8(AH, ( tempAL / 0x0A));
  GR[AL] = ( tempAL % 0x0A );
  setFlags_af( FLAG_SF,GR[AL] );
  setFlags_af( FLAG_ZF,GR[AL] );
  setFlags_af( FLAG_PF,GR[AL] );
} //ac_behavior( aam )

//!Instruction aas description
/* *****************************************************    
Adjusts the result of the subtraction of two unpacked BCD values to create a unpacked BCD                                                
result. The AL register is the implied source and destination operand for this instruction. The                                          
AAS instruction is only useful when it follows a SUB instruction that subtracts (binary subtraction)                                     
one unpacked BCD value from another and stores a byte result in the AL register. The AAA                                                 
instruction then adjusts the contents of the AL register to contain the correct 1-digit unpacked                                         
BCD result.                                                                                                                              
If the subtraction produced a decimal carry, the AH register decrements by 1, and the CF and                                             
AF flags are set. If no decimal carry occurred, the CF and AF flags are cleared, and the AH                                       
register is unchanged. In either case, the AL register is left with its top nibble set to 0.                                             
******************************************************* */
//!Instruction aas behavior method.
void ac_behavior( aas ){
  uint aux;
  acprintf ( " AAS <Not tested>\n" );
  if ( ((GR[AL] & 0x0F) > 9) || ( testFlag( FLAG_AF )) ){
    aux = GR[AL];
    aux = aux - 6;
    aux = (aux & 0x0F);
    GR[AL] = aux;
    aux = readRegister8(AH);
    //aux = GR[AH];
    aux = aux - 1;
    //    GR[AH] = aux;
    writeRegister8(AH,aux);
    setFlag( FLAG_AF );
    setFlag( FLAG_CF );
  }//if
  else{
    resetFlag( FLAG_CF );
    resetFlag( FLAG_AF );
    aux = GR[AL];
    aux = ( aux & 0x0F );
  }//else

} //ac_behavior( aas )


//!Instruction cbw_cwde  description
/****************************************
Description
Double the size of the source operand by means of sign extension. The CBW (convert byte to
word) instruction copies the sign (bit 7) in the source operand into every bit in the AH register.
The CWDE (convert word to doubleword) instruction copies the sign (bit 15) of the word in the
AX register into the high 16 bits of the EAX register.
CBW and CWDE reference the same opcode. The CBW instruction is intended for use when the
operand-size attribute is 16; CWDE is intended for use when the operand-size attribute is 32.
Some assemblers may force the operand size. Others may treat these two mnemonics as
synonyms (CBW/CWDE) and use the setting of the operand-size attribute to determine the size
of values to be converted.
*****************************************/

//!Instruction cbw_cwde behavior method.
void ac_behavior( cbw_cwde )
{
  signed int tempGR;
  if ( OperandSize == OPERAND_SIZE16 ){ //Instruction = CBW
    acprintf ( " CBW <Not Tested>\n" );
    tempGR = GR[AL];
    if (MSB8(tempGR))//negativo
      GR[AX] = 0xFF00 | (tempGR & MASK_8BITS);
    else
      GR[AX] = 0x0000 | (tempGR & MASK_8BITS);
  }//if
  else if ( OperandSize == OPERAND_SIZE32 ){//Instruction = CWDE
    acprintf ( "CWDE <Not Tested>\n" );
    tempGR = GR[AX];
    if (MSB16(tempGR))//negativo
      GR[EAX] = 0xFFFF0000 | (tempGR & MASK_16BITS);
    else
      GR[EAX] = 0x00000000 | (tempGR & MASK_16BITS);

  }//else 
}//ac_behavior (cbw_cwde)

//!Instruction daa  description
/*********************************************
Description
Adjusts the sum of two packed BCD values to create a packed BCD result. The AL register is
the implied source and destination operand. The DAA instruction is only useful when it follows
an ADD instruction that adds (binary addition) two 2-digit, packed BCD values and stores a byte
result in the AL register. The DAA instruction then adjusts the contents of the AL register to
contain the correct 2-digit, packed BCD result. If a decimal carry is detected, the CF and AF
flags are set accordingly.
*********************************************/
//!Instruction daa behavior method.
void ac_behavior( daa ){
  acprintf ( " DAA <Not Tested>\n" );
  uint tempAL,old_AL,old_FLAG_CF;
  uint op1, res; //para checar Flags
  old_AL = GR[AL];
  old_FLAG_CF = SPR[FLAG_CF];
 
  if( ((GR[AL] & 0x0F) > 9) || (testFlag(FLAG_AF)) ){
    tempAL = GR[AL];
    op1 = tempAL;
    tempAL += 6;
    res = tempAL;
    GR[AL] = tempAL;  
    writeFlag(FLAG_CF,( old_FLAG_CF ||( ( (unsigned int)tempAL) > 0xFF    )));
    setFlag(FLAG_AF);

    checkFlags8(OPER_ADD, op1,6,res,FLAG_SF );
    checkFlags8(OPER_ADD, op1,6,res,FLAG_ZF );
    checkFlags8(OPER_ADD, op1,6,res,FLAG_PF );

  }//if
  else
    resetFlag(FLAG_AF);

  if ((old_AL > 0x99) || (old_FLAG_CF == 1)){
    tempAL = GR[AL];
    op1 = tempAL;
    tempAL += 0x60;
    res = tempAL;
    GR[AL] = tempAL;
    setFlag(FLAG_CF);

    checkFlags8(OPER_ADD, op1,0x60,res,FLAG_SF );
    checkFlags8(OPER_ADD, op1,0x60,res,FLAG_ZF );
    checkFlags8(OPER_ADD, op1,0x60,res,FLAG_PF );
  }//if
  else
    resetFlag(FLAG_CF);
  
  
}//ac_behavior( daa )





/**************************************
//!Instruction das description
Description
Adjusts the result of the subtraction of two packed BCD values to create a packed BCD result.
The AL register is the implied source and destination operand. The DAS instruction is only
useful when it follows a SUB instruction that subtracts (binary subtraction) one 2-digit, packed
BCD value from another and stores a byte result in the AL register. The DAS instruction then
adjusts the contents of the AL register to contain the correct 2-digit, packed BCD result. If a
decimal borrow is detected, the CF and AF flags are set accordingly.
**************************************/
//!Instruction das behavior method.
void ac_behavior( das ){
  acprintf ( " DAS <Not Tested>\n" );

  uint tempAL,old_AL,old_FLAG_CF;
  uint op1, res; //para checar Flags
  old_AL = GR[AL];
  old_FLAG_CF = SPR[FLAG_CF];
 
  if( ((GR[AL] & 0x0F) > 9) || (testFlag(FLAG_AF)) ){
    tempAL = GR[AL];
    op1 = tempAL;
    tempAL = tempAL - 6;
    res = tempAL;
    GR[AL] = tempAL;  
    writeFlag(FLAG_CF,( old_FLAG_CF || ( checkCarry8(OPER_SUB,op1,6,0) )) );
    setFlag(FLAG_AF);

    checkFlags8(OPER_SUB, op1,6,res,FLAG_SF );
    checkFlags8(OPER_SUB, op1,6,res,FLAG_ZF );
    checkFlags8(OPER_SUB, op1,6,res,FLAG_PF );

  }//if
  else
    resetFlag(FLAG_AF);

  if ((old_AL > 0x99) || (old_FLAG_CF == 1)){
    tempAL = GR[AL];
    op1 = tempAL;
    tempAL = tempAL - 0x60;
    res = tempAL;
    GR[AL] = tempAL;
    setFlag(FLAG_CF);

    checkFlags8(OPER_SUB, op1,0x60,res,FLAG_SF );
    checkFlags8(OPER_SUB, op1,0x60,res,FLAG_ZF );
    checkFlags8(OPER_SUB, op1,0x60,res,FLAG_PF );
  }//if
  else
    resetFlag(FLAG_CF);



}//ac_behavior ( das );






/*







//!Instruction aam behavior method.
void ac_behavior( aam ){
	acprintf ( " AAM <Not implemented>\n" );
} //ac_behavior( aam )



//!Instruction aam behavior method.
void ac_behavior( aam ){
	acprintf ( " AAM <Not implemented>\n" );
} //ac_behavior( aam )


//!Instruction aam behavior method.
void ac_behavior( aam ){
	acprintf ( " AAM <Not implemented>\n" );
} //ac_behavior( aam )


*/
