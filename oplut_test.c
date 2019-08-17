/* Copyright 2019 Julian Ingram
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "oplut.h"

#include <stdio.h>

#define SEED (17)

size_t MAX_LUT_SIZE = 256;
int MAX_OP_SIZE = 2;
unsigned char MAX_FIELD_SIZE = 4;
unsigned int LUT_REPS = 10000;
unsigned int OP_REPS = 10000;

static int cb_def(void *od, void *ud, oplut_op op) { return 0; }

static int cb_odd(void *od, void *ud, oplut_op op) {
  unsigned char *i = od;
  return (*i) / 2;
}

static int cb_even(void *od, void *ud, oplut_op op) {
  unsigned char *i = od;
  return *i;
}

static oplut_op get_random_op(unsigned char max_size) {
  unsigned char i;
  oplut_op op = 0;
  for (i = 0; i < max_size; ++i) {
    op <<= CHAR_BIT;
    op |= rand() % UCHAR_MAX;
  }
  return op;
}

static ssize_t get_lut_index(struct oplut *ops, size_t size, oplut_op op) {
  size_t i;
  for (i = 0; i < size; ++i) {
    if ((op & ops[i].mask) == (ops[i].val & ops[i].mask)) {
      return i;
    }
  }
  return -1;
}

static unsigned char check_rc(oplut_op val, oplut_op mask, int rc) {
  val &= mask;
  if ((val % 2) == 0) {
    if (rc != (val & 0xff)) {
      return -1;
    }
  } else {
    if (rc != ((val & 0xff) / 2)) {
      return -1;
    }
  }
  return 0;
}

static int test(size_t *tests_completed) {
  unsigned int lut_size = (rand() % MAX_LUT_SIZE) + 1;

  struct oplut *ops = calloc(lut_size, sizeof(*ops));
  unsigned char *ods = malloc(lut_size);

  unsigned int i;
  for (i = 0; i < lut_size; ++i) {
    /* populate the lut */
    oplut_op val = get_random_op(MAX_OP_SIZE);
    oplut_op mask = get_random_op(MAX_OP_SIZE);

    ops[i].val = val;
    ops[i].mask = mask;
    ods[i] = val & mask & 0xff;

    ops[i].rt.cb = (ods[i] % 2) ? &cb_odd : &cb_even;
    ops[i].rt.od = &ods[i];
  }

  struct oplut_conf conf = {.def = {.cb = cb_def, .od = 0},
                            .size = lut_size,
                            .max_field_size = MAX_FIELD_SIZE};

  struct oplut_od *od = oplut_new(ops, &conf);
  if (od == 0) {
    struct oplut_alloc_size as = oplut_alloc_size(ops, &conf);
    free(ops);
    free(ods);
    return ((as.ods == 0) && (as.ops == 0)) ? 0 : -1;
  }

  ++(*tests_completed);

  int rc;
  for (i = 0; i < OP_REPS; ++i) {
    oplut_op val;
    oplut_op mask;
    ssize_t lut_index = -1;
    if (rand() < (RAND_MAX / 2)) {
      val = get_random_op(MAX_OP_SIZE);
    } else {
      lut_index = rand() % lut_size;
      val = ops[lut_index].val;
      mask = ops[lut_index].mask;
    }

    rc = oplut(od, 0, val);
    if (rc < 0) {
      oplut_delete(od);
      free(ops);
      free(ods);
      return rc;
    }

    if (lut_index >= 0) {
      if (check_rc(val, mask, rc) != 0) {
        printf("LUT check failed 0\n");
        oplut_delete(od);
        free(ops);
        free(ods);
        return -1;
      }
    } else {
      lut_index = get_lut_index(ops, lut_size, val);
      if (lut_index >= 0) {
        mask = ops[lut_index].mask;
        if (check_rc(val, mask, rc) != 0) {
          printf("LUT check failed 1\n");
          oplut_delete(od);
          free(ops);
          free(ods);
          return -1;
        }
      } else {
        if (rc != 0) {
          printf("not in LUT, something else went wrong\n");
          oplut_delete(od);
          free(ops);
          free(ods);
          return rc;
        }
      }
    }
  }

  oplut_delete(od);
  free(ops);
  free(ods);

  return 0;
}

int main() {
  srand(SEED);

  size_t tests_completed = 0;

  unsigned int i;
  for (i = 0; i < LUT_REPS; ++i) {
    int rc = test(&tests_completed);
    if (rc != 0) {
      printf("test failed on rep: %u\n", i);
      return rc;
    }
  }
  printf("%lu tests completed successfully\n", tests_completed);
  return 0;
}
