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

#include <linux/kprobes.h>
#include "fork.h"
#include "../probes.h"
#include "../init.h"

static struct jprobe probe_do_fork;

unsigned long sandboxer_on_do_fork(struct pt_regs* regs, u32 arch) {
    jprobe_return();
    return 0; /* never called */
}

int init_fork_tag(void) {
    probe_do_fork.kp.symbol_name = "_do_fork";
    probe_do_fork.entry = sandboxer_on_do_fork;
    initlib_push_errmsg(init_or_shutdown_jprobe, &probe_do_fork,
                        "sandboxer: failed to initialize probe on _do_fork");
    return 0;
}
