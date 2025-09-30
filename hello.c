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
/*
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
*/

int main() {
    char* msg = "Hello, World!\n\r";
    terminal_write(msg, 15);

    /* Uncomment this line of code
     * when implementing formatted output
     */

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

    return 0;
}
