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

#include <linux/kprobes.h>
#include "init.h"
#include "probes.h"

int sandboxer_init_probes(void) {
    // code here.
    return 0;
}

int init_or_shutdown_kprobe(int is_init, void *kp) {
    if (is_init)
        return register_kprobe((struct kprobe*) kp);
    unregister_kprobe((struct kprobe*) kp);
    return 0;
}

int init_or_shutdown_jprobe(int is_init, void *jp) {
    if (is_init)
        return register_jprobe((struct jprobe*) jp);
    unregister_jprobe((struct jprobe*) jp);
    return 0;
}

int init_or_shutdown_kretprobe(int is_init, void *krp) {
    if (is_init)
        return register_kretprobe((struct kretprobe*) krp);
    unregister_kretprobe((struct kretprobe*) krp);
    return 0;
}
