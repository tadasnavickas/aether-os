CC = gcc
CFLAGS = -Wall -Wextra -O2 -pipe -nostdlib -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -mgeneral-regs-only -MMD -Isrc
LDFLAGS = -nostdlib -static -no-pie -z max-page-size=0x1000 -T linker.ld

SRCS = $(wildcard src/*.c) $(wildcard src/*/*.c)
OBJS = $(SRCS:.c=.o)

all: aetheros.iso

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o kernel.elf

aetheros.iso: kernel.elf limine.cfg
	mkdir -p iso_root
	cp kernel.elf limine.cfg limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/
	xorriso -as mkisofs -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label iso_root -o aetheros.iso
	./limine/limine bios-install aetheros.iso
	rm -rf iso_root

run: aetheros.iso
	qemu-system-x86_64 -cdrom aetheros.iso -serial stdio -no-reboot

clean:
	rm -f $(OBJS) $(SRCS:.c=.d) kernel.elf aetheros.iso