#include "ssf.h"
#include "math2wav.h"
#include "../thirdparty/tinyexpr.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>

#define SSF_MAX_NAME 64
#define SSF_MAX_PARAMS 7
#define SSF_MAX_FUNCS 256
#define SSF_MAX_CONSTS 512
#define SSF_MAX_IMPORTS 256
#define SSF_STMT_MAX 8192

#define SSF_DEFAULT_SAMPLE_RATE 48000
#define SSF_DEFAULT_DURATION_MS 1000
#define SSF_DEFAULT_WIDTH 32
#define SSF_DEFAULT_FORMAT "wav"
#define SSF_DEFAULT_MAIN "0.25 * sin(2 * pi() * 440 * x)"

typedef struct ssf_func {
    char name[SSF_MAX_NAME];
    int arity;
    char param_names[SSF_MAX_PARAMS][SSF_MAX_NAME];
    double param_vals[SSF_MAX_PARAMS];
    char *body_src;
    te_expr *body;
} ssf_func;

typedef struct ssf_const {
    char name[SSF_MAX_NAME];
    double value;
    char *expr_src;
    te_expr *expr;
} ssf_const;

struct ssf_program {
    ssf_config config;

    ssf_func funcs[SSF_MAX_FUNCS];
    int func_count;

    ssf_const consts[SSF_MAX_CONSTS];
    int const_count;

    char imports[SSF_MAX_IMPORTS][PATH_MAX];
    int import_count;

    int main_index;
    bool compiled;
};

/* Guards against runaway recursion: returns NaN instead of overflowing. */
#define SSF_MAX_CALL_DEPTH 512
static int ssf_call_depth = 0;

static double ssf_invoke(ssf_func *f, const double *args){
    if(ssf_call_depth >= SSF_MAX_CALL_DEPTH)
        return NAN;

    double saved[SSF_MAX_PARAMS];
    for(int i = 0; i < f->arity; i++){
        saved[i] = f->param_vals[i];
        f->param_vals[i] = args[i];
    }

    ssf_call_depth++;
    double r = te_eval(f->body);
    ssf_call_depth--;

    for(int i = 0; i < f->arity; i++)
        f->param_vals[i] = saved[i];
    return r;
}

static double ssf_tramp0(void *c){ return ssf_invoke(c, NULL); }
static double ssf_tramp1(void *c, double a0){ double a[] = {a0}; return ssf_invoke(c, a); }
static double ssf_tramp2(void *c, double a0, double a1){ double a[] = {a0, a1}; return ssf_invoke(c, a); }
static double ssf_tramp3(void *c, double a0, double a1, double a2){ double a[] = {a0, a1, a2}; return ssf_invoke(c, a); }
static double ssf_tramp4(void *c, double a0, double a1, double a2, double a3){ double a[] = {a0, a1, a2, a3}; return ssf_invoke(c, a); }
static double ssf_tramp5(void *c, double a0, double a1, double a2, double a3, double a4){ double a[] = {a0, a1, a2, a3, a4}; return ssf_invoke(c, a); }
static double ssf_tramp6(void *c, double a0, double a1, double a2, double a3, double a4, double a5){ double a[] = {a0, a1, a2, a3, a4, a5}; return ssf_invoke(c, a); }
static double ssf_tramp7(void *c, double a0, double a1, double a2, double a3, double a4, double a5, double a6){ double a[] = {a0, a1, a2, a3, a4, a5, a6}; return ssf_invoke(c, a); }

static const void *ssf_trampolines[SSF_MAX_PARAMS + 1] = {
    ssf_tramp0, ssf_tramp1, ssf_tramp2, ssf_tramp3,
    ssf_tramp4, ssf_tramp5, ssf_tramp6, ssf_tramp7,
};

static char *ssf_trim(char *s){
    while(*s != '\0' && isspace((unsigned char)*s))
        s++;
    char *end = s + strlen(s);
    while(end > s && isspace((unsigned char)end[-1]))
        end--;
    *end = '\0';
    return s;
}

static bool ssf_is_ident(const char *s){
    if(s == NULL || *s == '\0')
        return false;
    if(!(isalpha((unsigned char)*s) || *s == '_'))
        return false;
    for(const char *p = s + 1; *p != '\0'; p++)
        if(!(isalnum((unsigned char)*p) || *p == '_'))
            return false;
    return true;
}

static bool ssf_starts_word(const char *s, const char *word){
    size_t n = strlen(word);
    if(strncmp(s, word, n) != 0)
        return false;
    char after = s[n];
    return after == ' ' || after == '\t' || after == '(';
}

static bool ssf_name_taken(const ssf_program *p, const char *name){
    for(int i = 0; i < p->func_count; i++)
        if(strcmp(p->funcs[i].name, name) == 0)
            return true;
    for(int i = 0; i < p->const_count; i++)
        if(strcmp(p->consts[i].name, name) == 0)
            return true;
    return false;
}

static char *ssf_read_file(const char *path){
    FILE *file = fopen(path, "rb");
    if(file == NULL){
        fprintf(stderr, "ssf: cannot open '%s': ", path);
        perror(NULL);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    if(size < 0){
        fclose(file);
        return NULL;
    }
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc((size_t)size + 1);
    if(buffer == NULL){
        fclose(file);
        return NULL;
    }
    size_t read = fread(buffer, 1, (size_t)size, file);
    buffer[read] = '\0';
    fclose(file);
    return buffer;
}

static void ssf_strip_comments(char *s){
    for(char *p = s; *p != '\0'; p++){
        bool is_comment = (*p == '#') || (p[0] == '/' && p[1] == '/');
        if(is_comment)
            while(*p != '\0' && *p != '\n')
                *p++ = ' ';
        if(*p == '\0')
            break;
    }
}

static bool ssf_is_cont_op(char c){
    return c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '^' || c == '%' || c == ',' || c == '=';
}

static bool ssf_next_statement(const char *src, size_t *pos, char *out, size_t out_size){
    size_t i = *pos;
    while(src[i] != '\0' && isspace((unsigned char)src[i]))
        i++;
    if(src[i] == '\0'){
        *pos = i;
        return false;
    }

    size_t out_len = 0;
    int depth = 0;
    char last_meaningful = '\0';

    while(src[i] != '\0'){
        char c = src[i];

        if(c == '(' || c == '[')
            depth++;
        else if(c == ')' || c == ']')
            depth--;

        if(c == '\n' || c == '\r'){
            if(depth > 0){
                i++;
                continue;
            }
            size_t peek = i;
            while(src[peek] != '\0' && isspace((unsigned char)src[peek]))
                peek++;
            char next = src[peek];
            if(ssf_is_cont_op(last_meaningful) || ssf_is_cont_op(next)){
                i = peek;
                if(out_len && out[out_len - 1] != ' ' && out_len + 1 < out_size)
                    out[out_len++] = ' ';
                continue;
            }
            i++;
            break;
        }

        if(out_len + 1 < out_size)
            out[out_len++] = c;
        if(!isspace((unsigned char)c))
            last_meaningful = c;
        i++;
    }

    out[out_len < out_size ? out_len : out_size - 1] = '\0';
    *pos = i;
    return true;
}

static int ssf_apply_setting(ssf_program *p, const char *key, const char *value, const char *path){
    static const char *const rate[] = {"samplerate", "sample_rate", "rate", "hz", NULL};
    static const char *const dur[]  = {"duration", "ms", NULL};
    static const char *const wid[]  = {"width", "bits", "bit_width", NULL};
    static const char *const fmt[]  = {"format", "container", NULL};

    uint32_t *target = NULL;
    for(int i = 0; rate[i]; i++) if(strcasecmp(key, rate[i]) == 0) target = &p->config.sample_rate;
    for(int i = 0; dur[i];  i++) if(strcasecmp(key, dur[i])  == 0) target = &p->config.duration_ms;
    for(int i = 0; wid[i];  i++) if(strcasecmp(key, wid[i])  == 0) target = &p->config.width;

    if(target != NULL){
        char *end = NULL;
        long parsed = strtol(value, &end, 10);
        while(end != NULL && isspace((unsigned char)*end)) end++;
        if(end == value || (end && *end != '\0') || parsed < 0){
            fprintf(stderr, "ssf: %s: '%s' expects a non-negative integer, got '%s'\n", path, key, value);
            return -1;
        }
        *target = (uint32_t)parsed;
        return 0;
    }

    for(int i = 0; fmt[i]; i++)
        if(strcasecmp(key, fmt[i]) == 0){
            snprintf(p->config.format, sizeof(p->config.format), "%s", value);
            return 0;
        }

    fprintf(stderr, "ssf: %s: unknown setting '%s' (use 'const %s = ...' to define a variable)\n", path, key, key);
    return -1;
}

static int ssf_add_const(ssf_program *p, const char *name, const char *expr, const char *path){
    if(!ssf_is_ident(name)){
        fprintf(stderr, "ssf: %s: invalid constant name '%s'\n", path, name);
        return -1;
    }
    if(ssf_name_taken(p, name)){
        fprintf(stderr, "ssf: %s: '%s' is already defined\n", path, name);
        return -1;
    }
    if(p->const_count >= SSF_MAX_CONSTS){
        fprintf(stderr, "ssf: %s: too many constants (max %d)\n", path, SSF_MAX_CONSTS);
        return -1;
    }
    ssf_const *c = &p->consts[p->const_count];
    snprintf(c->name, sizeof(c->name), "%s", name);
    c->value = 0.0;
    c->expr = NULL;
    c->expr_src = strdup(expr);
    if(c->expr_src == NULL)
        return -1;
    p->const_count++;
    return 0;
}

static int ssf_add_func(ssf_program *p, const char *header, const char *body, const char *path){
    char work[SSF_MAX_NAME * (SSF_MAX_PARAMS + 2)];
    snprintf(work, sizeof(work), "%s", header);

    char *open = strchr(work, '(');
    char *close = strrchr(work, ')');
    if(open == NULL || close == NULL || close < open){
        fprintf(stderr, "ssf: %s: malformed function header '%s'\n", path, header);
        return -1;
    }
    *open = '\0';
    *close = '\0';
    char *name = ssf_trim(work);
    char *params = open + 1;

    if(!ssf_is_ident(name)){
        fprintf(stderr, "ssf: %s: invalid function name '%s'\n", path, name);
        return -1;
    }

    ssf_func tmp;
    memset(&tmp, 0, sizeof(tmp));
    snprintf(tmp.name, sizeof(tmp.name), "%s", name);
    tmp.arity = 0;

    char *plist = ssf_trim(params);
    if(*plist != '\0'){
        char *token = strtok(plist, ",");
        while(token != NULL){
            char *pname = ssf_trim(token);
            if(tmp.arity >= SSF_MAX_PARAMS){
                fprintf(stderr, "ssf: %s: function '%s' has too many parameters (max %d)\n", path, name, SSF_MAX_PARAMS);
                return -1;
            }
            if(!ssf_is_ident(pname)){
                fprintf(stderr, "ssf: %s: invalid parameter name '%s' in '%s'\n", path, pname, name);
                return -1;
            }
            snprintf(tmp.param_names[tmp.arity], SSF_MAX_NAME, "%s", pname);
            tmp.arity++;
            token = strtok(NULL, ",");
        }
    }

    tmp.body_src = strdup(body);
    if(tmp.body_src == NULL)
        return -1;

    for(int i = 0; i < p->func_count; i++)
        if(strcmp(p->funcs[i].name, name) == 0){
            free(p->funcs[i].body_src);
            te_free(p->funcs[i].body);
            p->funcs[i] = tmp;
            p->funcs[i].body = NULL;
            return 0;
        }

    if(ssf_name_taken(p, name)){
        fprintf(stderr, "ssf: %s: '%s' is already defined as a constant\n", path, name);
        free(tmp.body_src);
        return -1;
    }
    if(p->func_count >= SSF_MAX_FUNCS){
        fprintf(stderr, "ssf: %s: too many functions (max %d)\n", path, SSF_MAX_FUNCS);
        free(tmp.body_src);
        return -1;
    }
    p->funcs[p->func_count++] = tmp;
    return 0;
}

static int ssf_parse_file_internal(ssf_program *p, const char *path);

static void ssf_resolve_path(const char *base_file, const char *import_path, char *out, size_t out_size){
    if(import_path[0] == '/'){
        snprintf(out, out_size, "%s", import_path);
        return;
    }
    const char *slash = strrchr(base_file, '/');
    if(slash == NULL)
        snprintf(out, out_size, "%s", import_path);
    else
        snprintf(out, out_size, "%.*s/%s", (int)(slash - base_file), base_file, import_path);
}

static int ssf_handle_import(ssf_program *p, const char *stmt, const char *base_file){
    const char *arg = stmt + strlen("import");
    while(*arg != '\0' && isspace((unsigned char)*arg))
        arg++;

    char pathbuf[PATH_MAX];
    if(*arg == '"' || *arg == '\''){
        char quote = *arg++;
        const char *end = strchr(arg, quote);
        if(end == NULL){
            fprintf(stderr, "ssf: %s: unterminated import path\n", base_file);
            return -1;
        }
        snprintf(pathbuf, sizeof(pathbuf), "%.*s", (int)(end - arg), arg);
    } else {
        snprintf(pathbuf, sizeof(pathbuf), "%s", arg);
        char *trimmed = ssf_trim(pathbuf);
        memmove(pathbuf, trimmed, strlen(trimmed) + 1);
    }
    if(pathbuf[0] == '\0'){
        fprintf(stderr, "ssf: %s: empty import path\n", base_file);
        return -1;
    }

    char resolved[PATH_MAX];
    ssf_resolve_path(base_file, pathbuf, resolved, sizeof(resolved));
    return ssf_parse_file_internal(p, resolved);
}

static int ssf_parse_statement(ssf_program *p, char *stmt, const char *base_file){
    char *s = ssf_trim(stmt);
    if(*s == '\0')
        return 0;

    if(ssf_starts_word(s, "import"))
        return ssf_handle_import(p, s, base_file);

    char *eq = strchr(s, '=');
    if(eq == NULL){
        fprintf(stderr, "ssf: %s: expected 'name = value' or 'import ...', got '%s'\n", base_file, s);
        return -1;
    }
    *eq = '\0';
    char *header = ssf_trim(s);
    char *value = ssf_trim(eq + 1);

    if(ssf_starts_word(header, "const")){
        char *name = ssf_trim(header + strlen("const"));
        if(*value == '\0'){
            fprintf(stderr, "ssf: %s: constant '%s' has no value\n", base_file, name);
            return -1;
        }
        return ssf_add_const(p, name, value, base_file);
    }

    if(ssf_starts_word(header, "fn"))
        header = ssf_trim(header + strlen("fn"));

    if(strchr(header, '(') != NULL){
        if(*value == '\0'){
            fprintf(stderr, "ssf: %s: function '%s' has an empty body\n", base_file, header);
            return -1;
        }
        return ssf_add_func(p, header, value, base_file);
    }

    return ssf_apply_setting(p, header, value, base_file);
}

static int ssf_parse_file_internal(ssf_program *p, const char *path){
    char canonical[PATH_MAX];
    if(realpath(path, canonical) == NULL)
        snprintf(canonical, sizeof(canonical), "%s", path);

    for(int i = 0; i < p->import_count; i++)
        if(strcmp(p->imports[i], canonical) == 0)
            return 0;
    if(p->import_count < SSF_MAX_IMPORTS)
        snprintf(p->imports[p->import_count++], PATH_MAX, "%s", canonical);

    char *src = ssf_read_file(path);
    if(src == NULL)
        return -1;
    ssf_strip_comments(src);

    char stmt[SSF_STMT_MAX];
    size_t pos = 0;
    int status = 0;
    while(ssf_next_statement(src, &pos, stmt, sizeof(stmt))){
        if(ssf_parse_statement(p, stmt, path) != 0){
            status = -1;
            break;
        }
    }

    free(src);
    return status;
}

ssf_program *ssf_program_new(void){
    ssf_program *p = calloc(1, sizeof(*p));
    if(p == NULL)
        return NULL;
    p->config.sample_rate = SSF_DEFAULT_SAMPLE_RATE;
    p->config.duration_ms = SSF_DEFAULT_DURATION_MS;
    p->config.width = SSF_DEFAULT_WIDTH;
    snprintf(p->config.format, sizeof(p->config.format), "%s", SSF_DEFAULT_FORMAT);
    p->main_index = -1;
    return p;
}

int ssf_program_parse_file(ssf_program *program, const char *path){
    if(program == NULL || path == NULL)
        return -1;
    return ssf_parse_file_internal(program, path);
}

int ssf_program_set_main_expr(ssf_program *program, const char *expression){
    if(program == NULL || expression == NULL)
        return -1;
    return ssf_add_func(program, "f(x)", expression, "<command line>");
}

void ssf_program_get_config(const ssf_program *program, ssf_config *out){
    if(program != NULL && out != NULL)
        *out = program->config;
}

void ssf_program_set_config(ssf_program *program, const ssf_config *config){
    if(program != NULL && config != NULL)
        program->config = *config;
}

static int ssf_build_symbols(ssf_program *p, te_variable *table){
    int n = 0;
    table[n++] = (te_variable){"fmod",  m2w_fmod,  TE_FUNCTION2 | TE_FLAG_PURE, 0};
    table[n++] = (te_variable){"min",   m2w_min,   TE_FUNCTION2 | TE_FLAG_PURE, 0};
    table[n++] = (te_variable){"max",   m2w_max,   TE_FUNCTION2 | TE_FLAG_PURE, 0};
    table[n++] = (te_variable){"clamp", m2w_clamp, TE_FUNCTION3 | TE_FLAG_PURE, 0};
    table[n++] = (te_variable){"sign",  m2w_sign,  TE_FUNCTION1 | TE_FLAG_PURE, 0};
    table[n++] = (te_variable){"saw",   m2w_saw,   TE_FUNCTION1 | TE_FLAG_PURE, 0};
    table[n++] = (te_variable){"tri",   m2w_tri,   TE_FUNCTION1 | TE_FLAG_PURE, 0};
    table[n++] = (te_variable){"sqr",   m2w_sqr,   TE_FUNCTION1 | TE_FLAG_PURE, 0};
    table[n++] = (te_variable){"noise", m2w_noise, TE_FUNCTION1 | TE_FLAG_PURE, 0};

    for(int i = 0; i < p->const_count; i++)
        table[n++] = (te_variable){p->consts[i].name, &p->consts[i].value, TE_VARIABLE, 0};

    for(int i = 0; i < p->func_count; i++){
        ssf_func *f = &p->funcs[i];
        /* Not pure: tinyexpr must never constant-fold a user function call. */
        table[n++] = (te_variable){f->name, ssf_trampolines[f->arity],
                                   TE_CLOSURE0 + f->arity, f};
    }
    return n;
}

int ssf_program_compile(ssf_program *program){
    if(program == NULL)
        return -1;

    bool has_f = false;
    for(int i = 0; i < program->func_count; i++)
        if(strcmp(program->funcs[i].name, "f") == 0){
            has_f = true;
            break;
        }
    if(!has_f && ssf_add_func(program, "f(x)", SSF_DEFAULT_MAIN, "<default>") != 0)
        return -1;

    static const int base_max = 9 + SSF_MAX_CONSTS + SSF_MAX_FUNCS;
    te_variable *base = malloc(sizeof(te_variable) * base_max);
    te_variable *local = malloc(sizeof(te_variable) * (base_max + SSF_MAX_PARAMS));
    if(base == NULL || local == NULL){
        free(base);
        free(local);
        return -1;
    }

    int base_n = ssf_build_symbols(program, base);
    int status = 0;

    for(int i = 0; i < program->const_count && status == 0; i++){
        int err = 0;
        program->consts[i].expr = te_compile(program->consts[i].expr_src, base, base_n, &err);
        if(program->consts[i].expr == NULL){
            fprintf(stderr, "ssf: error in constant '%s': %s\n", program->consts[i].name, program->consts[i].expr_src);
            if(err > 0)
                fprintf(stderr, "  %*s^ near column %d\n", err - 1, "", err);
            status = -1;
        }
    }

    for(int i = 0; i < program->func_count && status == 0; i++){
        ssf_func *f = &program->funcs[i];
        int m = 0;
        for(int pi = 0; pi < f->arity; pi++)
            local[m++] = (te_variable){f->param_names[pi], &f->param_vals[pi], TE_VARIABLE, 0};
        memcpy(local + m, base, sizeof(te_variable) * base_n);
        m += base_n;

        int err = 0;
        f->body = te_compile(f->body_src, local, m, &err);
        if(f->body == NULL){
            fprintf(stderr, "ssf: error in function '%s': %s\n", f->name, f->body_src);
            if(err > 0)
                fprintf(stderr, "  %*s^ near column %d\n", err - 1, "", err);
            status = -1;
        }
    }

    free(base);
    free(local);
    if(status != 0)
        return -1;

    for(int i = 0; i < program->const_count; i++)
        program->consts[i].value = te_eval(program->consts[i].expr);

    for(int i = 0; i < program->func_count; i++)
        if(strcmp(program->funcs[i].name, "f") == 0){
            if(program->funcs[i].arity != 1){
                fprintf(stderr, "ssf: output function must be 'f(x)' with exactly one parameter\n");
                return -1;
            }
            program->main_index = i;
        }

    if(program->main_index < 0){
        fprintf(stderr, "ssf: no output function 'f(x)' defined\n");
        return -1;
    }

    program->compiled = true;
    return 0;
}

double ssf_program_eval(const ssf_program *program, double x){
    if(program == NULL || !program->compiled || program->main_index < 0)
        return 0.0;
    ssf_func *f = (ssf_func *)&program->funcs[program->main_index];
    f->param_vals[0] = x;
    return te_eval(f->body);
}

void ssf_program_free(ssf_program *program){
    if(program == NULL)
        return;
    for(int i = 0; i < program->func_count; i++){
        free(program->funcs[i].body_src);
        te_free(program->funcs[i].body);
    }
    for(int i = 0; i < program->const_count; i++){
        free(program->consts[i].expr_src);
        te_free(program->consts[i].expr);
    }
    free(program);
}
