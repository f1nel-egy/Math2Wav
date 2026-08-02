#include <math2wav.h>
#include <ssf.h>

#define AUDIO_PCM_SIGNED true // it HAS to be true for .wav files

char default_output_file[] = "output.wav";

bool user_needs_help = false;
bool user_needs_version = false;
char *user_output_file = default_output_file;

bool set_rate = false;
bool set_duration = false;
bool set_width = false;
bool set_math = false;
uint32_t cli_sample_rate = 0;
uint32_t cli_duration = 0;
uint32_t cli_width = 0;
char *cli_math = NULL;
char *input_ssf_file = NULL;

static int parse_uint_arg(const char *value, uint32_t *out, const char *what){
    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if(end == value){
        printf("error: expected %s instead of nothing\n", what);
        return -1;
    }
    if(*end != '\0'){
        printf("error: expected end of %s number instead of '%c'\n", what, *end);
        return -1;
    }
    *out = (uint32_t)parsed;
    return 0;
}

static void print_help(const char *program){
    printf("Usage: %s [OPTION]... [FILE.ssf]\n", program);
    printf("Render a sound from a math formula and write it to a WAV file.\n");
    printf("The formula can come from a Sound Source File (.ssf), the -m option,\n");
    printf("or, if neither is given, a built-in default tone.\n");
    printf("\nexamples:\n");
    printf("  %s song.ssf -o song.wav\n", program);
    printf("  %s -m \"0.25 * sin(2 * pi() * 440 * x)\" -d 2000 -o beep.wav\n", program);
    printf("\nOptions:\n");
    printf("-h, --help                  Shows help\n");
    printf("-v, --version               Shows version\n");
    printf("-o, --output FILE           Output WAV file (default '%s')\n", default_output_file);
    printf("-i, --input FILE            Read settings and formula from an .ssf file\n");
    printf("-m, --math FORMULA          Use FORMULA directly (overrides the .ssf formula)\n");
    printf("-f, --frequency FREQ        Sample rate in Hz\n");
    printf("-d, --duration DURATION     Duration in ms\n");
    printf("-w, --width WIDTH           PCM bit width in bits (8, 16, 32 or 64)\n");
    printf("\nFormula notes:\n");
    printf("The variable 'x' is the time in seconds. The result should stay in\n");
    printf("[-1, 1]; anything outside is clamped. Supported symbols include:\n");
    printf("  + - * / ^ %%            arithmetic ('%%' is fmod, '^' is power)\n");
    printf("  sin cos tan  exp ln log log10  sqrt abs floor ceil\n");
    printf("  pi() e()               constants\n");
    printf("  fmod(a,b) min(a,b) max(a,b) clamp(v,lo,hi) sign(a)\n");
    printf("  saw(p) tri(p) sqr(p)   band-unlimited oscillators, one cycle per unit\n");
    printf("  noise(seed)            deterministic white noise in [-1, 1]\n");
    printf("\nSSF files can also 'import \"other.ssf\"', define 'const NAME = ...'\n");
    printf("constants, and 'fn name(args) = ...' functions that formulas call.\n");
    printf("\nReport bugs to: ahmed.retroflexos.dev@protonmail.com\n");
}

static void print_version(void){
    printf("Math2Wav 6.7\n");
    printf("Copyright (C) 2026 Ahmed & Simon.\nLicense GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n\nWritten by Ahmed\n");
}

int main(int argc, char *argv[], char *envp[]){
    int return_value = 0;

    if(argc <= 1 && argv != NULL){
        printf("Usage: %s [OPTION]... [FILE.ssf]\n", argv[0]);
        printf("Run '%s -h' for more\n", argv[0]);
        return 0;
    }

    if(argc > 1 && argv != NULL)
        for(int i = 1; i < argc; i++){
            if(argv[i] == NULL){
                printf("error: at index %d into argv expected a non-NULL pointer but got a NULL pointer. argc = %d, argv = %p, envp = %p\n", i, argc, (void *)argv, (void *)envp);
                return -1;
            }

            if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
                user_needs_help = true;
            else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
                user_needs_version = true;
            else if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0){
                if(i + 1 < argc)
                    user_output_file = argv[++i];
                else {
                    printf("error: expected filename after %s and not end of arguments\n", argv[i]);
                    return -1;
                }
            }
            else if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0){
                if(i + 1 < argc)
                    input_ssf_file = argv[++i];
                else {
                    printf("error: expected filename after %s and not end of arguments\n", argv[i]);
                    return -1;
                }
            }
            else if(strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--math") == 0){
                if(i + 1 < argc){
                    cli_math = argv[++i];
                    set_math = true;
                }
                else {
                    printf("error: expected formula after %s and not end of arguments\n", argv[i]);
                    return -1;
                }
            }
            else if(strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--frequency") == 0){
                if(i + 1 >= argc){
                    printf("error: expected frequency after %s and not end of arguments\n", argv[i]);
                    return -1;
                }
                if(parse_uint_arg(argv[++i], &cli_sample_rate, "frequency") != 0)
                    return -1;
                set_rate = true;
            }
            else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--duration") == 0){
                if(i + 1 >= argc){
                    printf("error: expected duration after %s and not end of arguments\n", argv[i]);
                    return -1;
                }
                if(parse_uint_arg(argv[++i], &cli_duration, "duration") != 0)
                    return -1;
                set_duration = true;
            }
            else if(strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0){
                if(i + 1 >= argc){
                    printf("error: expected bit width after %s and not end of arguments\n", argv[i]);
                    return -1;
                }
                if(parse_uint_arg(argv[++i], &cli_width, "bit width") != 0)
                    return -1;
                set_width = true;
            }
            else if(argv[i][0] != '-'){
                input_ssf_file = argv[i];
            }
            else {
                printf("error: unknown argument '%s'\n", argv[i]);
                return -1;
            }
        }

    if(user_needs_help && argv != NULL){
        print_help(argv[0]);
        return 0;
    }

    if(user_needs_version){
        print_version();
        return 0;
    }

    ssf_program *program = ssf_program_new();
    if(program == NULL){
        printf("error: out of memory\n");
        return -1;
    }

    if(input_ssf_file != NULL && ssf_program_parse_file(program, input_ssf_file) != 0){
        ssf_program_free(program);
        return -1;
    }

    if(set_math && ssf_program_set_main_expr(program, cli_math) != 0){
        ssf_program_free(program);
        return -1;
    }

    ssf_config config;
    ssf_program_get_config(program, &config);
    if(set_rate)
        config.sample_rate = cli_sample_rate;
    if(set_duration)
        config.duration_ms = cli_duration;
    if(set_width)
        config.width = cli_width;
    ssf_program_set_config(program, &config);

    if(config.sample_rate == 0){
        printf("error: sample rate must be greater than 0\n");
        ssf_program_free(program);
        return -1;
    }

    if(ssf_program_compile(program) != 0){
        ssf_program_free(program);
        return -1;
    }

    convert_string_lower(config.format);

    if(strcmp(config.format, "wav") != 0){
        printf("error: unsupported output format '%s' (only 'wav' is supported)\n", config.format);
        ssf_program_free(program);
        return -1;
    }

    FILE *audio_file = fopen(user_output_file, "wb");
    if(!audio_file){
        perror("fopen");
        ssf_program_free(program);
        return -1;
    }

    uint8_t wav_riff_signature[4] = {0x52, 0x49, 0x46, 0x46};
    uint8_t wav_wave_signature[4] = {0x57, 0x41, 0x56, 0x45};
    uint8_t wav_fmt__signature[4] = {0x66, 0x6d, 0x74, 0x20};
    uint8_t wav_data_signature[4] = {0x64, 0x61, 0x74, 0x61};

    uint32_t wav_format_chunk_size = AUDIO_WAV_FMT__CHUNK_SIZE - 8;

    uint16_t wav_audio_format = AUDIO_WAV_FMT__PCM_INT_TYPE;
    uint16_t wav_audio_channels = 1;

    uint32_t wav_bit_width = config.width;
    wav_bit_width = clamp(wav_bit_width, 8, 64);
    uint32_t wav_sample_rate = config.sample_rate;

    uint32_t wav_bytes_second = wav_sample_rate * wav_audio_channels * (wav_bit_width / 8);
    uint16_t wav_bytes_block = wav_audio_channels * (wav_bit_width / 8);

    uint32_t wav_audio_size = (((wav_bit_width / 8) * wav_sample_rate) * (config.duration_ms / 1000.0)) * wav_audio_channels;

    uint32_t wav_bits_sample = wav_bit_width;

    uint32_t wav_overall_size = (AUDIO_WAV_RIFF_CHUNK_SIZE + AUDIO_WAV_FMT__CHUNK_SIZE + AUDIO_WAV_DATA_HEADER_SIZE + wav_audio_size) - 8;

    uint8_t *audio_pcm_data = malloc(wav_audio_size);
    if(audio_pcm_data == NULL){
        printf("failed to allocate %d for WAV PCM data\n", wav_audio_size);
        return_value = -1;
        goto exit_file;
    }
    for(uint64_t t = 0; t < wav_audio_size / (wav_bit_width / 8); t++){
        double x = (double)t / wav_sample_rate;
        long double y = (long double)ssf_program_eval(program, x);
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
                goto exit_file;
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

    printf("wrote %s: %s, %u Hz, %u-bit, %u ms\n",
           user_output_file, config.format, wav_sample_rate, wav_bit_width, config.duration_ms);

exit_file:
    fclose(audio_file);
    ssf_program_free(program);
    return return_value;
}
