# RUN: llvm-mc %s -triple riscv64 -mattr=+c -M no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc -filetype=obj -triple=riscv64 -mattr=+c < %s \
# RUN:     | llvm-objdump --no-print-imm-hex -M no-aliases -d -r - \
# RUN:     | FileCheck --check-prefix=CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc %s -triple riscv64 -mattr=+zca -M no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc -filetype=obj -triple=riscv64 -mattr=+zca < %s \
# RUN:     | llvm-objdump --no-print-imm-hex --mattr=+zca -M no-aliases -d -r - \
# RUN:     | FileCheck --check-prefix=CHECK-ASM-AND-OBJ %s

# CHECK-ASM-AND-OBJ: c.slli zero, 63
# CHECK-ASM: encoding: [0x7e,0x10]
c.slli x0, 63
