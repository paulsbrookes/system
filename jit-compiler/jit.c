/*
 * Brainfuck JIT Compiler — x86-64 / Linux
 *
 * Translates Brainfuck source to x86-64 machine code at runtime, then
 * executes it.  Demonstrates:
 *   - mmap / mprotect for W^X code generation
 *   - x86-64 instruction encoding (REX prefixes, ModR/M, SIB, immediates)
 *   - Calling-convention interop (JIT'd code calls putchar / getchar)
 *   - Backpatching for loop compilation
 *   - Run-length optimisation (consecutive +/-/>/< collapsed)
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* ── constants ─────────────────────────────────────────────────────── */

#define CODE_SIZE    (1 << 20)          /* 1 MiB code buffer            */
#define TAPE_SIZE    (1 << 16)          /* 64 KiB BF tape (65 536 cells)*/
#define MAX_NESTING  256                /* maximum [ ] nesting depth    */

/* ── code buffer state ─────────────────────────────────────────────── */

static uint8_t *code;          /* base of mmap'd code buffer */
static size_t   code_len;      /* bytes emitted so far       */

/* ── verbose-mode log ──────────────────────────────────────────────── */

typedef struct {
    size_t   offset;            /* byte offset in code buffer */
    size_t   len;               /* length of this chunk       */
    char     desc[80];          /* human-readable description */
} log_entry;

static log_entry *vlog;
static size_t     vlog_len;
static size_t     vlog_cap;
static int        verbose;

static void vlog_push(size_t offset, size_t len, const char *desc)
{
    if (!verbose)
        return;
    if (vlog_len == vlog_cap) {
        vlog_cap = vlog_cap ? vlog_cap * 2 : 64;
        vlog = realloc(vlog, vlog_cap * sizeof *vlog);
        if (!vlog) { perror("realloc"); exit(1); }
    }
    vlog[vlog_len].offset = offset;
    vlog[vlog_len].len    = len;
    snprintf(vlog[vlog_len].desc, sizeof vlog[0].desc, "%s", desc);
    vlog_len++;
}

/* ── emit helpers ──────────────────────────────────────────────────── */

static void check_space(size_t need)
{
    if (code_len + need > CODE_SIZE) {
        fprintf(stderr, "error: code buffer overflow (%zu bytes used, "
                "need %zu more)\n", code_len, need);
        exit(1);
    }
}

static void emit_byte(uint8_t b)
{
    check_space(1);
    code[code_len++] = b;
}

static void emit_bytes(const uint8_t *buf, size_t n)
{
    check_space(n);
    memcpy(code + code_len, buf, n);
    code_len += n;
}

static void emit_u32(uint32_t v)
{
    check_space(4);
    memcpy(code + code_len, &v, 4);
    code_len += 4;
}

static void emit_u64(uint64_t v)
{
    check_space(8);
    memcpy(code + code_len, &v, 8);
    code_len += 8;
}

static void patch_u32(size_t offset, uint32_t v)
{
    memcpy(code + offset, &v, 4);
}

/* ── prologue / epilogue ───────────────────────────────────────────── */

/*
 * Prologue — called as  void jit_entry(uint8_t *tape)
 *
 * Register allocation:
 *   r12  = tape base pointer  (callee-saved)
 *   r13  = current cell offset (data pointer, callee-saved)
 *
 * push rbp              ; 55
 * mov  rbp, rsp         ; 48 89 e5
 * push r12              ; 41 54
 * push r13              ; 41 55
 * mov  r12, rdi         ; 49 89 fc   (tape base → r12)
 * xor  r13d, r13d       ; 45 31 ed   (offset = 0)
 *
 * 3 pushes → rsp ≡ 0 (mod 16).  The `call` that invoked us already
 * pushed 8 bytes, so now rsp ≡ 8 (mod 16) at the point after entry,
 * but with our 3 pushes (24 bytes) rsp ≡ 0 (mod 16) — any further
 * `call` from JIT'd code will leave rsp ≡ 8 (mod 16) on callee entry,
 * which is exactly what the System V ABI requires.
 */
static void emit_prologue(void)
{
    size_t start = code_len;
    static const uint8_t prologue[] = {
        0x55,                   /* push rbp            */
        0x48, 0x89, 0xe5,       /* mov  rbp, rsp       */
        0x41, 0x54,             /* push r12            */
        0x41, 0x55,             /* push r13            */
        0x49, 0x89, 0xfc,       /* mov  r12, rdi       */
        0x45, 0x31, 0xed,       /* xor  r13d, r13d     */
    };
    emit_bytes(prologue, sizeof prologue);
    vlog_push(start, sizeof prologue,
              "push rbp; mov rbp,rsp; push r12; push r13; "
              "mov r12,rdi; xor r13d,r13d");
}

/*
 * Epilogue:
 * pop r13   ; 41 5d
 * pop r12   ; 41 5c
 * pop rbp   ; 5d
 * ret       ; c3
 */
static void emit_epilogue(void)
{
    size_t start = code_len;
    static const uint8_t epilogue[] = {
        0x41, 0x5d,             /* pop r13  */
        0x41, 0x5c,             /* pop r12  */
        0x5d,                   /* pop rbp  */
        0xc3,                   /* ret      */
    };
    emit_bytes(epilogue, sizeof epilogue);
    vlog_push(start, sizeof epilogue, "pop r13; pop r12; pop rbp; ret");
}

/* ── BF instruction emitters ──────────────────────────────────────── */

/*
 * + / - (cell increment / decrement)
 *
 *  x1:  inc byte [r12+r13]   →  43 fe 04 2c          (4 bytes)
 *  xN:  add byte [r12+r13],N →  43 80 04 2c NN       (5 bytes)
 *
 *  x1:  dec byte [r12+r13]   →  43 fe 0c 2c          (4 bytes)
 *  xN:  sub byte [r12+r13],N →  43 80 2c 2c NN       (5 bytes)
 */
static void emit_add_cell(int n)
{
    size_t start = code_len;
    char desc[80];

    if (n == 1) {
        /* inc byte [r12+r13] */
        static const uint8_t enc[] = { 0x43, 0xfe, 0x04, 0x2c };
        emit_bytes(enc, 4);
        snprintf(desc, sizeof desc, "inc byte [r12+r13]");
    } else if (n == -1) {
        /* dec byte [r12+r13] */
        static const uint8_t enc[] = { 0x43, 0xfe, 0x0c, 0x2c };
        emit_bytes(enc, 4);
        snprintf(desc, sizeof desc, "dec byte [r12+r13]");
    } else if (n > 0) {
        /* add byte [r12+r13], imm8 */
        uint8_t enc[] = { 0x43, 0x80, 0x04, 0x2c, (uint8_t)n };
        emit_bytes(enc, 5);
        snprintf(desc, sizeof desc, "add byte [r12+r13], %d", n);
    } else {
        /* sub byte [r12+r13], imm8 */
        uint8_t enc[] = { 0x43, 0x80, 0x2c, 0x2c, (uint8_t)(-n) };
        emit_bytes(enc, 5);
        snprintf(desc, sizeof desc, "sub byte [r12+r13], %d", -n);
    }
    vlog_push(start, code_len - start, desc);
}

/*
 * > / < (pointer increment / decrement)
 *
 *  x1:  inc r13               →  49 ff c5             (3 bytes)
 *  xN:  add r13, imm8         →  49 83 c5 NN          (4 bytes, |N|≤127)
 *  xN:  add r13, imm32        →  49 81 c5 NN NN NN NN (7 bytes)
 *
 *  x1:  dec r13               →  49 ff cd             (3 bytes)
 *  xN:  sub r13, imm8         →  49 83 ed NN          (4 bytes, |N|≤127)
 *  xN:  sub r13, imm32        →  49 81 ed NN NN NN NN (7 bytes)
 */
static void emit_move_ptr(int n)
{
    size_t start = code_len;
    char desc[80];

    if (n == 1) {
        static const uint8_t enc[] = { 0x49, 0xff, 0xc5 };
        emit_bytes(enc, 3);
        snprintf(desc, sizeof desc, "inc r13");
    } else if (n == -1) {
        static const uint8_t enc[] = { 0x49, 0xff, 0xcd };
        emit_bytes(enc, 3);
        snprintf(desc, sizeof desc, "dec r13");
    } else if (n > 0 && n <= 127) {
        uint8_t enc[] = { 0x49, 0x83, 0xc5, (uint8_t)n };
        emit_bytes(enc, 4);
        snprintf(desc, sizeof desc, "add r13, %d", n);
    } else if (n < 0 && n >= -127) {
        uint8_t enc[] = { 0x49, 0x83, 0xed, (uint8_t)(-n) };
        emit_bytes(enc, 4);
        snprintf(desc, sizeof desc, "sub r13, %d", -n);
    } else if (n > 0) {
        emit_byte(0x49); emit_byte(0x81); emit_byte(0xc5);
        emit_u32((uint32_t)n);
        snprintf(desc, sizeof desc, "add r13, %d", n);
    } else {
        emit_byte(0x49); emit_byte(0x81); emit_byte(0xed);
        emit_u32((uint32_t)(-n));
        snprintf(desc, sizeof desc, "sub r13, %d", -n);
    }
    vlog_push(start, code_len - start, desc);
}

/*
 * .  (putchar)
 *
 * movzx edi, byte [r12+r13]  →  43 0f b6 3c 2c          (5 bytes)
 * mov   rax, imm64           →  48 b8 <8 bytes>          (10 bytes)
 * call  rax                  →  ff d0                     (2 bytes)
 *                                                   total: 17 bytes
 */
static void emit_putchar(void)
{
    size_t start = code_len;

    /* movzx edi, byte [r12+r13] */
    static const uint8_t movzx[] = { 0x43, 0x0f, 0xb6, 0x3c, 0x2c };
    emit_bytes(movzx, 5);

    /* mov rax, imm64 (address of putchar) */
    emit_byte(0x48); emit_byte(0xb8);
    emit_u64((uint64_t)(uintptr_t)putchar);

    /* call rax */
    emit_byte(0xff); emit_byte(0xd0);

    vlog_push(start, code_len - start,
              "movzx edi,[r12+r13]; mov rax,putchar; call rax");
}

/*
 * ,  (getchar)
 *
 * mov   rax, imm64           →  48 b8 <8 bytes>          (10 bytes)
 * call  rax                  →  ff d0                     (2 bytes)
 * mov   byte [r12+r13], al   →  43 88 04 2c              (4 bytes)
 *                                                   total: 16 bytes
 */
static void emit_getchar(void)
{
    size_t start = code_len;

    /* mov rax, imm64 (address of getchar) */
    emit_byte(0x48); emit_byte(0xb8);
    emit_u64((uint64_t)(uintptr_t)getchar);

    /* call rax */
    emit_byte(0xff); emit_byte(0xd0);

    /* mov byte [r12+r13], al */
    static const uint8_t mov[] = { 0x43, 0x88, 0x04, 0x2c };
    emit_bytes(mov, 4);

    vlog_push(start, code_len - start,
              "mov rax,getchar; call rax; mov [r12+r13],al");
}

/*
 * cmp byte [r12+r13], 0  →  43 80 3c 2c 00   (5 bytes)
 *
 * Shared by [ and ].
 */
static void emit_cmp_cell_zero(void)
{
    static const uint8_t cmp[] = { 0x43, 0x80, 0x3c, 0x2c, 0x00 };
    emit_bytes(cmp, 5);
}

/*
 * [  (loop open)
 *
 * cmp byte [r12+r13], 0   →  5 bytes
 * je  rel32               →  0f 84 <4 bytes>  (6 bytes, placeholder)
 *                                        total: 11 bytes
 *
 * Returns the offset of the rel32 displacement for backpatching.
 */
static size_t emit_loop_open(void)
{
    size_t start = code_len;

    emit_cmp_cell_zero();

    /* je rel32 (placeholder) */
    emit_byte(0x0f); emit_byte(0x84);
    size_t fixup = code_len;
    emit_u32(0);  /* placeholder */

    vlog_push(start, code_len - start,
              "cmp byte [r12+r13],0; je <forward>");

    return fixup;
}

/*
 * ]  (loop close)
 *
 * cmp byte [r12+r13], 0   →  5 bytes
 * jne rel32               →  0f 85 <4 bytes>  (6 bytes)
 *                                        total: 11 bytes
 *
 * Also patches the corresponding ['s je to point just past this ].
 */
static void emit_loop_close(size_t open_fixup)
{
    size_t start = code_len;

    emit_cmp_cell_zero();

    /* jne rel32 — backward to just after the ['s cmp */
    emit_byte(0x0f); emit_byte(0x85);
    /* target = open_fixup - 4  (the cmp is 5 bytes before the fixup slot,
       but we want to jump to the cmp, which is at open_fixup - 4 - 1...
       actually: the [ block is  cmp(5) + je-opcode(2) + rel32(4).
       The cmp starts at open_fixup - 2 - 5 = open_fixup - 7.
       But we want to re-evaluate from the cmp of the [, which is at
       the start of the [ block.  That's open_fixup - 6 (je opcode is 2 bytes,
       so fixup = start_of_bracket + 5 + 2, meaning start = fixup - 7).

       Actually let's be precise:
         [  emits:  cmp(5 bytes) + 0f 84(2 bytes) + rel32(4 bytes) = 11 bytes
         open_fixup points to the first byte of the rel32.
         Start of the [ block = open_fixup - 7.

       But for ], we want to jump back to the START of ] (our own cmp),
       not to [.  Wait — the semantics of ] is: if cell != 0, jump back
       to just after [.  "Just after [" means the instruction after the
       je, which is at open_fixup + 4.  But that's the BODY.  Actually
       standard BF compilation: ] jumps back to [ (i.e. re-check the
       condition at [).  So ] should jump to the cmp of [.

       The cmp of [ starts at  open_fixup - 7.
       Our jne rel32: the displacement is relative to the instruction
       AFTER the jne, i.e. code_len + 4.
       displacement = target - (code_len + 4)
       target = open_fixup - 7
    */
    int32_t back_disp = (int32_t)((open_fixup - 7) - (code_len + 4));
    emit_u32((uint32_t)back_disp);

    /* Patch the ['s je to jump here (just past this ]) */
    int32_t fwd_disp = (int32_t)(code_len - (open_fixup + 4));
    patch_u32(open_fixup, (uint32_t)fwd_disp);

    vlog_push(start, code_len - start,
              "cmp byte [r12+r13],0; jne <back>");
}

/* ── bracket validation ────────────────────────────────────────────── */

static int validate_brackets(const char *src, size_t len)
{
    int depth = 0;
    int max_depth = 0;
    size_t first_unmatched = 0;

    for (size_t i = 0; i < len; i++) {
        if (src[i] == '[') {
            if (depth == 0) first_unmatched = i;
            depth++;
            if (depth > max_depth) max_depth = depth;
        } else if (src[i] == ']') {
            if (depth == 0) {
                fprintf(stderr, "error: unmatched ']' at position %zu\n", i);
                return -1;
            }
            depth--;
        }
    }
    if (depth != 0) {
        fprintf(stderr, "error: unmatched '[' at position %zu "
                "(%d unclosed)\n", first_unmatched, depth);
        return -1;
    }
    if (max_depth > MAX_NESTING) {
        fprintf(stderr, "error: nesting depth %d exceeds maximum %d\n",
                max_depth, MAX_NESTING);
        return -1;
    }
    return 0;
}

/* ── source reading ────────────────────────────────────────────────── */

static char *read_source(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        perror(path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) { perror("ftell"); fclose(f); return NULL; }
    rewind(f);

    char *buf = malloc((size_t)sz + 1);
    if (!buf) { perror("malloc"); fclose(f); return NULL; }

    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    *out_len = n;
    return buf;
}

/* ── compilation ───────────────────────────────────────────────────── */

static int compile(const char *src, size_t len)
{
    /* backpatch stack */
    size_t fixups[MAX_NESTING];
    int    depth = 0;

    emit_prologue();

    for (size_t i = 0; i < len; ) {
        char c = src[i];

        switch (c) {
        case '+':
        case '-': {
            /* Run-length: count consecutive +/- as net delta */
            int delta = 0;
            while (i < len && (src[i] == '+' || src[i] == '-')) {
                delta += (src[i] == '+') ? 1 : -1;
                i++;
            }
            /* Clamp to byte range: delta mod 256, but keep sign info */
            delta = ((delta % 256) + 256) % 256;
            if (delta == 0) break;
            if (delta <= 128)
                emit_add_cell(delta);
            else
                emit_add_cell(delta - 256);  /* emit as sub */
            break;
        }

        case '>':
        case '<': {
            int delta = 0;
            while (i < len && (src[i] == '>' || src[i] == '<')) {
                delta += (src[i] == '>') ? 1 : -1;
                i++;
            }
            if (delta == 0) break;
            emit_move_ptr(delta);
            break;
        }

        case '.':
            emit_putchar();
            i++;
            break;

        case ',':
            emit_getchar();
            i++;
            break;

        case '[':
            fixups[depth++] = emit_loop_open();
            i++;
            break;

        case ']':
            if (depth == 0) {
                fprintf(stderr, "error: unmatched ']' during compilation "
                        "at position %zu\n", i);
                return -1;
            }
            emit_loop_close(fixups[--depth]);
            i++;
            break;

        default:
            /* Ignore non-BF characters (comments) */
            i++;
            break;
        }
    }

    emit_epilogue();
    return 0;
}

/* ── verbose output ────────────────────────────────────────────────── */

static void print_disassembly(void)
{
    printf("--- disassembly (%zu bytes) ---\n", code_len);
    for (size_t i = 0; i < vlog_len; i++) {
        printf("  %04zx: ", vlog[i].offset);
        for (size_t j = 0; j < vlog[i].len && j < 16; j++)
            printf("%02x ", code[vlog[i].offset + j]);
        /* Pad to align descriptions */
        for (size_t j = vlog[i].len; j < 16; j++)
            printf("   ");
        printf(" %s\n", vlog[i].desc);
    }
    printf("--- end disassembly ---\n\n");
}

/* ── usage ─────────────────────────────────────────────────────────── */

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options] <file.bf>\n"
        "       %s [options] -e '<brainfuck code>'\n"
        "\n"
        "Options:\n"
        "  -v    Verbose mode: show generated machine code disassembly\n"
        "  -e    Execute inline code instead of reading a file\n",
        prog, prog);
}

/* ── main ──────────────────────────────────────────────────────────── */

int main(int argc, char **argv)
{
    const char *inline_code = NULL;
    const char *filename    = NULL;
    verbose = 0;

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-e") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "error: -e requires an argument\n");
                usage(argv[0]);
                return 1;
            }
            inline_code = argv[i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            usage(argv[0]);
            return 1;
        } else {
            filename = argv[i];
        }
    }

    if (!inline_code && !filename) {
        usage(argv[0]);
        return 1;
    }
    if (inline_code && filename) {
        fprintf(stderr, "error: cannot use both -e and a filename\n");
        return 1;
    }

    /* Get source */
    char   *src;
    size_t  src_len;

    if (inline_code) {
        src_len = strlen(inline_code);
        src = malloc(src_len + 1);
        if (!src) { perror("malloc"); return 1; }
        memcpy(src, inline_code, src_len + 1);
    } else {
        src = read_source(filename, &src_len);
        if (!src) return 1;
    }

    /* Validate brackets */
    if (validate_brackets(src, src_len) != 0) {
        free(src);
        return 1;
    }

    /* Allocate code buffer (writable, not executable — W^X) */
    code = mmap(NULL, CODE_SIZE, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) {
        perror("mmap (code buffer)");
        fprintf(stderr, "hint: if on a system with restricted mmap, "
                "check SELinux / seccomp policy\n");
        free(src);
        return 1;
    }

    /* Allocate tape (zero-initialized by mmap) */
    uint8_t *tape = mmap(NULL, TAPE_SIZE, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (tape == MAP_FAILED) {
        perror("mmap (tape)");
        munmap(code, CODE_SIZE);
        free(src);
        return 1;
    }

    /* Compile */
    code_len = 0;
    if (compile(src, src_len) != 0) {
        munmap(tape, TAPE_SIZE);
        munmap(code, CODE_SIZE);
        free(src);
        return 1;
    }
    free(src);

    /* Verbose: print disassembly and memory layout */
    if (verbose) {
        printf("code buffer: %p (%zu bytes used of %d)\n",
               (void *)code, code_len, CODE_SIZE);
        printf("tape:        %p (%d bytes)\n\n",
               (void *)tape, TAPE_SIZE);
        print_disassembly();
    }

    /* W^X: remove write, add execute */
    if (mprotect(code, CODE_SIZE, PROT_READ | PROT_EXEC) != 0) {
        perror("mprotect (W→X)");
        fprintf(stderr, "hint: if on a system with restricted mprotect, "
                "check SELinux / seccomp policy\n");
        munmap(tape, TAPE_SIZE);
        munmap(code, CODE_SIZE);
        return 1;
    }

    /* Execute — memcpy avoids pedantic warning about object-to-function
       pointer cast (ISO C technically forbids it, POSIX guarantees it) */
    void (*jit_entry)(uint8_t *);
    memcpy(&jit_entry, &code, sizeof jit_entry);
    jit_entry(tape);

    /* Flush stdout in case the BF program didn't print a newline */
    fflush(stdout);

    /* Clean up */
    munmap(tape, TAPE_SIZE);
    munmap(code, CODE_SIZE);
    free(vlog);

    return 0;
}
