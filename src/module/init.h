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

int initlib_init(void); /* Initializes the initializing system. To initialize module of module while
                          initializing module. Returns errno. */

int initlib_push(int (*init_func)(int, void *), void *data);
int initlib_push_errmsg(int (*init_func)(int, void *), void *data, const char *errmsg);
void initlib_pop_all(void);

#endif //SANBOXER_INIT_H_
