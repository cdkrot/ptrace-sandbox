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

#ifndef SANDBOXER_PROBES_H_
#define SANDBOXER_PROBES_H_


/**
  * Sets all required probes and registers them in initlib.
  * returns errno.
  */
int sandboxer_init_probes(void);

/**
  * initlib helper for kprobe registration,
  * kp is pointer to initialized with data struct kprobe.
  */
int init_or_shutdown_kprobe(int is_init, void *kp);

/**
  * initlib helper for kprobe registration,
  * jp is pointer to initialized with data struct jprobe.
  */
int init_or_shutdown_jprobe(int is_init, void *jp); // *jp - struct jprobe

/**
  * initlib helper for kprobe registration,
  * krp is pointer to initialized with data struct kretprobe.
  */
int init_or_shutdown_kretprobe(int is_init, void *krp);

#endif
