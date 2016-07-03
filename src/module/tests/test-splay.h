//  Sandboxer, kernel module sandboxing stuff
//  Copyright (C) 2016  Sayutin Dmitry, Alferov Vasiliy
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

#ifndef SANDBOXER_TEST_SPLAY_H_
#define SANDBOXER_TEST_SPLAY_H_

/**
  * Performs a splay tree test. 
  * Suitable for using with initlib.
  *
  * tests performed only with initlib_mode == 1, and errno returned on error.
  * Second argument is ignored.
  *
  * Please note that this tests general splay tree health and its containment, not threading health or
  * general performance.
  */
int random_splay_test(int initlib_mode, __attribute__((unused)) void *ignored);

/**
  * init_splay_threading_test(1, NULL):
  *
  * Creates file /proc/sbsplaytest.
  * Any process can write from this file and read from this file. Since this feature is designed only to test
  * some module subsystems, the testing processes are supposed to follow the following format of interaction
  * with module through it. Otherwise, the module behavior is undefined. It is strictly NOT recomended to 
  * launch that subsystem if ANY sandboxing is supposed to be done.
  *
  * This file provides an interface of interaction with a 'set' of integers that is stored inside the module.
  * 'set' supports inserting an integer if it is not already there, erasing the integer from set if it is 
  * already there and answering whether the integer is inside of module or not. 
  * If process want to perform a query to the structure, it should write a single string ending with '\n' 
  * to that file. The query of first type should be represented as '+ x\n', where x is an integer. x must not
  * violate the range (-2 * 10^9, 2 * 10^9), inclusively. When the query is proceeded, x is added to the set.
  * The query of second type should be represented as '- x\n', where x is an integer (same as previous). 
  * After proceeding this query, there wouldn't be such element in the set. All queries of first and 
  * second type doesn't receive any response. The query of third type should be represented as '? x\n',
  * simillary. After that the response to that query is stored. It can be read from that file in any time.
  * Due to some internal constraints, only last response for concrete PID can be read. The response is simply
  * string '0' if the element exists and '1' otherwise.
  *
  *
  * init_splay_threading_test(0, NULL):
  *
  * Removes file /proc/sbsplaytest and frees the memory used by 'set'.
  */
int init_splay_threading_test(int initlib_mode, __attribute__((unused)) void *ignored);

#endif
