AA64 = /opt/cross/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-elf/bin/aarch64-elf-
AA64CFLAGS = -Os -Wall -std=c99 -fno-builtin -fno-common -fno-pic -fno-unwind-tables -fomit-frame-pointer -fno-dwarf2-cfi-asm -mgeneral-regs-only

all: linux.macho

linux.macho: machopack preboot.bin Image apple-m1-j274.dtb
	./machopack $@ preboot.bin@0x803040000 Image@0x803080000 apple-m1-j274.dtb@0x803060000

preboot.bin: preboot.elf
	$(AA64)objcopy -Obinary $^ $@

preboot.elf: preboot.o preboot-c.o tunable.o dtree-dict.o dtree.o adtree.o libc.o printf.o unscii-16.o preboot.ld
	$(AA64)gcc -Wl,-T,preboot.ld -Wl,--build-id=none -nostdlib -o $@ preboot.o preboot-c.o tunable.o dtree-dict.o dtree.o adtree.o printf.o libc.o unscii-16.o

preboot.o: preboot.S
	$(AA64)gcc -c $^

preboot-c.o: preboot-c.c dtree.h adtree.h libc.h tunable.h
	$(AA64)gcc $(AA64CFLAGS) -c $<
tunable.o: tunable.c dtree.h libc.h tunable.h
	$(AA64)gcc $(AA64CFLAGS) -c $<
dtree-dict.o: dtree-dict.c dtree.h libc.h
	$(AA64)gcc $(AA64CFLAGS) -c $<
dtree.o: dtree.c dtree.h libc.h
	$(AA64)gcc $(AA64CFLAGS) -c $<
adtree.o: adtree.c adtree.h dtree.h libc.h
	$(AA64)gcc $(AA64CFLAGS) -c $<
libc.o: libc.c libc.h
	$(AA64)gcc $(AA64CFLAGS) -c $<
printf.o: printf.c libc.h
	$(AA64)gcc $(AA64CFLAGS) -c $<
unscii-16.o: unscii-16.c
	$(AA64)gcc $(AA64CFLAGS) -c $<

clean:
	rm -f machopack linux.macho preboot.bin preboot.elf preboot.o preboot-c.o tunable.o dtree-dict.o dtree.o adtree.o libc.o printf.o unscii-16.o
