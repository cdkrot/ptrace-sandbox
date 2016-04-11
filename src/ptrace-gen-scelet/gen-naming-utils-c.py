#!/usr/bin/python3

#  Different utils for getting friendly name of syscalls, signals, etc.
#  Copyright (C) 2016  Sayutin Dmitry, Vasiliy Alferov
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.


import sys

def gen_inner_part(lst, table_sz, guards=True):
    print("    static int initialized = 0;")
    print("    static const char* names[" + str(table_sz) + "] = {0};")
    print("    if (!initialized) {")
    print("        initialized = 1;")
    for (symbol, friendly) in lst:
        if guards:
            print("#ifdef " + symbol)
        print("        if (0 <= " + symbol + " && " + symbol + " < " + str(table_sz) + ")")
        print("            names[" + symbol + "] = \"" + friendly + "\";")
        if guards:
            print("#endif")
    print("    }")
    print("    if (0 <= id && id < " + str(table_sz) + ")")
    print("        return names[id];")
    print("    return 0;")

def nopretty(a):
    return a

def pretty_sys(entry):
    if entry.startswith("SYS_"):
        return entry[4:]
    else:
        return entry

def read_list(listname, prettifier=nopretty):
    res = []
    with open(listname, "r") as f:
        for line in f:
            line = line.strip()
            if line != "":
                res.append((line, prettifier(line)))
    return res

def main():
    syscalls = read_list("syscalls.list", pretty_sys)
    signals  = read_list("signals.list")
    events   = read_list("pevents.list")
                    
    print("// This file is generated by automatic")
    print("// script, don't modify it, modify the generator instead.")
    print("// Generator can be found under src/gen-scelet/")
    print("#include \"naming_utils.h\"")
    print("#include <sys/syscall.h>")
    print("#include <sys/ptrace.h>")
    print("#include <signal.h>")
    print("")
    print("/* returns string description of syscall, or NULL */")
    print("const char* get_syscall_name(long id) {")
    gen_inner_part(syscalls, 1000)
    print("}")
    print("/* returns string description of signal, or NULL */")
    print("const char* get_signal_name(long id) {")
    gen_inner_part(signals, 100)
    print("}")
    print("/* returns string description of ptrace event, or NULL */")
    print("const char* get_ptraceevent_name(long id) {")
    gen_inner_part(events, 20, guards=False)
    print("}")
main()
