package main

import "core:testing"

// NOTE: only digit (low 4) bits are added, carry goes into bit 4
bcd_add_digit :: proc(a: u8, b: u8, carry: bool = false) -> u8 {
  result := a&0xF + b&0xF + cast(u8)carry&0x1
  if result >= 10 {
    result -= 10
    result |= 0x10
  }

  return result
}

// NOTE: only digit (low 4) bits are subtracted, carry goes into bit 4
bcd_sub_digit :: proc(a: u8, b: u8, carry: bool = true) -> u8 {
  result := a&0xF - b&0xF + (cast(u8)carry&0x1-1)
  if result & 0x80 > 0 {
  	result = (result-6)&0xF
    result |= 0x10
  }

  return result
}

bcd_add :: proc(a: u8, b: u8, carry: bool = false) -> (u8, bool) {
  lo := bcd_add_digit(a, b, carry)
  hi := bcd_add_digit(a>>4, b>>4, (lo & 0x10) > 0)

  return (lo&0xF)|(hi<<4), hi & 0x10 != 0
}

bcd_sub :: proc(a, b: u8, carry: bool = true) -> (u8, bool) {
  lo := bcd_sub_digit(a, b, carry)
  hi := bcd_sub_digit(a>>4, b>>4, (lo & 0x10) == 0)

  return (lo&0xF)|(hi<<4), hi & 0x10 == 0
}

@(test)
test_bcd_add :: proc(t: ^testing.T) {
	bcd_add_chk :: proc(t: ^testing.T, a: u8, b: u8, carry: bool, res: u8, car: bool) {
		val, carry := bcd_add(a, b, carry)
		testing.expectf(t, val == res, "expected %02X got %02X", res, val)
		testing.expectf(t, carry == car, "expected %v got %v", car, carry)
	}
	
	testing.expect(t, bcd_add_digit(0x1, 0x2) == 0x03)
  testing.expect(t, bcd_add_digit(0x9, 0x3) == 0x12)
  testing.expect(t, bcd_add_digit(0x9, 0x9) == 0x18)
  testing.expect(t, bcd_add_digit(0x9, 0x9, true) == 0x19)
  testing.expect(t, bcd_add_digit(0x5, 0x5, false) == 0x10)
  testing.expect(t, bcd_add_digit(0x5, 0x4, true) == 0x10)
  
  bcd_add_chk(t, 0x10, 0x20, false, 0x30, false)
  bcd_add_chk(t, 0x99, 0x99, false, 0x98, true)
  bcd_add_chk(t, 0x99, 0x99, true, 0x99, true)
  bcd_add_chk(t, 0x12, 0x67, false, 0x79, false)
  bcd_add_chk(t, 0x12, 0x69, false, 0x81, false)
  bcd_add_chk(t, 0x12, 0x69, true, 0x82, false)
  bcd_add_chk(t, 0x50, 0x50, false, 0x00, true)
  bcd_add_chk(t, 0x50, 0x50, true, 0x01, true)
  bcd_add_chk(t, 0x05, 0x05, false, 0x10, false)
  bcd_add_chk(t, 0x11, 0x11, false, 0x22, false)
}

@(test)
test_bcd_sub :: proc(t: ^testing.T) {
	bcd_sub_chk :: proc(t: ^testing.T, a: u8, b: u8, carry: bool, res: u8, car: bool, loc := #caller_location) {
		val, carry := bcd_sub(a, b, carry)
		testing.expectf(t, val == res, "expected %02X got %02X", res, val, loc=loc)
		testing.expectf(t, carry == car, "expected %v got %v", car, carry, loc=loc)
	}
	
	testing.expect(t, bcd_sub_digit(0x0, 0x1, true) == 0x19)
	testing.expect(t, bcd_sub_digit(0x9, 0x4, true) == 0x5)
	testing.expect(t, bcd_sub_digit(0x9, 0x4, false) == 0x4)
	testing.expect(t, bcd_sub_digit(0x1, 0x1, false) == 0x19)
	testing.expect(t, bcd_sub_digit(0x0, 0x0, false) == 0x19)
	testing.expect(t, bcd_sub_digit(0x9, 0x9, true) == 0x00)
	testing.expect(t, bcd_sub_digit(0x1, 0x4, true) == 0x17)
	testing.expect(t, bcd_sub_digit(0x0, 0x9, true) == 0x11)
	
	bcd_sub_chk(t, 0x01, 0x00, true, 0x01, true)
	bcd_sub_chk(t, 0x10, 0x05, true, 0x05, true)
	bcd_sub_chk(t, 0x00, 0x01, true, 0x99, false)
	bcd_sub_chk(t, 0x00, 0x02, true, 0x98, false)
	bcd_sub_chk(t, 0x00, 0x09, true, 0x91, false)
	bcd_sub_chk(t, 0x00, 0x29, true, 0x71, false)
	bcd_sub_chk(t, 0x00, 0x19, true, 0x81, false)
	bcd_sub_chk(t, 0x00, 0x01, false, 0x98, false)
	bcd_sub_chk(t, 0x00, 0x02, false, 0x97, false)
	bcd_sub_chk(t, 0x00, 0x09, false, 0x90, false)
	bcd_sub_chk(t, 0x00, 0x29, false, 0x70, false)
	bcd_sub_chk(t, 0x00, 0x19, false, 0x80, false)
	bcd_sub_chk(t, 0x99, 0x19, true, 0x80, true)
	bcd_sub_chk(t, 0x99, 0x19, false, 0x79, true)
}
