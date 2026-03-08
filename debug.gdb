set architecture riscv:rv64
target remote localhost:25501
add-symbol-file user/_proc_2 0x0
b proc_2.c:53
c
