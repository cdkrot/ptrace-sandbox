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

#ifndef SANDBOXER_PROPERTIES_H_
#define SANDBOXER_PROPERTIES_H_

/*
 * Pushes all properties to proc module. Initlib-friendly. Should be called in initialization time but 
 * _after_ initialization proc subsystem.
 **/
int init_or_shutdown_properties(int initlib_mode, __attribute__((unused)) void *ignored);

#endif //PROPERTIES_H_
