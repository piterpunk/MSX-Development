sdcc --code-loc 0x180 --data-loc 0 -mz80 --disable-warning 196 --no-std-crt0 crt0_msxdos_advanced.rel fusion-c\include\printf_simple.rel fusion-c\include\putchar.rel base64.lib asm.lib hget.c
hex2bin -e com hget.ihx