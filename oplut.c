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

static int _oplut(void *_od, void *ud, oplut_op val) {
  struct oplut_od *od = (struct oplut_od *)_od;
  size_t lut_index = (val >> od->field.shift) & od->field.mask;
  struct oplut_rt rt = od->lut[lut_index];
  return rt.cb(rt.od, ud, val);
}

int oplut(struct oplut_od *od, void *ud, oplut_op val) { return _oplut(od, ud, val); }

/* checks if the op does match and isn't just the nearest guess */
static int check(void *od, void *ud, oplut_op val) {
  struct oplut *op = od;
  return (((val & op->mask) != (op->val & op->mask)) || (op->rt.cb == 0))
             ? 0
             : op->rt.cb(op->rt.od, ud, val);
}

struct glob {
  oplut_op mask;
  oplut_op val;
};

static struct oplut_field get_field(struct oplut *ops, struct oplut_conf *conf,
                                    struct glob glob) {
  struct oplut_field field = {.lut_size = 2, .mask = (oplut_op)-1, .shift = 0};
  size_t i;
  for (i = 0; i < conf->size; ++i) {
    if ((ops[i].val & glob.mask) != glob.val) {
      continue;
    }
    field.mask &= (ops[i].mask & ~glob.mask);
  }

  if (field.mask == 0) {
    return field;
  }

  while ((field.mask & 1) == 0) {
    field.mask >>= 1;
    ++field.shift;
  }

  i = 1;
  while (((field.mask & field.lut_size) != 0) && (i < conf->max_field_size)) {
    field.lut_size <<= 1;
    ++i;
  }

  field.mask &= (field.lut_size - 1);
  return field;
}

static struct oplut_alloc_size alloc_size(struct oplut *ops,
                                          struct oplut_conf *conf,
                                          struct glob glob) {

  struct oplut_alloc_size as = {0};
  struct oplut_field field = get_field(ops, conf, glob);

  if (field.mask == 0) {
    return as; /* indistinguishable opcodes */
  }

  struct glob new_glob = {.mask = glob.mask | (field.mask << field.shift)};

  /* allocate a list of next level opcodes */
  as.ops = field.lut_size;
  as.ods = 1;

  size_t i;
  for (i = 0; i < conf->size; ++i) {
    if ((ops[i].val & glob.mask) != glob.val) {
      continue;
    }
    struct oplut *opi = &ops[i];
    size_t lut_indexi = (opi->val >> field.shift) & field.mask;
    size_t count = 0;
    size_t j;
    /* must keep constant memory */
    for (j = 0; j < i; ++j) {
      if ((ops[j].val & glob.mask) != glob.val) {
        continue;
      }
      struct oplut *opj = &ops[j];
      size_t lut_indexj = (opj->val >> field.shift) & field.mask;
      if (lut_indexi == lut_indexj) {
        ++count;
      }
    }
    if (count == 1) {
      ++as.ods;
      new_glob.val = opi->val & new_glob.mask;
      struct oplut_alloc_size nas = alloc_size(ops, conf, new_glob);
      if (nas.ops == 0) {
        /* indistinguishable opcodes */
        return nas;
      }
      as.ops += nas.ops;
      as.ods += nas.ods;
    }
  }
  return as;
}

struct oplut_alloc_size oplut_alloc_size(struct oplut *ops,
                                         struct oplut_conf *conf) {
  struct glob glob = {.val = 0, .mask = 0};
  return alloc_size(ops, conf, glob);
}

static struct oplut_alloc_size init(struct oplut_od *ods, struct oplut *ops,
                                    struct oplut_conf *conf, struct glob glob) {
  struct oplut_field field = get_field(ops, conf, glob);
  struct glob new_glob = {.mask = glob.mask | (field.mask << field.shift)};
  struct oplut_alloc_size as = {.ops = field.lut_size, .ods = 1};
  size_t i;
  for (i = 0; i < conf->size; ++i) {
    if ((ops[i].val & glob.mask) != glob.val) {
      continue;
    }
    struct oplut *op = &ops[i];
    size_t lut_index = (op->val >> field.shift) & field.mask;
    struct oplut_rt *next_op = &ods->lut[lut_index];
    if (next_op->cb == 0) { /* 1st time - set to user data */
      if (new_glob.mask != op->mask) {
        /* not in a unique position given masks */
        next_op->od = op; /* references the input ops list */
        next_op->cb = &check;
      } else {
        *next_op = op->rt;
      }
    } else if (next_op->cb != &_oplut) { /* 2nd time - requires another stage */
      next_op->od = ods + as.ods;
      ++as.ods;
      ((struct oplut_od *)next_op->od)->lut = ods->lut + as.ops;
      next_op->cb = &_oplut;
      new_glob.val = op->val & new_glob.mask;
      struct oplut_alloc_size nas = init(next_op->od, ops, conf, new_glob);
      as.ops += nas.ops;
      as.ods += nas.ods;
    }
  }

  for (i = 0; i < field.lut_size; ++i) {
    if (ods->lut[i].cb == 0) {
      ods->lut[i].cb = conf->def.cb;
      if (ods->lut[i].od == 0) {
        ods->lut[i].od = conf->def.od;
      }
    }
  }
  ods->field = field;
  return as;
}

void oplut_init(struct oplut_od *ods, struct oplut *ops,
                struct oplut_conf *conf) {
  struct glob glob = {.val = 0, .mask = 0};
  (void)init(ods, ops, conf, glob);
}

void oplut_delete(struct oplut_od *od) {
  free(od->lut);
  free(od);
}

struct oplut_od *oplut_new(struct oplut *ops, struct oplut_conf *conf) {
  struct oplut_alloc_size as = oplut_alloc_size(ops, conf);
  if (as.ops == 0) {
    return 0;
  }
  struct oplut_od *ods = calloc(as.ods, sizeof(*ods));
  if (ods == 0) {
    return 0;
  }
  ods->lut = calloc(as.ops, sizeof(*ods->lut));
  if (ods->lut == 0) {
    free(ods);
    return 0;
  }
  oplut_init(ods, ops, conf);
  return ods;
}
