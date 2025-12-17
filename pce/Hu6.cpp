#include "Hu6.H"

auto Hu6Carry(Hu6 &cpu)
{
  return (cpu.flags & FLG_CAR) > 0;
}

auto Hu6SetFlag(Hu6 &cpu, U8 flag, Bool set)
{
  if (set)
    cpu.flags|=flag;
  else
    cpu.flags&=~flag;
}

auto Hu6MemMap(Hu6 &cpu, U16 addr) -> U32
{
  U16 low_addr=addr&BNK_MASK;
  U16 bnk_sel=addr>>BNK_SHIFT;
  return ((U32)(cpu.mpr[bnk_sel])<<BNK_SHIFT)|low_addr;
}

auto Hu6Cycle(Hu6 &cpu)
{
  if (cpu.fast) {
    for (U8 i=0; i<3; i++) 
      cpu.clk.Cycle();
  } else {
    for (U8 i=0; i<12; i++) 
      cpu.clk.Cycle();
  }
}

auto Hu6Read16(Hu6 &cpu, U16 addr) -> U16
{
  U8 byte = cpu.bus.Read(Hu6MemMap(cpu, addr));
  Hu6Cycle(cpu);
  U8 byte2 = cpu.bus.Read(Hu6MemMap(cpu, addr+1));
  Hu6Cycle(cpu);
  return (((U16)byte2)<<8) | byte;
}

auto Hu6ReadMem(Hu6 &cpu, U16 addr) -> U8
{
  U8 byte = cpu.bus.Read(Hu6MemMap(cpu, addr));
  Hu6Cycle(cpu);
  return byte;
}

auto Hu6WriteMem(Hu6 &cpu, U16 addr, U8 data) -> void
{
  cpu.bus.Write(Hu6MemMap(cpu, addr), data);
  Hu6Cycle(cpu);
}

auto OpcFmt(Opc &opc, Hu6 &cpu)
{
  char bufOpc[1024];
  sprintf(bufOpc, "%04X (%06X) %5s", cpu.pc-1, Hu6MemMap(cpu, cpu.pc-1), opc.instr_name);

  U16 pc = cpu.pc;
  auto Read8 = [&]() -> U8 {
    return cpu.bus.Read(Hu6MemMap(cpu, pc++));
  };

  auto Read16 = [&]() -> U16 {
    U16 l = cpu.bus.Read(Hu6MemMap(cpu, pc++));
    U16 h = cpu.bus.Read(Hu6MemMap(cpu, pc++));

    return (h << 8) | l;
  };


  char bufAdr[1024];
  switch (opc.addr_mode) {
  case ADR_IMP:
    sprintf(bufAdr, "");
    break;
  case ADR_IMM:
    sprintf(bufAdr, "$%02X", Read8());
    break;
  case ADR_ZPG:
    sprintf(bufAdr, "[$20%02X]", Read8());
    break;
  case ADR_ZPX:
    sprintf(bufAdr, "[$20%02X], X", Read8());
    break;
  case ADR_ZPY:
    sprintf(bufAdr, "[$20%02X], Y", Read8());
    break;
  case ADR_ZPR:
    sprintf(bufAdr, "[$20%02X], %hhd", Read8(), (I8)Read8());
    break;
  case ADR_REL:
    sprintf(bufAdr, "%hhd", (I8)Read8());
    break;
  case ADR_ABS:
    sprintf(bufAdr, "$%04X", Read16());
    break;
  case ADR_ABX:
    sprintf(bufAdr, "$%04X, X", Read16());
    break;
  case ADR_ABY:
    sprintf(bufAdr, "$%04X, Y", Read16());
    break;
  case ADR_ABI:
    sprintf(bufAdr, "[$%04X]", Read16());
    break;
  case ADR_AXI:
    sprintf(bufAdr, "[$%04X, X]", Read16());
    break;
  case ADR_ACC:
    sprintf(bufAdr, "A");
    break;
  default:
    sprintf(bufAdr, "UNHANDLED\n");
    break;
  }

  if (strcmp(opc.instr_name, "UNK") == 0) {
    sprintf(bufAdr, "%02X", opc.opc);
  }
  
  char bufRegs[1024];
  sprintf(bufRegs, "A=%02X X=%02X Y=%02X P=%02X", cpu.a, cpu.x, cpu.y, cpu.flags);

  char combined[2048];
  sprintf(combined, "%15s %-20s %s", bufOpc, bufAdr, bufRegs);
  return std::string(combined);
}

auto Hu6SetNZ(Hu6 &cpu, I8 data) -> U8
{
  if (data==0) cpu.flags|=FLG_ZER;
  else cpu.flags&=~FLG_ZER;
  if (data<0) cpu.flags|=FLG_NEG;
  else cpu.flags&=~FLG_NEG;
  return data;
}

auto Hu6SetC(Hu6 &cpu, Bool set)
{
  if (set) cpu.flags|=FLG_CAR;
  else cpu.flags&=~FLG_CAR;
}

auto StkRead(Hu6 &cpu) -> U8
{
  return Hu6ReadMem(cpu, STK_START+cpu.sp);
}

auto StkWrite(Hu6 &cpu, U8 val)
{
  return Hu6WriteMem(cpu, STK_START+cpu.sp, val);
}

auto StkPush(Hu6 &cpu, U8 val)
{
  Hu6ReadMem(cpu, STK_START+cpu.sp); // dummy
  return Hu6WriteMem(cpu, STK_START+cpu.sp--, val);
}

auto StkPop(Hu6 &cpu) -> U8
{
  Hu6ReadMem(cpu, STK_START+((cpu.sp+1)&0xFF)); // dummy
  return Hu6ReadMem(cpu, STK_START+ ++cpu.sp);
}

auto Hu6AdrNew(Hu6 &cpu, U64 addr_mode) -> Hu6Adr
{
  Hu6Adr adr={};
  adr.cpu=&cpu;

  switch (addr_mode) {
  case ADR_IMP: break;
  case ADR_IMM: adr.isimm=true; adr.imm=Hu6ReadMem(cpu, cpu.pc++); break;
  case ADR_ZPG: adr.adr=Hu6ReadMem(cpu, cpu.pc++)+ZPG_START; break;
  case ADR_ZPX: adr.adr=((Hu6ReadMem(cpu, cpu.pc++)+cpu.x)&0xFF)|ZPG_START; break; 
  case ADR_ZPY: adr.adr=((Hu6ReadMem(cpu, cpu.pc++)+cpu.y)&0xFF)|ZPG_START; break; 
  case ADR_ZPR: Dbg::Fail("CPU/A", "Handle ZPR in the instruction\n"); 
  case ADR_ABS: adr.adr=Hu6ReadMem(cpu, cpu.pc++); adr.adr|=Hu6ReadMem(cpu, cpu.pc++)<<8; break;
  case ADR_ABX: adr.adr=Hu6ReadMem(cpu, cpu.pc++); adr.adr|=Hu6ReadMem(cpu, cpu.pc++)<<8; adr.adr+=cpu.x; break;
  case ADR_ACC: /* dumdum */ Hu6ReadMem(cpu, cpu.pc); adr.isa=true; adr.imm=cpu.a; break;
  default: Dbg::Fail("CPU/A", "AdrNew on unknown adr mode %02X PC=%04X CYC=%08llX\n", (int)addr_mode, cpu.instr_pc, cpu.clk.counter); break;
  }

  return adr;
}

auto AdrRead(Hu6Adr &adr) -> U8
{
  if (adr.isimm) {
    Dbg::Info("CPU/A", "AdrRead IMM: %02X", adr.imm);
    return adr.imm;
  }

  if (adr.isa) {
    Dbg::Info("CPU/A", "AdrRead A: %02X", adr.imm);
    return adr.imm;
  }

  U8 val = Hu6ReadMem(*adr.cpu, adr.adr);
  Dbg::Info("CPU/A", "AdrRead SRC:$%04X VAL:%02X", adr.adr, val);
  return val;
}

auto AdrWrite(Hu6Adr &adr, U8 data) -> void
{
  if (adr.isimm) {
    Dbg::Fail("CPU/A", "AdrWrite on IMM adr mode o_o");
  }

  if (adr.isa) {
    Dbg::Info("CPU/A", "AdrWrite A VAL:%02X", data);
    adr.cpu->a = data;
    return;
  }

  Dbg::Info("CPU/A", "AdrWrite DST:%04X VAL:%02X", adr.adr, data);
  Hu6ReadMem(*adr.cpu, adr.adr); // dummy
  Hu6WriteMem(*adr.cpu, adr.adr, data);
}

auto InstrUNK(Hu6 &cpu, Opc opc)
{
  Dbg::Fail("CPU", "UNK opcode %02X PC=%04X CYC=%08llX", opc.opc, cpu.instr_pc, cpu.clk.counter);
}

auto InstrCLI(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.flags &= ~FLG_INT;
}

auto InstrSEI(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.flags |= FLG_INT;
}

auto InstrCSH(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.fast = true;
}

auto InstrCSL(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.fast = false;
}

auto InstrCLD(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.flags&=~FLG_DEC;
}

auto InstrLDX(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr = Hu6AdrNew(cpu, opc.addr_mode);
  cpu.x=Hu6SetNZ(cpu, AdrRead(adr));
}

auto InstrLDA(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr = Hu6AdrNew(cpu, opc.addr_mode);
  cpu.a=Hu6SetNZ(cpu, AdrRead(adr));
}

auto InstrTAM(Hu6 &cpu, Opc opc)
{
  U8 mask=Hu6ReadMem(cpu, cpu.pc++);
  Dbg::Info("CPU/I", "TAM: %02X=%02X", mask, cpu.a);
  for (U8 i=0; i<8; i++) {
    if (mask&(1<<i)) {
      Dbg::Info("CPU/I", "TAM: Mapping %04X to page %06X (page: %02X)", i*0x2000, cpu.a<<13, cpu.a);
      cpu.mpr[i]=cpu.a;
    }
  }
}

auto InstrTMA(Hu6 &cpu, Opc opc)
{
  U8 mask=Hu6ReadMem(cpu, cpu.pc++);
  if (PopCnt32(mask) != 1) {
    Dbg::Fail("CPU/I", "Invalid TMA popcnt: %d", PopCnt32(mask));
  }

  U8 bit=Ctz32(mask);

  Dbg::Info("CPU/I", "TMA: %d(%02X)=%02X", bit, mask, cpu.mpr[bit]);
  cpu.a = cpu.mpr[bit];
}

auto InstrTXS(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.sp=cpu.x;
}

auto InstrSTZ(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  AdrWrite(adr, 0);
}

auto InstrTII(Hu6 &cpu, Opc opc)
{
  U64 clk_start=cpu.clk.counter;

  U16 src=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 dst=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 len=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 len_orig=len;
  U16 src_orig=src;
  U16 dst_orig=dst;

  StkPush(cpu, cpu.y);
  StkPush(cpu, cpu.a);
  StkWrite(cpu, cpu.x);

  U64 clk_tran_start=cpu.clk.counter;

  while (--len) {
    Hu6WriteMem(cpu, dst++, Hu6ReadMem(cpu, src++));
  }

  U64 clk_tran_end=cpu.clk.counter;

  cpu.x=StkRead(cpu);
  cpu.a=StkPop(cpu);
  cpu.y=StkPop(cpu);

  U64 cyc_total=cpu.clk.counter-clk_start;
  U64 cyc_tran=clk_tran_end-clk_tran_start;
  U64 cyc_notran=cyc_total-cyc_tran+1; // account for opc cycle

  Dbg::Warn("CPU/I", "BLK TII: src:%04X, dst:%04X, cyctot:%lld, cyctrn:%lld, cycntr:%lld, len:%04hX", src_orig, dst_orig, cyc_total, cyc_tran, cyc_notran, len_orig);
}

auto InstrTAI(Hu6 &cpu, Opc opc)
{
  U64 clk_start=cpu.clk.counter;

  U16 src=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 dst=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 len=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 len_orig=len;
  U16 src_orig=src;
  U16 dst_orig=dst;

  StkPush(cpu, cpu.y);
  StkPush(cpu, cpu.a);
  StkWrite(cpu, cpu.x);

  U64 clk_tran_start=cpu.clk.counter;
  bool inc = false;

  while (--len) {
    Hu6WriteMem(cpu, dst++, Hu6ReadMem(cpu, inc ? src++ : src--));
    inc = !inc;
  }

  U64 clk_tran_end=cpu.clk.counter;

  cpu.x=StkRead(cpu);
  cpu.a=StkPop(cpu);
  cpu.y=StkPop(cpu);

  U64 cyc_total=cpu.clk.counter-clk_start;
  U64 cyc_tran=clk_tran_end-clk_tran_start;
  U64 cyc_notran=cyc_total-cyc_tran+1; // account for opc cycle

  Dbg::Warn("CPU/I", "BLK TAI: src:%04X, dst:%04X, cyctot:%lld, cyctrn:%lld, cycntr:%lld, len:%04hX", src_orig, dst_orig, cyc_total, cyc_tran, cyc_notran, len_orig);
}

auto InstrJSR(Hu6 &cpu, Opc opc)
{
  // todo: possible cycle hole?
  U16 addr=Hu6Read16(cpu, cpu.pc);
  cpu.pc += 2;

  /* subtract 1, jsr puts pc at byte 3 not next opcode */
  StkPush(cpu, (cpu.pc-1)>>8);
  StkPush(cpu, (cpu.pc-1)&0xff);

  Dbg::Info("CPU/I", "JSR: pc:$%04hX, addr:$%04hX", (cpu.pc-1), addr);

  cpu.pc=addr;
}

auto InstrSTA(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  AdrWrite(adr, cpu.a);
}

auto InstrSTX(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  AdrWrite(adr, cpu.x);
}

auto InstrSTY(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  AdrWrite(adr, cpu.x);
}

auto InstrRTS(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.pc=StkPop(cpu);
  cpu.pc|=StkPop(cpu)<<8;
  cpu.pc++;
  Dbg::Info("CPU/I", "RTS: pc:$%04hX", cpu.pc);
}

auto InstrCLX(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.x=0;
}

auto InstrCMP(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  U8 check = AdrRead(adr);
  U8 result = cpu.a-check;
  Hu6SetFlag(cpu, FLG_CAR, cpu.a>=check);
  Hu6SetFlag(cpu, FLG_ZER, cpu.a==check);
  Hu6SetFlag(cpu, FLG_NEG, result&0x80);
}

auto InstrBNE(Hu6 &cpu, Opc opc)
{
  I8 rel=Hu6ReadMem(cpu, cpu.pc++);
  if ((cpu.flags&FLG_ZER) == 0) {
    /* TODO: handle page crossing cycle */
    Hu6Cycle(cpu);
    cpu.pc += rel;
    Hu6Cycle(cpu);
  }
}

auto InstrBPL(Hu6 &cpu, Opc opc)
{
  I8 rel=Hu6ReadMem(cpu, cpu.pc++);
  if ((cpu.flags&FLG_NEG) == 0) {
    /* TODO: handle page crossing cycle */
    Hu6Cycle(cpu);
    cpu.pc += rel;
    Hu6Cycle(cpu);
  }
}

auto InstrBEQ(Hu6 &cpu, Opc opc)
{
  I8 rel=Hu6ReadMem(cpu, cpu.pc++);
  if (cpu.flags&FLG_ZER) {
    /* TODO: handle page crossing cycle */
    Hu6Cycle(cpu);
    cpu.pc += rel;
    Hu6Cycle(cpu);
  }
}

auto InstrBRA(Hu6 &cpu, Opc opc)
{
  I8 rel=Hu6ReadMem(cpu, cpu.pc++);
  /* TODO: handle page crossing cycle */
  Hu6Cycle(cpu);
  cpu.pc += rel;
  Hu6Cycle(cpu);
}

auto InstrINX(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.x++;
  Hu6SetNZ(cpu, cpu.x);
}

auto InstrDEX(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.x--;
  Hu6SetNZ(cpu, cpu.x);
}

auto InstrCLA(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.a=0;
}

auto InstrCLC(Hu6 &cpu, Opc opc) 
{
  Hu6ReadMem(cpu, cpu.pc);
  Hu6SetFlag(cpu, FLG_CAR, 0);
}

auto InstrADC(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);

  U8 val = AdrRead(adr);
  U16 carry = (U16)cpu.a + (U16)val + (U16)Hu6Carry(cpu);
  I8 res = (I8)cpu.a + (I8)val + (I8)Hu6Carry(cpu);
  I16 overflow = (I16)(I8)cpu.a + (I16)(I8)val + (I16)(I8)Hu6Carry(cpu);

  // god i hope this works
  Hu6SetFlag(cpu, FLG_CAR, carry&0xFF00);
  Hu6SetFlag(cpu, FLG_NEG, res<0);
  Hu6SetFlag(cpu, FLG_ZER, res==0);
  Hu6SetFlag(cpu, FLG_OFW, overflow<-128 || overflow>127);
  cpu.a = res;
}

auto InstrINC(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  I8 res = AdrRead(adr);
  res++;
  Hu6SetFlag(cpu, FLG_NEG, res<0);
  Hu6SetFlag(cpu, FLG_ZER, res==0);
  AdrWrite(adr, res);
}

auto InstrPHA(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  StkPush(cpu, cpu.a);
}

auto InstrPHX(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  StkPush(cpu, cpu.x);
}

auto InstrPHY(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  StkPush(cpu, cpu.y);
}

auto InstrPLA(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  cpu.a = StkPop(cpu);
  Hu6SetNZ(cpu, cpu.a);
}

auto InstrPLX(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  cpu.x = StkPop(cpu);
  Hu6SetNZ(cpu, cpu.x);
}

auto InstrPLY(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  cpu.y = StkPop(cpu);
  Hu6SetNZ(cpu, cpu.y);
}

auto InstrBBS(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, ADR_ZPG);
  U8 value = AdrRead(adr);
  I8 rel = Hu6ReadMem(cpu, cpu.pc++);
  if (value & (1<<opc.extra)) {
    cpu.pc += rel;
  }
}

auto InstrRMB(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  U8 value = AdrRead(adr);
  value &= ~(1<<opc.extra);
}

auto InstrSMB(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  U8 value = AdrRead(adr);
  value |= 1<<opc.extra;
}

auto InstrJMP(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  cpu.pc = adr.adr;
}

auto InstrRTI(Hu6 &cpu, Opc opc)
{
  cpu.flags = StkPop(cpu);
  cpu.pc = StkPop(cpu)<<8;
  cpu.pc |= StkPop(cpu);
}

auto InstrST0(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  cpu.bus.Write(0x1FE000, AdrRead(adr)); // VDC register
  Hu6Cycle(cpu);
}

auto InstrST1(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  cpu.bus.Write(0x1FE002, AdrRead(adr)); // VDC data low
  Hu6Cycle(cpu);
}

auto InstrST2(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  cpu.bus.Write(0x1FE003, AdrRead(adr)); // VDC data hi
  Hu6Cycle(cpu);
}

auto InstrASL(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  U8 value = AdrRead(adr);
  Hu6SetC(cpu, value&0x80);
  AdrWrite(adr, Hu6SetNZ(cpu, value<<1));
}

auto Hu6DoInterrupt(Hu6 &cpu, U16 addr)
{
  StkPush(cpu, (cpu.pc)&0xff);
  StkPush(cpu, (cpu.pc)>>8);
  StkPush(cpu, cpu.flags);

  cpu.flags |= FLG_INT;

  cpu.pc = addr;
}

auto Hu6TryIrq(Hu6 &cpu) -> bool
{
  if ((cpu.flags & FLG_INT) == 0) {
    return true;
  }

  return false;
}


auto Hu6Irq(Hu6 &cpu) -> void
{
  if (!Hu6TryIrq(cpu)) {
    return;
  }

  if (cpu.irq.Check(Irq::IRQ1)) {
    Hu6DoInterrupt(cpu, Hu6Read16(cpu, MEM_VECIRQ1));
    Dbg::Info("CPU/Q", "Vblank received!");
  } else if (cpu.irq.Check(Irq::TIMER)) {
    Hu6DoInterrupt(cpu, Hu6Read16(cpu, MEM_VECTIMER));
    Dbg::Info("CPU/Q", "Timer received!");
  } else if (cpu.irq.Check(Irq::IRQ2)) {
    Hu6DoInterrupt(cpu, Hu6Read16(cpu, MEM_VECIRQ2));
    Dbg::Info("CPU/Q", "IRQ2 received!");
  }
}

auto Hu6GetOpc(U8 opc) -> Opc
{
  switch (opc) {
  case 0x03:return{"ST0",ADR_IMM,InstrST0,opc,4};
  case 0x06:return{"ASL",ADR_ZPG,InstrASL,opc,6};
  case 0x0E:return{"ASL",ADR_ABS,InstrASL,opc,7};
  case 0x10:return{"BPL",ADR_REL,InstrBPL,opc,2};
  case 0x13:return{"ST1",ADR_IMM,InstrST1,opc,4};
  case 0x16:return{"ASL",ADR_ZPX,InstrASL,opc,6};
  case 0x18:return{"CLC",ADR_IMP,InstrCLC,opc,2};
  case 0x1E:return{"ASL",ADR_ABX,InstrASL,opc,7};
  case 0x1A:return{"INC",ADR_ACC,InstrINC,opc,2};
  case 0x20:return{"JSR",ADR_ABS,InstrJSR,opc,7};
  case 0x23:return{"ST2",ADR_IMM,InstrST2,opc,4};
  case 0x40:return{"RTI",ADR_IMP,InstrRTI,opc,7};
  case 0x43:return{"TMA",ADR_IMP,InstrTMA,opc,4};
  case 0x48:return{"PHA",ADR_IMP,InstrPHA,opc,3};
  case 0x4C:return{"JMP",ADR_ABS,InstrJMP,opc,4};
  case 0x53:return{"TAM",ADR_IMP,InstrTAM,opc,5};
  case 0x54:return{"CSL",ADR_IMP,InstrCSL,opc,0}; // csl cyc count is unknown
  case 0x58:return{"CLI",ADR_IMP,InstrCLI,opc,2};
  case 0x5A:return{"PHY",ADR_IMP,InstrPHY,opc,3};
  case 0x60:return{"RTS",ADR_IMP,InstrRTS,opc,7};
  case 0x62:return{"CLA",ADR_IMP,InstrCLA,opc,2};
  case 0x64:return{"STZ",ADR_ZPG,InstrSTZ,opc,4};
  case 0x65:return{"ADC",ADR_ZPG,InstrADC,opc,4};
  case 0x68:return{"PLA",ADR_IMP,InstrPLA,opc,4};
  case 0x69:return{"ADC",ADR_IMM,InstrADC,opc,2};
  case 0x73:return{"TII",ADR_IMP,InstrTII,opc,0}; // blk transfer instr can be variable
  case 0x78:return{"SEI",ADR_IMP,InstrSEI,opc,2};
  case 0x7A:return{"PLY",ADR_IMP,InstrPLY,opc,4};
  case 0x80:return{"BRA",ADR_REL,InstrBRA,opc,4};
  case 0x82:return{"CLX",ADR_IMP,InstrCLX,opc,2};
  case 0x85:return{"STA",ADR_ZPG,InstrSTA,opc,4};
  case 0x8D:return{"STA",ADR_ABS,InstrSTA,opc,5};
  case 0x8E:return{"STX",ADR_ABS,InstrSTX,opc,5};
  case 0x9A:return{"TXS",ADR_IMP,InstrTXS,opc,2};
  case 0x9C:return{"STZ",ADR_ABS,InstrSTZ,opc,5};
  case 0xA2:return{"LDX",ADR_IMM,InstrLDX,opc,2};
  case 0xA5:return{"LDA",ADR_ZPG,InstrLDA,opc,4};
  case 0xA9:return{"LDA",ADR_IMM,InstrLDA,opc,2};
  case 0xAD:return{"LDA",ADR_ABS,InstrLDA,opc,5};
  case 0xBD:return{"LDA",ADR_ABX,InstrLDA,opc,5};
  case 0xC5:return{"CMP",ADR_ZPG,InstrCMP,opc,4};
  case 0xC9:return{"CMP",ADR_IMM,InstrCMP,opc,2};
  case 0xCA:return{"DEX",ADR_IMP,InstrDEX,opc,2};
  case 0xD0:return{"BNE",ADR_REL,InstrBNE,opc,2};
  case 0xD4:return{"CSH",ADR_IMP,InstrCSH,opc,0}; // csh cyc count is unknown
  case 0xD8:return{"CLD",ADR_IMP,InstrCLD,opc,2};
  case 0xDA:return{"PHX",ADR_IMP,InstrPHX,opc,3};
  case 0xE8:return{"INX",ADR_IMP,InstrINX,opc,2};
  case 0xF0:return{"BEQ",ADR_REL,InstrBEQ,opc,2};
  case 0xF3:return{"TAI",ADR_IMP,InstrTAI,opc,0}; // blk instruction
  case 0xFA:return{"PLX",ADR_IMP,InstrPLX,opc,4};
  }

  U8 hi = opc>>4;
  U8 lo = opc&0xF;
  if (lo == 0xF) {
    // BBR 0xF0 - 0xF7
    if (hi >= 0x0 && hi <= 0x7) {
      // TODO
    }

    // BBS 0xF8 - 0xFF
    if (hi >= 0x8 && hi <= 0xF) {
      return {"BBS",ADR_ZPR,InstrBBS,opc,6,(U8)(hi-0x8)};
    }
  } else if (lo == 0x7) {
    // RMB 0x70 - 0x77
    if (hi >= 0x0 && hi <= 0x7) {
      return {"RMB",ADR_ZPG,InstrRMB,opc,7,(U8)(hi-0x0)};
    }

    // SMB 0x78 - 0x7F
    if (hi >= 0x8 && hi <= 0xF) {
      return {"SMB",ADR_ZPG,InstrSMB,opc,7,(U8)(hi-0x8)};
    }
  }

  return{"UNK",ADR_IMP,InstrUNK,opc,0};
}


auto Hu6::Run() -> void
{
  Opc opc_tbl[256];

  for (int i=0; i<256; i++) {
    opc_tbl[i] = Hu6GetOpc(i);
  }

  while (this->clk.IsGoing()) {
    Hu6Irq(*this);

    this->instr_pc = this->pc;
    U64 cyc_pre = this->clk.counter;
    U8 opc_id = Hu6ReadMem(*this, pc++);
    Opc opc = opc_tbl[opc_id];
    std::string opc_fmt = OpcFmt(opc, *this);
    Dbg::Info("CPU/I", "%s", opc_fmt.c_str());
    /* TODO disable for mem flag set instruction */
    this->flags &= ~FLG_MEM;
    opc.Handler(*this, opc);
    U64 cyc_post = this->clk.counter;
    I64 cyc_diff = cyc_post-cyc_pre-clk.extra_counter;
    clk.extra_counter = 0; /* HACK */
    opc.ref_cyc *= 3;
    if (!this->fast) opc.ref_cyc*=4;
    if (opc.ref_cyc!=0 && cyc_diff!=opc.ref_cyc) {
      Dbg::Warn("CPU/T", "cyc mismatch %s (exp:%d,got:%lld) PC=%04X CYC=%08lld",
              opc.instr_name, opc.ref_cyc, cyc_diff, instr_pc, clk.counter);
    }
  }
}