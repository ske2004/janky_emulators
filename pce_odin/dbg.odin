package main

import "core:fmt"
import "core:log"

format_adr :: proc(cpu: ^Cpu, opc_info: OpcInfo, adr_decoded: AdrDecoded) -> string {
  #partial switch opc_info.adr {
    case .Imp:
      return ""
    case .Imm:
      return fmt.aprintf("$%02X", adr_decoded.(AdrImm).val)
    case .Acc:
      return fmt.aprintf("A=$%02X", cpu.a)
    case .Abs, .Zpg:
      return fmt.aprintf("%04X", adr_decoded.(AdrBasic).addr)
    case .Abx:
      return fmt.aprintf("%04X, X=$%02X", adr_decoded.(AdrBasic).addr, adr_decoded.(AdrBasic).offs)
    case .Aby:
      return fmt.aprintf("%04X, Y=$%02X", adr_decoded.(AdrBasic).addr, adr_decoded.(AdrBasic).offs)
    case .Rel:
      return fmt.aprintf("%+d", adr_decoded.(AdrRel).val)
    case .Zxi:
      return fmt.aprintf("(%04X, X=$%02X)", adr_decoded.(AdrIndirect).addr, adr_decoded.(AdrIndirect).inner_offs)
    case .Ziy:
      return fmt.aprintf("(%04X), Y=$%02X", adr_decoded.(AdrIndirect).addr, adr_decoded.(AdrIndirect).outer_offs)
    case:
      return "???"
  }
}

trace_indent := 0

log_instr_info :: proc(fmt_arg: string, args: ..any, no_log: bool = false) {
  when #config(ENABLE_TRACING, false) {
    if !no_log {
      log.infof(fmt_arg, ..args)
    }
    if trace_indent > 0 {
      for i in 0..<trace_indent {
        fmt.fprintf(instr_dbg_file, "  ", flush=false)
      }
    }

    fmt.fprintf(instr_dbg_file, "  ", flush=false)
    fmt.fprintf(instr_dbg_file, fmt_arg, ..args, flush=false)
    fmt.fprintf(instr_dbg_file, "\n")
  }
}

trace_instr :: proc(cpu: ^Cpu, opc: u8, pc: u16, opc_info: OpcInfo, adr_decoded: AdrDecoded, stdout := false) {
  when #config(ENABLE_TRACING, false) {
    context.allocator = context.temp_allocator
    t := format_adr(cpu, opc_info, adr_decoded)

    bitset := bit_set[Instr]{.BBR, .BBS, .RMB, .SMB}

    extra := ""

    if opc_info.instr in bitset {
      extra = fmt.aprintf("%v", opc_info.extra)
    }

    if stdout {
      fmt.printf("%02X %04X %06X %s%s %s\n", opc, pc, cpu_mem_map(cpu, pc), opc_info.instr, extra, t)
    } else {
      if trace_indent > 0 {
        for i in 0..<trace_indent {
          fmt.fprintf(instr_dbg_file, "  ", flush=false)
        }
      }
      fmt.fprintf(instr_dbg_file, "%02X %04X %06X %s%s %s\n", opc, pc, cpu_mem_map(cpu, pc), opc_info.instr, extra, t)
    }
  }
}