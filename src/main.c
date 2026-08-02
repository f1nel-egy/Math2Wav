#include <math2wav.h>
#include <tinyexpr.h>

#define AUDIO_PCM_SIGNED true // it HAS to be true for .wav files
#define AUDIO_PCM_WIDTH 32 // in bits
#define AUDIO_SAMPLE_RATE 48000 // in hz
#define AUDIO_DURATION 1000 // in ms

#define AUDIO_OUTPUT_FILENAME "output.wav"
#define AUDIO_FORMAT "wav"

#define f(x) (0.25 * sin(x * 440.0 * 2.0 * PI))
//#define f(x) (fmod(cos(floor(x)), sin(99 * x)))
//#define f(x) ( \
//    /* 1. Sub Kick: Pitch drop (185 Hz -> 45 Hz) + punchy decay */ \
//    (sin(2.0 * M_PI * (45.0 * fmod(x, 0.5) + 4.0 * (1.0 - exp(-fmod(x, 0.5) * 35.0)))) * exp(-fmod(x, 0.5) * 10.0) * 0.6) + \
//    /* 2. Funky Bass: 16th-note decaying synth with pitch steps */ \
//    (sin(2.0 * M_PI * (55.0 * (1.0 + (((int)(x * 4.0) * 3) % 5) * 0.25)) * x) * exp(-fmod(x, 0.25) * 12.0) * 0.3) + \
//    /* 3. Offbeat Hi-Hat: Pseudo-random noise on the 8th-note offbeats */ \
//    ((fmod(fabs(sin((x) * 12.9898) * 43758.5453), 1.0) * 2.0 - 1.0) * exp(-fmod((x) + 0.25, 0.5) * 45.0) * 0.1) \
//)

char default_output_file[] = AUDIO_OUTPUT_FILENAME;
char default_audio_format[] = AUDIO_FORMAT;
char default_formula[] = "0.25 * sin(x * 440.0 * 2.0 * 3.14159265358979323846)";

bool user_needs_help = false;
bool user_needs_version = false;
bool user_requested_custom_output_file = false; 
char *user_custom_output_file = default_output_file;
uint32_t user_custom_audio_sample_rate = AUDIO_SAMPLE_RATE;
uint32_t user_custom_audio_duration = AUDIO_DURATION;
char *user_custom_audio_format = default_audio_format;
uint32_t user_custom_audio_width = AUDIO_PCM_WIDTH;
char *user_custom_formula = default_formula;

double math_pi = M_PI;
double math_eps = EPS;
double math_exp = EXP;
double math_e = E;
double math_dbl_min = DBL_MIN;
double math_dbl_max = DBL_MAX;
double math_inf = INF;

double te_and(double a, double b)  { return (double)((uint64_t)a & (uint64_t)b); }
double te_or(double a, double b)   { return (double)((uint64_t)a | (uint64_t)b); }
double te_xor(double a, double b)  { return (double)((uint64_t)a ^ (uint64_t)b); }
double te_shl(double a, double b)  { return (double)((uint64_t)a << (uint64_t)b); }
double te_shr(double a, double b)  { return (double)((uint64_t)a >> (uint64_t)b); }
double te_not(double a)            { return (double)(~(uint64_t)a); }

double te_fabs_wrap(double a) { return fabs(a); }
double te_fmod_wrap(double a, double b) { return fmod(a, b); }
double te_clamp(double x, double min, double max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

int main(int argc, char *argv[], char *envp[]){
    int return_value = 0;
    te_expr *compiled_expr = NULL;
    
    if(argc <= 1 && argv != NULL){
        printf("Usage: %s [OPTION] -o [FILE]\n", argv[0]);
        printf("Run '%s -h' for more\n", argv[0]);
        return_value = 0;
        goto exit_main;
    }
    
    if(argc > 1 && argv != NULL)
        for(int i = 1; i < argc; i++){
            if(argv[i] == NULL){
                printf("error: at index %d into argv expected a non-NULL pointer but got a NULL pointer. argc = %d, argv = %p, envp = %p\n", i, argc, (void *)argv, (void *)envp);
                return_value = -1;
                goto exit_main;
            }
            
            if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
                user_needs_help = true;
            else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
                user_needs_version = true;
            else if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0){
                if(i + 1 < argc)
                    user_custom_output_file = argv[++i];
                else {
                    printf("error: expected filename after %s and not end of arguments\n", argv[i]);
                    return_value = -1;
                    goto exit_main;
                }
            }
            else if(strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--frequency") == 0){
                if(i + 1 < argc){
                    char *number_string_end = NULL;
                    user_custom_audio_sample_rate = strtol(argv[++i], &number_string_end, 10);
                    if(number_string_end == argv[i]){    
                        printf("error: expected frequency instead of nothing\n");
                        return_value = -1;
                        goto exit_main;
                    }
                    else if(*number_string_end != '\0'){
                        printf("error: expected end of frequency number instead of '%c'\n", *number_string_end);
                        return_value = -1;
                        goto exit_main;
                    }
                }
                else {
                    printf("error: expected frequency after %s and not end of arguments\n", argv[i]);
                    return_value = -1;
                    goto exit_main;
                }
            }
            else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--duration") == 0){
                if(i + 1 < argc){
                    char *number_string_end = NULL;
                    user_custom_audio_duration = strtol(argv[++i], &number_string_end, 10);
                    if(number_string_end == argv[i]){    
                        printf("error: expected duration instead of nothing\n");
                        return_value = -1;
                        goto exit_main;
                    }
                    else if(*number_string_end != '\0'){
                        printf("error: expected end of duration number instead of '%c'\n", *number_string_end);
                        return_value = -1;
                        goto exit_main;
                    }
                }
                else {
                    printf("error: expected duration after %s and not end of arguments\n", argv[i]);
                    return_value = -1;
                    goto exit_main;
                }
            }
            else if(strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0){
                if(i + 1 < argc){
                    char *number_string_end = NULL;
                    user_custom_audio_width = strtol(argv[++i], &number_string_end, 10);
                    if(number_string_end == argv[i]){    
                        printf("error: expected bith width instead of nothing\n");
                        return_value = -1;
                        goto exit_main;
                    }
                    else if(*number_string_end != '\0'){
                        printf("error: expected end of bit width number instead of '%c'\n", *number_string_end);
                        return_value = -1;
                        goto exit_main;
                    }
                }
                else {
                    printf("error: expected bith width after %s and not end of arguments\n", argv[i]);
                    return_value = -1;
                    goto exit_main;
                }
            }
            else if(strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--math") == 0){
                if(i + 1 < argc){
                    user_custom_formula = argv[++i];
                } else {
                    printf("error: expected math formula string after %s\n", argv[i]);
                    return_value = -1;
                    goto exit_main;
                }
            }
            else {
                printf("error: unknown argument '%s'", argv[i]);
                return_value = -1;
                goto exit_main;
            }
        }
    
    if(user_needs_help && argv != NULL){
        printf("Usage: %s [OPTION] -o [FILE]\n", argv[0]);
        printf("Create a sound file using a math formula and output it as [FILE] or output.wav\n");
        printf("example: %s -f %d -d %d -o result.wav\n", argv[0], AUDIO_SAMPLE_RATE, AUDIO_DURATION);
        printf("\nOptions:\n");
        printf("-h, --help                  Shows help\n");
        printf("-v, --version               Shows version\n");
        printf("-o, --output FILE           Changes default output file from '%s' to FILE\n", AUDIO_OUTPUT_FILENAME);
        printf("-f, --frequency FREQ        Changes default sample rate from %d hz to FREQ hz\n", AUDIO_SAMPLE_RATE);
        printf("-d, --duration DURATION     Changes default duration from %d ms to DURATION ms\n", AUDIO_DURATION);
        printf("-w, --width WIDTH           Changes default bit width from %d bits to WIDTH bits\n", AUDIO_PCM_WIDTH);
        printf("-m, --math FORMULA          Changes default formula from (0.25 * sin(x * 440.0 * 2.0 * PI)) to FORMULA\n");
        printf("\nNotes:\n");
        printf("Using unicode symbols for math formulas will lead to errors\n");
        printf("Use these symbols instead:\n");
        printf("'+' for addition\n");
        printf("'-' for subtraction\n");
        printf("'*' for multiplication or dot\n");
        printf("'/' for division\n");
        printf("'PI' for 3.14159265358979323846264338327950288\n");
        printf("'EPS' for 1e-17\n");
        printf("'EXP' for 2.7182818284590452353602874713526624\n");
        printf("'E' for 2.7182818284590452353602874713526624\n");
        printf("'DBL_MAX' for 1.7976931348623157e308\n");
        printf("'DBL_MIN' for 2.2250738585072014e-308\n");
        printf("'INF' or 'INFINITY' for infinity\n");
        printf("'NAN' for NaN (Not a Number)\n");
        printf("\nReport bugs to: ahmed.retroflexos.dev@protonmail.com\n");
        
        return_value = 0;
        goto exit_main;
    }
    
    if(user_needs_version){
        printf("Math2Wav 0.1\n");
        printf("Copyright (C) 2026 Ahmed.\nLicense GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n\nWritten by Ahmed");
        
        return_value = 0;
        goto exit_main;
    }

    FILE *audio_file = fopen(user_custom_output_file, "wb");
    if(!audio_file){
        perror("fopen");
        return_value = -1;
        goto exit_main;
    }

    fseek(audio_file, 0, SEEK_SET);

    convert_string_lower(user_custom_audio_format);

    if(strcmp(user_custom_audio_format, "wav") == 0){
        uint8_t wav_riff_signature[4] = {0x52, 0x49, 0x46, 0x46};
        uint8_t wav_wave_signature[4] = {0x57, 0x41, 0x56, 0x45};
        uint8_t wav_fmt__signature[4] = {0x66, 0x6d, 0x74, 0x20};
        uint8_t wav_data_signature[4] = {0x64, 0x61, 0x74, 0x61};
        
        uint32_t wav_format_chunk_size = AUDIO_WAV_FMT__CHUNK_SIZE - 8;

        uint16_t wav_audio_format = AUDIO_WAV_FMT__PCM_INT_TYPE;
        uint16_t wav_audio_channels = 1;
        
        uint32_t wav_bit_width = user_custom_audio_width;
        wav_bit_width = clamp(wav_bit_width, 8, 64);
        uint32_t wav_sample_rate = user_custom_audio_sample_rate;
        
        uint32_t wav_bytes_second = wav_sample_rate * wav_audio_channels * (wav_bit_width / 8);
        uint16_t wav_bytes_block = wav_audio_channels * (wav_bit_width / 8);
        
        uint32_t wav_audio_size = (((wav_bit_width / 8) * wav_sample_rate) * (user_custom_audio_duration / 1000.0)) * wav_audio_channels;
        
        uint32_t wav_bits_sample = wav_bit_width;
        
        uint32_t wav_overall_size = (AUDIO_WAV_RIFF_CHUNK_SIZE + AUDIO_WAV_FMT__CHUNK_SIZE + AUDIO_WAV_DATA_HEADER_SIZE + wav_audio_size) - 8;
        
        double x_val = 0.0;
        te_variable vars[] = {
            {"x", &x_val},

            {"PI", &math_pi},
            {"EPS", &math_eps},
            {"EXP", &math_exp},
            {"E", &math_e},
            {"DBL_MIN", &math_dbl_min},
            {"DBL_MAX", &math_dbl_max},
            {"INF", &math_inf},
            
            {"pi", &math_pi},
            {"e", &math_e},
            {"dbl_max", &math_dbl_min},
            {"dbl_min", &math_dbl_max},
            {"inf", &math_inf},
            {"infinity", &math_inf},
            
            {"M_PI", &math_pi},
            {"M_EPS", &math_eps},
            {"M_EXP", &math_exp},
            {"M_E", &math_e},
            {"M_DBL_MIN", &math_dbl_min},
            {"M_DBL_MAX", &math_dbl_max},
            {"M_INF", &math_inf},

            {"and", (const void*)te_and, TE_FUNCTION2},
            {"or",  (const void*)te_or,  TE_FUNCTION2},
            {"xor", (const void*)te_xor, TE_FUNCTION2},
            {"shl", (const void*)te_shl, TE_FUNCTION2},
            {"shr", (const void*)te_shr, TE_FUNCTION2},
            {"not", (const void*)te_not, TE_FUNCTION1},

            {"fabs",  (const void*)te_fabs_wrap, TE_FUNCTION1},
            {"fmod",  (const void*)te_fmod_wrap, TE_FUNCTION2},
            {"clamp", (const void*)te_clamp,     TE_FUNCTION3}
        };

        int var_count = sizeof(vars) / sizeof(vars[0]);

        int err_pos = 0;
        compiled_expr = te_compile(user_custom_formula, vars, var_count, &err_pos);
        if(!compiled_expr){
            printf("error: invalid math formula syntax near character %d\n", err_pos);
            printf("formula: %s\n", user_custom_formula);
            printf("         %*s^\n", err_pos - 1, "");
            return_value = -1;
            goto exit_file;
        }

        uint8_t *audio_pcm_data = malloc(wav_audio_size);
        if(audio_pcm_data == NULL){
            printf("failed to allocate %d for WAV PCM data\n", wav_audio_size);
            return_value = -1;
            goto exit_file;
        }
        for(uint64_t t = 0; t < wav_audio_size / (wav_bit_width / 8); t++){
            x_val = (double)t / wav_sample_rate;

            long double y = (long double)te_eval(compiled_expr);
            y = clamp(y, -1.0, 1.0);

            uint64_t i = t * (wav_bit_width / 8);
            switch(wav_bit_width){
                case 8:
                    uint8_t pcm_value8 = (uint8_t)((y * 0.5 + 0.5) * (long double)CALCULATE_MAX_VALUE(wav_bit_width, AUDIO_PCM_SIGNED));
                    audio_pcm_data[i] = (uint8_t)(pcm_value8 & 0xff);
                    break;
                case 16:
                    uint16_t pcm_value16 = (uint16_t)(y * (long double)CALCULATE_MAX_VALUE(wav_bit_width, AUDIO_PCM_SIGNED));
                    audio_pcm_data[i++] = (uint8_t)(pcm_value16 & 0xff);
                    audio_pcm_data[i] = (uint8_t)((pcm_value16 & 0xff00) >> 8);
                    break;
                case 32:
                    uint32_t pcm_value32 = (uint32_t)(y * (long double)CALCULATE_MAX_VALUE(wav_bit_width, AUDIO_PCM_SIGNED));
                    audio_pcm_data[i++] = (uint8_t)(pcm_value32 & 0xff);
                    audio_pcm_data[i++] = (uint8_t)((pcm_value32 & 0xff00) >> 8);
                    audio_pcm_data[i++] = (uint8_t)((pcm_value32 & 0xff0000) >> 16);
                    audio_pcm_data[i] = (uint8_t)((pcm_value32 & 0xff000000) >> 24);
                    break;
                case 64:
                    uint64_t pcm_value64 = (uint64_t)(y * (long double)CALCULATE_MAX_VALUE(wav_bit_width, AUDIO_PCM_SIGNED));
                    audio_pcm_data[i++] = (uint8_t)(pcm_value64 & 0xff);
                    audio_pcm_data[i++] = (uint8_t)((pcm_value64 & 0xff00) >> 8);
                    audio_pcm_data[i++] = (uint8_t)((pcm_value64 & 0xff0000) >> 16);
                    audio_pcm_data[i++] = (uint8_t)((pcm_value64 & 0xff000000) >> 24);
                    audio_pcm_data[i++] = (uint8_t)((pcm_value64 & 0xff00000000) >> 32);
                    audio_pcm_data[i++] = (uint8_t)((pcm_value64 & 0xff0000000000) >> 40);
                    audio_pcm_data[i++] = (uint8_t)((pcm_value64 & 0xff000000000000) >> 48);
                    audio_pcm_data[i] = (uint8_t)((pcm_value64 & 0xff00000000000000) >> 56);
                    break;
                default:
                    printf("error: unsupported bit width %d\n", wav_bit_width);
                    free(audio_pcm_data);
                    return_value = -1;
                    goto exit_tinyexpr;
            }
        }

        FILE_WRITE_8(wav_riff_signature, 4, audio_file); // RIFF master chunk signature
        FILE_WRITE_32LE(&wav_overall_size, 1, audio_file); // RIFF master chunk overall size - 8
        FILE_WRITE_8(wav_wave_signature, 4, audio_file); // RIFF master chunk format

        FILE_WRITE_8(wav_fmt__signature, 4, audio_file); // WAVE data format chunk
        
        FILE_WRITE_32LE(&wav_format_chunk_size, 1, audio_file);
        
        FILE_WRITE_16LE(&wav_audio_format, 1, audio_file);
        FILE_WRITE_16LE(&wav_audio_channels, 1, audio_file);
        
        FILE_WRITE_32LE(&wav_sample_rate, 1, audio_file);
        FILE_WRITE_32LE(&wav_bytes_second, 1, audio_file);
        FILE_WRITE_16LE(&wav_bytes_block, 1, audio_file);
        FILE_WRITE_16LE(&wav_bits_sample, 1, audio_file);

        FILE_WRITE_8(wav_data_signature, 4, audio_file);
        FILE_WRITE_32LE(&wav_audio_size, 1, audio_file);
        FILE_WRITE_8((uint8_t*)audio_pcm_data, wav_audio_size, audio_file);

        free(audio_pcm_data);
    }

exit_tinyexpr:
    te_free(compiled_expr);
exit_file:
    fclose(audio_file);
exit_main:
    return return_value;
}
