/*
  initial: 31.1.2015
  implement fetch execute cycle, interrupts
  16.8.2015: implement step
*/

#include "flags.h"
#include "alu.h"
#include "register.h"
#include "opcode_tbl.h"
#include "memory.h"


extern struct _vic* vic;
int sp2int(struct _6510_cpu*cpu);
void draw_bitmap_memory(struct _6510_cpu* cpu, struct _vic* vic, char memory[][9]);
void rnd_update();

void access_memory( struct _6510_cpu* cpu, char memory[][9]) {
  int adr = word2int(cpu->abrh, cpu->abrl);
  
  if(adr==0){ 
  } else  {
    if(adr==1){
    } else {
      if(adr>0xA000 || adr<0xBFFF){ // Basic ROM
	if(memory[1][LORAM] =='0'){
	  cp_byte(ram(adr, memory), cpu->dbr);  // OFF
	} else {
	  cp_byte(basic_ROM(adr, memory),cpu->dbr);  // ON
	} 
      }
    }
  }
  if(adr>0xD000 || adr<0xDFFF){ //  CHAR ROM
    if(memory[1][CHAREN] =='0'){
      cp_byte(char_ROM(adr, memory),cpu->dbr);  // ON
    } else {
      if(memory[1][0]=='0' && memory[1][1]=='0') {
	cp_byte(ram(adr, memory),cpu->dbr);  // OFF
      } else { 
	cp_byte(IO_ram(adr, memory),cpu->dbr);  // I/O memory
      }
	
    }
    if(adr>0xE000){    // kernel_ROM
      if(memory[1][HIRAM] =='0'){
	cp_byte(ram(adr, memory),cpu->dbr); // OFF
      } else {
	cp_byte(kernel_ROM(adr, memory),cpu->dbr); // ON      
      }
    }
  }
}

// wird vom CPU beim erkennen eines interrupts aufgerufen
void cpu_6502_interrupt(struct _6510_cpu* cpu, char mem[][9]){
  // set interrupt flag
  cpu->flags[IFLAG] = '1';
  // reset cpu->interrupt pin (low active)
  cpu->interrupt = '1';
  int sp = sp2int(cpu);
  cp_register(cpu->pch, mem[sp]);
  cp_register(cpu->pcl, mem[ ((sp - 1) & 0xff ) | 0x100 ]);
  cp_register(cpu->flags, mem[ ((sp - 2) & 0xff ) | 0x100 ]);
  alu(ALU_OP_SUB, cpu->sp, "00000011", cpu->sp, 0);
  cp_register(mem[0xfffe], cpu->pcl);
  cp_register(mem[0xffff], cpu->pch);
}

int pc2int(struct _6510_cpu*cpu){
  return  (conv_bitstr2int(cpu->pch,0,7)<<8 ) | ( conv_bitstr2int(cpu->pcl,0,7));
}

void inc_pc(struct _6510_cpu* cpu){
  int i= 7;
  while(i>-1){
    if(cpu->pcl[i]=='0'){
      cpu->pcl[i] = '1';
      return;
    }else
      cpu->pcl[i] = '0';
    i--;
  }
  i = 7;
  while(i>-1){
    if(cpu->pch[i]=='0'){
      cpu->pch[i] = '1';
      return;
    }else
      cpu->pch[i] = '0';
    i--;
  }
  return;
}

void dec_pc(struct _6510_cpu* cpu){
  int i= 7;
  while(i>-1){
	if(cpu->pcl[i]=='1'){
	  cpu->pcl[i] = '0';
	  return;
	}else
	  cpu->pcl[i] = '1';
	i--;
  }
  i = 7;
  while(i>-1){
	if(cpu->pch[i]=='1'){
	  cpu->pch[i] = '0';
	  return;
	}else
	  cpu->pch[i] = '1';
	i--;
  }
  return;
}



int fetch( struct _6510_cpu* cpu, char memory[][9]  ) {
  if( (cpu->interrupt =='0') && (cpu->flags[IFLAG] == '0') ){  
    cpu_6502_interrupt(cpu, memory);
  }
  rnd_update(cpu, memory);

  cp_byte(cpu->pcl, cpu->abrl);
  cp_byte(cpu->pch, cpu->abrh);
  cpu->rw='1';
  access_memory(cpu, memory);

  
  cp_byte(cpu->dbr, cpu->ir);
  
  inc_pc(cpu);  
  return conv_bitstr2int(cpu->ir,0,7);
}

int exec( struct _6510_cpu* cpu, char memory[][9] ) {
  int op_code = conv_bitstr2int(cpu->ir,0,7);
  struct opcode_entry* entryp = operation_lookup(op_code);
  void (*fct)(struct _6510_cpu* cpu, char memory[][9] );
 
 fct = entryp->op_fct;
  if(fct){
    (*fct)(cpu, memory);
    draw_bitmap_memory(cpu, vic, memory);
    cpu->cycles += entryp->cycles;
    if(cpu->cycles %1000 <10)
      draw_bitmap_memory(cpu, vic, memory);
    
  } else {
    return -1;
  }
  return 1;
}

void fetch_exec_step( struct _6510_cpu* cpu, char memory[][9] ) {
  fetch(cpu, memory);
  exec(cpu, memory);
}


void reset_cpu(struct _6510_cpu* cpu){
  char null[9] = "00000000";
  cp_byte(null,cpu->pcl);
  cp_byte(null,cpu->pch);
  cp_byte(null,cpu->regx);
  cp_byte(null,cpu->regy);
  cp_byte(null,cpu->rega);
  cp_byte("00110000",cpu->flags);
  cp_byte("11111111",cpu->sp);
  cp_byte(null,cpu->abrl);
  cp_byte(null,cpu->abrh);
  cp_byte(null,cpu->dbr);
  cp_byte(null,cpu->ir);
  cp_byte(null,cpu->pcl);
  cp_byte(null,cpu->pcl);
  cp_byte(null,cpu->pcl);
  cpu->interrupt='1';
  cpu->cycles=0;
}

