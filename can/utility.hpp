#pragma once

typedef char i8;
typedef unsigned char u8;

typedef short i16;
typedef unsigned short u16;

typedef int i32;
typedef unsigned u32;

typedef long long i64;
typedef unsigned long long u64;

static_assert(sizeof(i8) == 1 && sizeof(u8) == 1, "8-bit integer size does not match");
static_assert(sizeof(i16) == 2 && sizeof(u16) == 2, "16-bit integer size does not match");
static_assert(sizeof(i32) == 4 && sizeof(u32) == 4, "32-bit integer size does not match");
static_assert(sizeof(i64) == 8 && sizeof(u64) == 8, "64-bit integer size does not match");
