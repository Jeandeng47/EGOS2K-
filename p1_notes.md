# Part 1: Formatted output

## printf
```C
int printf(const char* format, ...) {
    char buf[512];
    va_list args;
    va_start(args, format);
    format_to_str(buf, format, args);
    va_end(args);
    terminal_write(buf, strlen(buf));

    return 0;
}
```
**1. Prologue**
- Expand stack by 592 bytes, which covers output buffer at `s0-528`
- Save regs and keep a small area to stage `va_list` and spilled regs
- For variadic function, compiler spills `a1...a7` to stack `4(s0)...28(s0)`
- `a0` stores the format `const char*` of `va_list`
```
8000017c:	db010113          	addi	sp,sp,-592
80000180:	22112623          	sw	ra,556(sp)
80000184:	22812423          	sw	s0,552(sp)
80000188:	23010413          	addi	s0,sp,560

8000018c:	dca42e23          	sw	a0,-548(s0)  # format saved
80000190:	00b42223          	sw	a1,4(s0)
80000194:	00c42423          	sw	a2,8(s0)
80000198:	00d42623          	sw	a3,12(s0)
8000019c:	00e42823          	sw	a4,16(s0)
800001a0:	00f42a23          	sw	a5,20(s0)
800001a4:	01042c23          	sw	a6,24(s0)
800001a8:	01142e23          	sw	a7,28(s0)
```

**2. Build va_list args**
- Net effect: let `args` points to the position of `a1`-- `4(s0)`
- Match `va_start(args, format)`: start at the first unnamed arg, the one after format
```
800001ac:	02040793          	addi	a5,s0,32    # a5 = s0 +32
800001b0:	dcf42c23          	sw	a5,-552(s0)     # store tmp ptr
800001b4:	dd842783          	lw	a5,-552(s0)     # reload tmp
800001b8:	fe478793          	addi	a5,a5,-28   # a5 = (s0 + 32) - 28 = s0 + 4
800001bc:	def42623          	sw	a5,-532(s0)     # store back to stack
```
**3.Call function**
- With `a0=buf`, `a1=format`, `a2=args`, call `format_to_str(buf, format, args)`
- Inside `format_to_str`, the `%s/%d` consumes `args` by reading and advancing the pointer (`*(char**)args`, `*(int*)args`)
```
800001c0:	dec42703          	lw	a4,-532(s0)     # a4 = args
800001c4:	df040793          	addi	a5,s0,-528  # a5 = &buf[0]
800001c8:	00070613          	mv	a2,a4           # a2 = args
800001cc:	ddc42583          	lw	a1,-548(s0)     # a1 = format
800001d0:	00078513          	mv	a0,a5           # a0 = buf
800001d4:	ea5ff0ef          	jal	80000078 <format_to_str>
```

**4. Write result to terminal**
- Computes `strlen(buf)` and calls `terminal_write(buf, len)`
```
800001d8:	df040793          	addi	a5,s0,-528      # a5 = buf
800001dc:	00078513          	mv	a0,a5               # a0 = buf
800001e0:	39c000ef          	jal	8000057c <strlen>   # len = strlen(buf)
800001e4:	00050793          	mv	a5,a0               # a5 = len
800001e8:	00078713          	mv	a4,a5               # a4 = len
800001ec:	df040793          	addi	a5,s0,-528      # a5 = buf
800001f0:	00070593          	mv	a1,a4               # a1 = len
800001f4:	00078513          	mv	a0,a5               # a0 = buf
800001f8:	e19ff0ef          	jal	80000010 <terminal_write> 
```

## format_to_str
```C
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
            }
        }
    }
}
```
**0. Overview**
- Scan `fmt` using a for loop
    - if char is not `%`, append it to `out` through `strncat()`
    - if char is `%`, check the next char
        - if it's `%s`, take next from `args` (type `char*`), append to `out`
        - if it's `%d`, take next from `args` (type `int`), append to `out`
- Finish scanning if encounter `\0`

**1. Prologue**
- Stack grows down by 32 bytes to save `ra`, `s0`, `s1` and parameters `a0`, `a1`, `a2`
- Load `out` pointer to `a5` and zero-terminate

```
80000078 <format_to_str>:
format_to_str():
80000078:	fe010113          	addi	sp,sp,-32 # stack grows down by 32 bytes
8000007c:	00112e23          	sw	ra,28(sp)
80000080:	00812c23          	sw	s0,24(sp)
80000084:	00912a23          	sw	s1,20(sp)
80000088:	02010413          	addi	s0,sp,32 # s0 = stack base

8000008c:	fea42623          	sw	a0,-20(s0) # out
80000090:	feb42423          	sw	a1,-24(s0) # fmt
80000094:	fec42223          	sw	a2,-28(s0) # args
80000098:	fec42783          	lw	a5,-20(s0) # a5 = out
8000009c:	00078023          	sb	zero,0(a5) # null-terminate (*out = '\0')
800000a0:	0b40006f          	j	80000154 <format_to_str+0xdc>
```

**2. The for loop**
- Jump to `0x80000154` after prologue, which is the loop condition check (do-while)
- Increment `fmt` each iteration
```
80000148:	fe842783          	lw	a5,-24(s0)
8000014c:	00178793          	addi	a5,a5,1 # fmt++
80000150:	fef42423          	sw	a5,-24(s0)

80000154:	fe842783          	lw	a5,-24(s0)  # a5 = fmt
80000158:	0007c783          	lbu	a5,0(a5)    # *fmt
8000015c:	f40794e3          	bnez	a5,800000a4 <format_to_str+0x2c>  #if(*fmt) goto body
```

**3. The body**
- If `*fmt` pass condition check, go to `0x800000a4`
- Load current char and compare to `%` (ASCII 37)
    - `non-%` path: go to `0x800000b4`, call `strncat(out, fmt, 1)`
    - `%` path: go to `0x800000c8`, skip `%`, compare to 's' (ASCII 115)
        - `s` path: go to `0x800000e4`, call `strcat(out, va_arg(args, char*))`
        - `d` path: go to `0x80000104`, call  `itoa(va_arg(args, int), out + strlen(out), 10)`
```
800000a4:	fe842783          	lw	a5,-24(s0)  # a5 = fmt
800000a8:	0007c703          	lbu	a4,0(a5)    # a4 = *fmt
800000ac:	02500793          	li	a5,37       # a5 = '%'
800000b0:	00f70c63          	beq	a4,a5,800000c8 <format_to_str+0x50> # if '%' -> handle format

# case: non-%
800000b4:	00100613          	li	a2,1        # a2 = 1
800000b8:	fe842583          	lw	a1,-24(s0)  # a1 = fmt
800000bc:	fec42503          	lw	a0,-20(s0)  # a0 = out
800000c0:	410000ef          	jal	800004d0 <strncat> # -> go to strncat(out, fmt, 1)
800000c4:	0840006f          	j	80000148 <format_to_str+0xd0>

# case: %
800000c8:	fe842783          	lw	a5,-24(s0)  # a5 = fmt
800000cc:	00178793          	addi	a5,a5,1 # fmt++ -> skip the %
800000d0:	fef42423          	sw	a5,-24(s0)
800000d4:	fe842783          	lw	a5,-24(s0)
800000d8:	0007c703          	lbu	a4,0(a5)    # a4 = *fmt
800000dc:	07300793          	li	a5,115      # a5 = 115
800000e0:	02f71263          	bne	a4,a5,80000104 <format_to_str+0x8c> # if not 's', check d
```
- case %s: `strcat(out, va_arg(args, char*))`
- 1. Fetch the value: treat memory that `args` points to as `*char` variable
    - `args` is pointer to variable argument area
    - `(char**)args` interpret `args` as pointing to `char*` (String)
    - `*(char**)args` dereferences it, producing the actual string pointer.
- 2. Advance the pointer: advance the pointer `args` by 4 (size of `*char`)
```
# case: %s
800000e4:	fe442783          	lw	a5,-28(s0)      # a5 = args
800000e8:	00478713          	addi	a4,a5,4     # a4 = args + 4  -> move to next arg
800000ec:	fee42223          	sw	a4,-28(s0)      # store updated args to stack
800000f0:	0007a783          	lw	a5,0(a5)        # a5 = *(char**)args -> deref the old ptr
800000f4:	00078593          	mv	a1,a5           # a1 = args string
800000f8:	fec42503          	lw	a0,-20(s0)      # a0 = out
800000fc:	334000ef          	jal	80000430 <strcat>
80000100:	0480006f          	j	80000148 <format_to_str+0xd0> # fmt++
```

- case %d: `itoa(va_arg(args, int), out + strlen(out), 10)`
- 1. Fetch the value: treat memory that `args` points to as `int` variable
    - `args` is pointer to variable argument area 
    - `(int*)args` interpret `args` as pointing to `int`
    - `*(int*)args` dereferences it, producing the integer value
- 2. Advance the pointer: advance the pointer `args` by 4 (size of `int`)
- 3. Append decimal string to output:
    - `va_arg(args, int)`: return int value
    - `out + strlen(out)`: add current length of string, find end of `out`
    - `10`: specified base-10
```

# case: %d
80000104:	fe842783          	lw	a5,-24(s0)  # a5 = fmt
80000108:	0007c703          	lbu	a4,0(a5)    # a4 = *fmt
8000010c:	06400793          	li	a5,100      
80000110:	02f71c63          	bne	a4,a5,80000148 <format_to_str+0xd0> # if not 'd', fall through to fmt++

80000114:	fe442783          	lw	a5,-28(s0)  # a5 = args
80000118:	00478713          	addi	a4,a5,4 # a4 = args + 4
8000011c:	fee42223          	sw	a4,-28(s0)  # store updated args to stack
80000120:	0007a483          	lw	s1,0(a5)    # s1 = *(int*)arg

80000124:	fec42503          	lw	a0,-20(s0)  # a0 = out
80000128:	454000ef          	jal	8000057c <strlen>
8000012c:	00050713          	mv	a4,a0       # a4 = strlen(out)
80000130:	fec42783          	lw	a5,-20(s0)  # a5 = out
80000134:	00e787b3          	add	a5,a5,a4    # a5 = out + strlen(out) -> end of out buffer
80000138:	00a00613          	li	a2,10       # a2 = 10 -> base 10
8000013c:	00078593          	mv	a1,a5       # a1 = out + strlen(out)
80000140:	00048513          	mv	a0,s1       # a0 = integer value
80000144:	128000ef          	jal	8000026c <itoa>
```

**4. The epilogue**
- The stack grows upward and pop the stored local values
```
80000168:	01c12083          	lw	ra,28(sp)
8000016c:	01812403          	lw	s0,24(sp)
80000170:	01412483          	lw	s1,20(sp)
80000174:	02010113          	addi	sp,sp,32
80000178:	00008067          	ret
```

## Complete format_to_str()
```C
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
```
- Scans `fmt` one byte at a time
- If byte isn't `%`, append char to output
- If byte is `%`, advances `fmt` and dispatches by specifier:
    - `%s`: take next arg as string, append it
    - `%d`: take next arg as an int, append in base-10
    - `%c`: take next arg as an int, keep only one char, append it
    - `%x`: take next arg as unsigned int, convert to hex (lowercase), append
    - `%u`: take next arg as unsigned int, convert into decimal using /10 & %10, append
    - `%p`: take next arg as unsigned long, turn into 0x + hex digits (address format)
    - `%llu`: take next arg as unsigned long long, convert into decimal using /10 & %10, append
- After loop: always append `\n`


# Part 2: Dynamic Memory Allocation
## Modify printf()
**1. Goal: hold longer string**
- The original `printf()` only allocates 512 bytes for output
- When length of output string is unknown or longer than 512 bytes, we need to allocate larger memory flexibly
```C
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
```
**2. Helper function**
- Helps to compute the length of types supported by the current `printf()`
```C
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
```
**3. Test with stdlib**
- Use  `malloc()` and `free()` from `stdlib` to test
- The `printf_da()` works correctly for provided test cases and longer string
```C
    printf_da("mix: %s %d %u %x %c %p %llu",
                "ok", -42, 4294967295u, 0xBEEF, 'Z', (void*)msg, 1234567890123456789ULL);
    // mix: ok -42 4294967295 beef Z 0x80002cf4 1234567890123456789

    int len = 700;
    char long_str[len + 1];
    for (int i = 0; i < len; i++) { long_str[i] = 'A' + (i % 26);  };
    long_str[len] = '\0';

    size_t L = strlen(long_str);
    printf_da("len = %u last = %c", (unsigned)L, long_str[L - 1]);
    // len = 700 last = X
```

