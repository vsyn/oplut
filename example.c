#include "oplut.h"

#include <stdio.h>

int cb_def(void *od, void *ud, oplut_op val) {
  return 0;
}

int print_op(void *od, void *ud, oplut_op val) {
  return printf("%s %s 0x%x\n", (char *)ud, (char *)od, val);
}

static struct oplut ops[] = {
    /* callback, user data, opcode, opcode fixed-mask */
    {print_op, "nop",          0x0000, 0xffff},
    {print_op, "movw",         0x0100, 0xff00},
    {print_op, "muls",         0x0200, 0xff00},
    {print_op, "mulsu",        0x0300, 0xff88},
    {print_op, "fmul",         0x0308, 0xff88},
    {print_op, "fmuls",        0x0380, 0xff88},
    {print_op, "fmulsu",       0x0388, 0xff88},
    {print_op, "cpc",          0x0400, 0xfc00}
    /* ... */
  };

int main() {
  struct oplut_conf conf = {.def = {.cb = cb_def, .od = 0,},
                          .size = sizeof(ops) / sizeof(ops[0]),
                          .max_field_size = 4};

  struct oplut_od *od = oplut_new(ops, &conf);
  if (od == 0) {
    return -1;
  }

  /* calls print_op, printing "op: muls 0x234" */
  return oplut(od, "op: ", 0x0234);
}
