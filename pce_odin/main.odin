#+feature dynamic-literals
package main

import "core:os"
import "core:log"
import "core:time"
import "core:fmt"
import "core:mem"
import "core:io"
import "base:runtime"
import "core:sync"
import "core:prof/spall"
import "vendor:raylib"
 
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

ENABLE_TRACING_AFTER_FRAME :: 1000

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

  instr_dbg_file = os.open("trace.txt", os.O_WRONLY|os.O_TRUNC|os.O_CREATE) or_else panic("can't open trace")
  defer os.close(instr_dbg_file)

  rom := os.read_entire_file(os.args[1]) or_else panic("cant open rom :(")
  defer delete(rom)

  bus := bus_init(rom)
  cpu := cpu_init(&bus)

  fmt.printf("rom size: %x", len(rom))
  log.infof("cpu: pc=%04X", cpu.pc)

  when #config(HEADLESS, false) {
    for {
      cpu_exec_instr(&cpu)
      free_all(context.temp_allocator)
    }
  } else {
    raylib.SetTraceLogLevel(.ERROR)

    raylib.InitWindow(256*3+520, 224*3, "pcPÒW")
    defer raylib.CloseWindow()

    mem_scroll := 0
    font := raylib.LoadFontEx("C:\\WINDOWS\\FONTS\\LUCON.TTF", 20, nil, 0)
    if !raylib.IsFontValid(font) {
      font = raylib.GetFontDefault()
    }

    raylib.SetTargetFPS(60)

    screen := raylib.GenImageColor(256, 224, raylib.WHITE)
    raylib.ImageFormat(&screen, .UNCOMPRESSED_R8G8B8A8)
    defer raylib.UnloadImage(screen)

    frame := 0

    for !raylib.WindowShouldClose() {
      if frame == ENABLE_TRACING_AFTER_FRAME {
        is_tracing_enabled = true
      }

      defer frame += 1

      bus.joy.joypad.a = raylib.IsKeyDown(.X)
      bus.joy.joypad.b = raylib.IsKeyDown(.Z)
      bus.joy.joypad.sel = raylib.IsKeyDown(.RIGHT_SHIFT)
      bus.joy.joypad.run = raylib.IsKeyDown(.ENTER)
      bus.joy.joypad.up = raylib.IsKeyDown(.UP)
      bus.joy.joypad.down = raylib.IsKeyDown(.DOWN)
      bus.joy.joypad.left = raylib.IsKeyDown(.LEFT)
      bus.joy.joypad.right = raylib.IsKeyDown(.RIGHT)

      start := time.now()
      for !bus.vblank_occured {
        cpu_exec_instr(&cpu)
      }
      bus.vblank_occured = false
      diff := time.now()._nsec - start._nsec

      raylib.BeginDrawing()
      raylib.ClearBackground(raylib.BLACK)

      pixels := cast([^]u8)screen.data
      pixels_i := 0

      for v, i in bus.screen {
        pixels[i*4+0] = cast(u8)(v.r*32)
        pixels[i*4+1] = cast(u8)(v.g*32)
        pixels[i*4+2] = cast(u8)(v.b*32)
        pixels[i*4+3] = 0xFF
      }

      texture := raylib.LoadTextureFromImage(screen)
      defer raylib.UnloadTexture(texture)

      raylib.DrawTextureEx(texture, {520, 0}, 0, 3, raylib.WHITE)
      raylib.DrawTextEx(
        font,
        fmt.caprintf("FPS: %v\nFRAME: %v\nTIME: %v", raylib.GetFPS(), frame, diff, allocator = context.temp_allocator),
        {0, 0},
        20,
        0,
        raylib.RED,
      ) 

      start_x := 10
      start_y := 200

      mem_scroll -= cast(int)(raylib.GetMouseWheelMove()) * (raylib.IsKeyDown(.LEFT_SHIFT) ? 8 : 0)

      for i in 0..<16 {
        offs := (i+mem_scroll)*8

        raylib.DrawTextEx(
          font, 
          fmt.caprintf("%04X", offs, allocator = context.temp_allocator),
          {f32(start_x), f32(i*20+start_y)},
          20,
          0,
          raylib.GRAY,
        )

        for x in 0..<8 {
          raylib.DrawTextEx(
            font, 
            fmt.caprintf("%04X", bus.vdc.vram.vram[offs+x], allocator = context.temp_allocator),
            {f32(x*55+start_x+55), f32(i*20+start_y)},
            20,
            0,
            raylib.BLUE,
          )
        }
      }

      if raylib.IsKeyDown(.ONE) {
        for i in 0..<0x200 {
          pixels: [4]u8
          v := bus.vce.pal[i]
          pixels[0] = cast(u8)(v.r*32)
          pixels[1] = cast(u8)(v.g*32)
          pixels[2] = cast(u8)(v.b*32)
          pixels[3] = 0xFF

          w, h := 5, 5
          x := i%16*5
          y := i/16*5

          raylib.DrawRectangleRec({cast(f32)x, cast(f32)y, cast(f32)w, cast(f32)h}, raylib.Color(pixels))
        }
      }

      if raylib.IsKeyDown(.TWO) {
        for i in 0..<uint(64) {
          sprite := vram_get_sprite(&bus.vdc.vram, i)^
          w, h := vram_sprite_dims(sprite)

          raylib.DrawRectangleLinesEx(
            {
              cast(f32)sprite.x*3-32*3+520,
              cast(f32)sprite.y*3-64*3,
              cast(f32)w*3,
              cast(f32)h*3,
            },
            3,
            raylib.WHITE,
          )

          raylib.DrawRectangleLinesEx(
            {
              cast(f32)sprite.x*3-32*3+520,
              cast(f32)sprite.y*3-64*3,
              cast(f32)w*3,
              cast(f32)h*3,
            },
            1,
            raylib.BLACK,
          )
        }
      }



      raylib.EndDrawing()

      free_all(context.temp_allocator)
    }
  }
}