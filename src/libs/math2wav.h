#pragma once

#include <unistd.h>

#include <math.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define PI 3.14159265358979323846264338327950288
#define EPS 1e-17
#define EXP 2.7182818284590452353602874713526624
#define E 2.71828182845904523536028747135266249
#define DBL_MAX 1.7976931348623157e308L
#define DBL_MIN 2.2250738585072014e-308L
#define INF (1.0 / 0.0)

/*#define M_PI PI
#define M_EPS EPS
#define M_EXP EXP
#define M_E E
#define M_DBL_MAX DBL_MAX
#define M_DBL_MIN DBL_MIN
#define M_INF INF*/

#define CALCULATE_BITMASK(width) ((width) == 64 ? 0xffffffffffffffffULL : ((1ULL << width) - 1))
#define CALCULATE_MAX_VALUE(width, reserved_bits) ((width - reserved_bits) == 64 ? 0xffffffffffffffffULL : ((1ULL << (width - reserved_bits)) - 1))

#define FILE_WRITE_64LE(value_address, count, file) fwrite(value_address, sizeof(uint64_t), count, file)
#define FILE_WRITE_32LE(value_address, count, file) fwrite(value_address, sizeof(uint32_t), count, file)
#define FILE_WRITE_16LE(value_address, count, file) fwrite(value_address, sizeof(uint16_t), count, file)
#define FILE_WRITE_8(value_address, count, file) fwrite(value_address, sizeof(uint8_t), count, file)
#define FILE_WRITE_STRING(string_address, file) fwrite(string_address, sizeof(char), strlen(string_address), file)

#define AUDIO_WAV_RIFF_CHUNK_SIZE 12
#define AUDIO_WAV_FMT__CHUNK_SIZE 24
#define AUDIO_WAV_DATA_HEADER_SIZE 8
#define AUDIO_WAV_FMT__PCM_INT_TYPE 1
#define AUDIO_WAV_FMT__IEEE_754_FLOAT_TYPE 3

void convert_string_lower(char *string);
void big_endian_to_little(void *big_endian_value, uint32_t size_bytes);

double m2w_fmod(double a, double b);
double m2w_min(double a, double b);
double m2w_max(double a, double b);
double m2w_clamp(double value, double low, double high);
double m2w_sign(double a);
double m2w_saw(double phase);
double m2w_tri(double phase);
double m2w_sqr(double phase);
double m2w_noise(double seed);

uint64_t u64clampu64u64u64(uint64_t value, uint64_t min, uint64_t max);
uint32_t u32clampu32u32u32(uint32_t value, uint32_t min, uint32_t max);
uint16_t u16clampu16u16u16(uint16_t value, uint16_t min, uint16_t max);
uint8_t u8clampu8u8u8(uint8_t value, uint8_t min, uint8_t max);
int64_t i64clampi64i64i64(int64_t value, int64_t min, int64_t max);
int32_t i32clampi32i32i32(int32_t value, int32_t min, int32_t max);
int16_t i16clampi16i16i16(int16_t value, int16_t min, int16_t max);
int8_t i8clampi8i8i8(int8_t value, int8_t min, int8_t max);
char cclampccc(char value, char min, char max);
float fclampfff(float value, float min, float max);
double dclampddd(double value, double min, double max);
long double ldclampldldld(long double value, long double min, long double max); 

#define clamp(value, min, max) _Generic(value, \
    uint64_t: u64clampu64u64u64(value, min, max), \
    uint32_t: u32clampu32u32u32(value, min, max), \
    uint16_t: u16clampu16u16u16(value, min, max), \
    uint8_t: u8clampu8u8u8(value, min, max), \
    int64_t: i64clampi64i64i64(value, min, max), \
    int32_t: i32clampi32i32i32(value, min, max), \
    int16_t: i16clampi16i16i16(value, min, max), \
    int8_t: i8clampi8i8i8(value, min, max), \
    char: cclampccc(value, min, max), \
    float: fclampfff(value, min, max), \
    double: dclampddd(value, min, max), \
    long double: ldclampldldld(value, min, max))

// Like Tsoding said "whoever in the ISO C committee thought it's a good idea to call this feature generics, you should be ashamed of yourself"
