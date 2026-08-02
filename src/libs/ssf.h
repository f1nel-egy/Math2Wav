#pragma once

/* SSF - Sound Source File: a tiny math language interpreted at runtime. */

#include <stdint.h>
#include <stdbool.h>

#define SSF_FORMAT_MAX 16

typedef struct ssf_config {
    uint32_t sample_rate;
    uint32_t duration_ms;
    uint32_t width;
    char format[SSF_FORMAT_MAX];
} ssf_config;

typedef struct ssf_program ssf_program;

ssf_program *ssf_program_new(void);
int ssf_program_parse_file(ssf_program *program, const char *path);
int ssf_program_set_main_expr(ssf_program *program, const char *expression);
void ssf_program_get_config(const ssf_program *program, ssf_config *out);
void ssf_program_set_config(ssf_program *program, const ssf_config *config);
int ssf_program_compile(ssf_program *program);
double ssf_program_eval(const ssf_program *program, double x);
void ssf_program_free(ssf_program *program);
