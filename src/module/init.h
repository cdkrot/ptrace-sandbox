//  Sandboxer, kernel module sandboxing stuff
//  Copyright (C) 2016  Alferov Vasiliy, Sayutin Dmitry
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef SANDBOXER_INIT_H_
#define SANDBOXER_INIT_H_

/* Initializes the initialization system. To initialize module of module while initializing module.
 * Returns errno.
 */
int initlib_init(void);

/* Calls init_func(1, data) and adds init_func(0, data) to list of deinitialization functions.
 * Failure of init_func also does all deinitializations. 
 * Returns result of init_func or -ENOMEM in case of list entry structure allocation failure.
 */
int initlib_push(int (*init_func)(int, void *), void *data);

/* Same as previous but prints errmesg in case of failure of init_func. */
int initlib_push_errmsg(int (*init_func)(int, void *), void *data, const char *errmsg);

/**
  * The most advanced initlib push overload.
  * It allows to specify errmsg as in initlib_push_errmsg.
  *
  * Also allows to not treat init failure as fatal -- suitable if you have fallback alternative.
  *
  * If init was marked as "not fatal" and init failed, than:
  * Current initialization stack is _not_ changed (not adding new entry and not undoing old).
  * the init_func's errno is returned.
  *
  * In other cases function behaviour is similar to other initlib_push functions.
  */
int initlib_push_advanced(int (*init_func)(int, void *), void *data, const char *errmsg, bool is_fatal);

/* Calling all deinitialization functions added by initlib_push functions.. */
void initlib_pop_all(void);

#endif //SANBOXER_INIT_H_
