void terminal_write(const char *str, int len) {
    for (int i = 0; i < len; i++) {
        *(char*)(0x10000000) = str[i]; // UART send
    }
}

/* Uncomment this code block
 * when implementing formatted output
 */

#include <string.h>  // for strlen() and strcat()
#include <stdlib.h>  // for itoa()
#include <stdarg.h>  // for va_start(), va_end() and va_arg()
#include <stdint.h>  // for uintptr_t

// Simplified version of printf
void format_to_str(char* out, const char* fmt, va_list args) {
    for(out[0] = 0; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            strncat(out, fmt, 1);
        } else {
            fmt++;
            if (*fmt == 's') {
                strcat(out, va_arg(args, char*));
            } else if (*fmt == 'd') {
                itoa(va_arg(args, int), out + strlen(out), 10);
            } else if (*fmt == 'c') {
                // char prompted as int 
                char temp[2];
                temp[0] = (char)va_arg(args, int);
                temp[1] = '\0';
                strcat(out, temp);
            } else if (*fmt == 'x') {
                unsigned int v = va_arg(args, unsigned int);
                itoa((int)v, out + strlen(out), 16);
            } else if (*fmt == 'u') {
                unsigned int v = va_arg(args, unsigned int);
                char temp[11]; // 10 digits + '\0'
                int n = 0;
                do {
                    temp[n++] = (char)('0' + (v % 10u));
                    v /= 10u;
                } while (v);
                // reverse
                temp[n] = '\0';
                for (int i = 0, j = n - 1; i < j; i++, j--) {
                    char c = temp[i];
                    temp[i] = temp[j];
                    temp[j] = c;
                }
                strcat(out, temp);
            } else if (*fmt == 'p') {
                unsigned long v = va_arg(args, unsigned long);

                char buf[2 + sizeof(unsigned long) * 2 + 1]; // '0x' + 2 hex for each byte +'\0'
                char *p = buf;
                *p++ = '0';
                *p++ = 'x';
                
                char temp[sizeof(unsigned long) * 2];
                int n = 0;
                do {
                    unsigned d = v & 0xF; // take lowest 4 bits (0..15)
                    temp[n++] = (char)(d < 10? ('0' + d): ('a' + (d - 10))); // map to hex number 
                    v >>= 4;
                } while (v);
                while (n--) { *p++ = temp[n]; }

                strcat(out, buf);
            } else if (*fmt == 'l' && fmt[1] == 'l' && fmt[2] == 'u') {
                unsigned long long v = va_arg(args, unsigned long long);

                char temp[20 + 1]; // 20 bits(2^64-1) + '\0'
                int n = 0;
                do {
                    unsigned d = v % 10ULL;
                    temp[n++] = (char)('0' + d);
                    v /= 10ULL;
                } while (v);
                temp[n] = '\0';
                for (int i = 0, j = n - 1; i < j; i++, j--) {
                    char c = temp[i];
                    temp[i] = temp[j];
                    temp[j] = c;
                }
                strcat(out, temp);

                fmt += 2; // consume extra 2 symbols, 'l' & 'u'
            }
        }
    }
    // always append a new line
    size_t len = strlen(out);
    if (len == 0 || out[len - 1]) {
        strncat(out, "\n", 1);
    }
}

int printf(const char* format, ...) {
    char buf[512];
    va_list args; // pointer pointing to the next param
    va_start(args, format); // start fetching params from format
    format_to_str(buf, format, args);
    va_end(args);
    terminal_write(buf, strlen(buf));

    return 0;
}


/* Uncomment this code block
 * when implementing dynamic memory allocation
 */

// #include <stdlib.h>  // for malloc(), free()

size_t format_to_str_len(const char* fmt, va_list args) {
    va_list ap;
    va_copy(ap, args); // copy
    size_t L = 0;

    for (; *fmt; fmt++) {
        if (*fmt != '%') { L++; continue; }
        fmt++; // consume %
        if (*fmt == 's') {          // string
            const char* s = va_arg(ap, char*);
            L += strlen(s);
        } else if (*fmt == 'd') {   // signed 32-bit
            int v = va_arg(ap, int);
            unsigned uv = v < 0? (0u - (unsigned)v) : (unsigned) v;
            if (v < 0) L++; // '-'
            do { L++; } while (uv /= 10u);
        } else if (*fmt == 'c') {   // char
            (void)va_arg(ap, int);
            L += 1;
        } else if (*fmt == 'x') {   // hex 32-bit
            unsigned v = va_arg(ap, unsigned);
            do { L++; } while (v >>= 4);
        } else if (*fmt == 'u') {   // unsigned 32-bit
            unsigned v = va_arg(ap, unsigned);
            do { L++; } while (v /= 10u);
        } else if (*fmt == 'p') {   // pointer: 0x + hex(ptr)
            uintptr_t p = va_arg(ap, uintptr_t);
            L += 2;                               // "0x"
            do { ++L; } while (p >>= 4);
        } else if (*fmt == 'l' && fmt[1]=='l' && fmt[2]=='u') {
            unsigned long long v = va_arg(ap, unsigned long long);
            do { ++L; } while (v /= 10ULL);
            fmt += 2;
        } else {
            L += 2; // treat unknow type as %?
        }
        
        va_end(ap);
        return L + 1 + 1; // inclue '\n' + '\0'
    }
}

#include <stddef.h>

#define ALIGN         (sizeof(void*))         
#define ALIGN_UP(x)   (((x) + (ALIGN-1)) & ~(ALIGN-1))

typedef struct blk {
    size_t size;        // size of payload
    struct blk* next;   // pointer to next block
    int free;           // whether the block is freed
} blk_t;

static blk_t* g_head = NULL; // track head of list

extern char __heap_start, __heap_end;
static char* brk = &__heap_start;
char* _sbrk(int size) {
    if (brk + size > (char*)&__heap_end) {
        terminal_write("_sbrk: heap grows too large\r\n", 29);
        return NULL;
    }

    char* old_brk = brk;
    brk += size;
    return old_brk;
}

static blk_t* request_memory(size_t need) {
    size_t total = sizeof(blk_t) + need; // header + payload
    blk_t* b = (blk_t*)_sbrk((int) total);
    if (!b) return NULL;
    b->size = need;
    b->next = NULL;
    b->free = 0;
    return b;
} 

// Return the pointer to the newly-allocated memory region
void* malloc_e(size_t sz) {
    if (sz == 0) return NULL;
    sz = ALIGN_UP(sz);
    
    // Find first-fit
    for (blk_t* cur = g_head; cur; cur = cur->next) {
        if (cur->free && cur->size >= sz) {
            cur->free = 0;
            return (void*)(cur + 1);
        }
    }

    // If no fit, allocate
    if (!g_head) { 
        blk_t* b = request_memory(sz);
        if (!b) return NULL;
        g_head = b;
        return (void*)(b + 1); // skip sizeof(blk_t), point to payload
    } 

    // Iterate the find the next block
    blk_t* cur = g_head;
    while (cur->next) { cur = cur->next; }
    blk_t *b = request_memory(sz);
    if (!b) return NULL;
    cur->next = b;
    return (void*)(b + 1);
}

// Free the memory region pointed by ptr
void free_e(void* ptr) {
    if(!ptr) return;
    blk_t* b = ((blk_t*)ptr) - 1;
    b->free = 1; // mark unsued
}

// Printf with dynamic allocation
int printf_da(const char* format, ...) {
    va_list args;
    va_start(args, format);
    // Print output string that is longer than 512 bytes
    size_t len = format_to_str_len(format, args);
    char *buf = malloc(len);
    format_to_str(buf, format, args);
    va_end(args);
    terminal_write(buf, strlen(buf));
    free(buf);

    return 0;
}

// Test printf() and printf_da()
static void test_printf(char* msg) {
    printf("\n======= Test printf =======");
    printf("%s-%d is awesome!", "egos", 2000);
    printf("%c is character $", '$');
    printf("%c is character 0", (char)48);
    printf("%x is integer 1234 in hexadecimal", 1234);
    printf("%u is the maximum of unsigned int", (unsigned int)0xFFFFFFFF);
    printf("%p is the hexadecimal address of the hello-world string", msg);
    printf("%llu is the maximum of unsigned long long", 0xFFFFFFFFFFFFFFFFULL);
}

static void test_printf_da(char* msg) {
    printf("\n======= Test printf_da =======");
    // mixed format
    printf_da("mix: %s %d %u %x %c %p %llu",
              "ok", -42, 4294967295u, 0xBEEF, 'Z', (void*)msg, 1234567890123456789ULL);

    // longer string
    int len = 700;
    char long_str[len + 1];
    for (int i = 0; i < len; i++) { long_str[i] = 'A' + (i % 26); }
    long_str[len] = '\0';
    size_t L = strlen(long_str);
    printf_da("len = %u last = %c", (unsigned)L, long_str[L - 1]);
}

// Test malloc() & free()
static int is_aligned(void* p) {
    return ((uintptr_t)p % sizeof(void*)) == 0;
}
static void test_allocator() {
    printf("\n======= Test allocator =======");
    int total = 0, pass = 0;
    
    // 1) Basic alloc, free, re-use
    total++;
    void* a = malloc_e(100);
    void* b = malloc_e(200);
    if (a && b && a != b) {
        free_e(a);
        void* c = malloc_e(100);
        if (c == a) { printf("PASS: basic reuse"); pass++; }
        else        { printf("FAIL: basic reuse (c=%p a=%p)", c, a); }
    } else {
        printf("FAIL: basic alloc (a=%p b=%p)", a, b);
    }

    // 2) Test alignment
    total++;
    void *p1 = malloc_e(1);
    void* p2 = malloc_e(3);
    if (p1 && p2 && is_aligned(p1) && is_aligned(p2)) {
        printf("PASS: alignment"); pass++;
    } else {
        printf("FAIL: alignment (p1=%p p2=%p)", p1, p2);
    }

    // 3) zero-size and free(NULL)
    total++;
    void* z = malloc_e(0);
    free_e(NULL); // should be no-op
    if (z == NULL) { printf("PASS: zero-size + free(NULL)"); pass++; }
    else           { printf("FAIL: zero-size (got %p)", z); }

    // 4) first-fit hole usage vs. larger request to tail
    total++;
    void* x = malloc_e(400);
    void* y = malloc_e(400);
    void* w = malloc_e(400);
    if (x && y && w) {
        free_e(y);                    // create a 400-sized hole
        void* y2 = malloc_e(200);     // should land in y
        void* big = malloc_e(500);    // should be new tail (not y)
        if (y2 == y && big && big != y) { printf("PASS: first-fit behavior"); pass++; }
        else { printf("FAIL: first-fit (y2=%p y=%p big=%p)", y2, y, big); }
    } else {
        printf("FAIL: setup for first-fit");
    }

    printf("Allocator tests: %d/%d passed\n", pass, total);
}

int main() {
    char* msg = "Hello, World!\n\r";
    terminal_write(msg, 15);

    /* Uncomment this line of code
     * when implementing formatted output
     */

    // part 1. formatted output
    test_printf(msg);
    test_printf_da(msg);

    // part 2. dynamic memory allocation
    test_allocator();
    return 0;
}

