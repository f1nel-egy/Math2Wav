#include "math2wav.h"

void convert_string_lower(char *string){
    for(long unsigned int i = 0; i < strlen(string); i++)
        string[i] = tolower(string[i]);
}

void big_endian_to_little(void *big_endian_value, uint32_t size_bytes){
    uint8_t byte_save = 0;
    uint8_t *big_endian = (uint8_t*)big_endian_value;

    size_bytes--;
    for(uint32_t i = 0; i < (size_bytes + 1) / 2; i++){
        byte_save = big_endian[i];
        big_endian[i] = big_endian[size_bytes - i];
        big_endian[size_bytes - i] = byte_save;
    }
}

uint64_t u64clampu64u64u64(uint64_t value, uint64_t min, uint64_t max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

uint32_t u32clampu32u32u32(uint32_t value, uint32_t min, uint32_t max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

uint16_t u16clampu16u16u16(uint16_t value, uint16_t min, uint16_t max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

uint8_t u8clampu8u8u8(uint8_t value, uint8_t min, uint8_t max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

int64_t i64clampi64i64i64(int64_t value, int64_t min, int64_t max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

int32_t i32clampi32i32i32(int32_t value, int32_t min, int32_t max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

int16_t i16clampi16i16i16(int16_t value, int16_t min, int16_t max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

int8_t i8clampi8i8i8(int8_t value, int8_t min, int8_t max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

char cclampccc(char value, char min, char max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

float fclampfff(float value, float min, float max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

double dclampddd(double value, double min, double max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}

long double ldclampldldld(long double value, long double min, long double max){ 
    if(value > max)
        value = max;
    else if(value < min)
        value = min;
    return value;
}
