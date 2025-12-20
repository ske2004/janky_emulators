package main

import "core:os"
import "core:log"
import "core:time"
import "core:fmt"
import "core:mem"
import "base:runtime"
import "core:sync"
import "core:prof/spall"
import "vendor:/raylib"
  
instr_dbg_file : os.Handle

when #config(ENABLE_SPALL, false) {

  spall_ctx: spall.Context
  @(thread_local) spall_buffer: spall.Buffer

  @(instrumentation_enter)
  spall_enter :: proc "contextless" (proc_address, call_site_return_address: rawptr, loc: runtime.Source_Code_Location) {
    spall._buffer_begin(&spall_ctx, &spall_buffer, "", "", loc)
  }

  @(instrumentation_exit)
  spall_exit :: proc "contextless" (proc_address, call_site_return_address: rawptr, loc: runtime.Source_Code_Location) {
    spall._buffer_end(&spall_ctx, &spall_buffer)
  }

  @(no_instrumentation)
  spall_assert_fail :: proc(prefix, message: string, loc: runtime.Source_Code_Location) -> ! {
    spall.buffer_flush(&spall_ctx, &spall_buffer)
    runtime.default_assertion_failure_proc(prefix, message, loc)
  }
}

main :: proc() {
  when #config(ENABLE_SPALL, false) {
    spall_ctx = spall.context_create("trace.spall")
    defer spall.context_destroy(&spall_ctx)

    buffer_backing := make([]u8, spall.BUFFER_DEFAULT_SIZE)
    defer delete(buffer_backing)

    spall_buffer = spall.buffer_create(buffer_backing, u32(sync.current_thread_id()))
    defer spall.buffer_destroy(&spall_ctx, &spall_buffer)
    
    context.assertion_failure_proc = spall_assert_fail
  }

  when #config(ENABLE_TRACING, false) {
    context.logger = log.create_console_logger(.Debug, {.Terminal_Color, .Short_File_Path, .Procedure})
  }

  if len(os.args) != 2 {
    panic("give a rom")
  }

  instr_dbg_file = os.open("trace.txt", os.O_WRONLY|os.O_CREATE|os.O_TRUNC) or_else panic("can't open trace")
  defer os.close(instr_dbg_file)

  rom := os.read_entire_file(os.args[1]) or_else panic("cant open rom :(")
  defer delete(rom)

  bus := bus_init(rom)
  cpu := cpu_init(&bus)

  log.infof("rom size: %v", len(rom))
  log.infof("cpu: pc=%04X", cpu.pc)

  when #config(HEADLESS, false) {
    for {
      cpu_exec_instr(&cpu)
      free_all(context.temp_allocator)
    }
  } else {
    raylib.InitWindow(640, 480, "pcPÒW")
    defer raylib.CloseWindow()

    raylib.SetTargetFPS(60)

    screen := raylib.GenImageColor(256, 224, raylib.WHITE)
    raylib.ImageFormat(&screen, .UNCOMPRESSED_R8G8B8A8)
    defer raylib.UnloadImage(screen)

    frame := 0

    for !raylib.WindowShouldClose() {
      defer frame += 1

      start := time.now()
      for !bus.vblank_occured {
        cpu_exec_instr(&cpu)
      }
      bus.vblank_occured = false
      diff := time.now()._nsec - start._nsec

      raylib.BeginDrawing()

      pixels := cast([^]u8)screen.data
      pixels_i := 0

      for v, i in bus.screen {
        pixels[i*4+0] = cast(u8)v.r*32
        pixels[i*4+1] = cast(u8)v.g*32
        pixels[i*4+2] = cast(u8)v.b*32
        pixels[i*4+3] = 0xFF
      }

      texture := raylib.LoadTextureFromImage(screen)
      defer raylib.UnloadTexture(texture)

      raylib.DrawTextureRec(texture, {0, 0, 256, 224}, {0, 0}, raylib.WHITE)
      raylib.DrawText(fmt.caprintf("FRAME: %v\nTIME: %v", frame, diff, allocator = context.temp_allocator), 0, 0, 20, raylib.RED) 
      raylib.EndDrawing()

      free_all(context.temp_allocator)
    }
  }
}