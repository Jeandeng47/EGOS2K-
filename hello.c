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
    va_list args;
    va_start(args, format);
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


// To ensure stack & heap not overlap, brk (end of heap)
// should be lower than stack_start(0x80400000): *brk <= 0x80200000
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


int main() {
    char* msg = "Hello, World!\n\r";
    terminal_write(msg, 15);

    /* Uncomment this line of code
     * when implementing formatted output
     */

    
    // part 1. formatted output
    printf("%s-%d is awesome!", "egos", 2000);
    printf("%c is character $", '$');
    printf("%c is character 0", (char)48);
    printf("%x is integer 1234 in hexadecimal", 1234);
    printf("%u is the maximum of unsigned int", (unsigned int)0xFFFFFFFF);
    printf("%p is the hexadecimal address of the hello-world string", msg);
    printf("%llu is the maximum of unsigned long long", 0xFFFFFFFFFFFFFFFFULL);

    // Expected output:
    // Hello, World!
    // egos-2000 is awesome!
    // $ is character $
    // 0 is character 0
    // 4d2 is integer 1234 in hexadecimal
    // 4294967295 is the maximum of unsigned int
    // 0x800029f8 is the hexadecimal address of the hello-world string
    // 18446744073709551615 is the maximum of unsigned long long

    // part 2. dynamic memory allocation

    // Mixed format
    printf_da("mix: %s %d %u %x %c %p %llu",
              "ok", -42, 4294967295u, 0xBEEF, 'Z', (void*)msg, 1234567890123456789ULL);

    // Longer string
    int len = 700;
    char long_str[len + 1];
    for (int i = 0; i < len; i++) { long_str[i] = 'A' + (i % 26);  };
    long_str[len] = '\0';
    // printf_da("%s ", long_str);

    size_t L = strlen(long_str);
    printf_da("len = %u last = %c", (unsigned)L, long_str[L - 1]); // expect len=700, last=x

    return 0;
}

