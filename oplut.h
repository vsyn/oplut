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

#include <limits.h>
#include <stdlib.h>

typedef unsigned int oplut_op;

struct oplut_rt {
  int (*cb)(void *, void *, oplut_op);
  void *od;
};

struct oplut {
  struct oplut_rt rt;
  oplut_op val;
  oplut_op mask;
};

struct oplut_field {
  size_t lut_size;
  oplut_op mask;
  unsigned char shift;
};

struct oplut_od {
  struct oplut_rt *lut;
  struct oplut_field field;
};

struct oplut_conf {
  /* this is called if the input value to the `oplut` function is not matched in
   * the input LUT */
  struct oplut_rt def;
  /* input LUT size */
  size_t size;
  /* log2 (max runtime LUT size) */
  unsigned char max_field_size;
};

struct oplut_alloc_size {
  size_t ops;
  size_t ods;
};

/* takes a `struct oplut_od` from the `oplut_create` function and an opcode
 * will call the associated callback from the ops list passed into the
 * `oplut_create` function
 * note that the ops list must still exist at this point
 * returns the return value of the callback
 */
int oplut(struct oplut_od *od, void *ud, oplut_op val);

/* mainly an internal function, but can be useful if your system doesn't have
 * dynamic memory allocation for an example of it's use, see oplut_new in
 * oplut.c
 */
struct oplut_alloc_size oplut_alloc_size(struct oplut *ops,
                                         struct oplut_conf *conf);

/* mainly an internal function, same goes for this and `oplut_alloc_size` */
void oplut_init(struct oplut_od *uds, struct oplut *ops,
                struct oplut_conf *conf);

/* deletes a `struct oplut_od` as created by `oplut_new` */
void oplut_delete(struct oplut_od *ud);

/* takes in a list of `struct oplut`s each representing an opcode and it's
 * associated callback and the size of that list
 * returns an oplut_od struct - pass this to oplut with a opcode value to
 * perform a lookup
 */
struct oplut_od *oplut_new(struct oplut *ops, struct oplut_conf *conf);
