//  Different utils for getting friendly name of syscalls, signals, etc.
//  Copyright (C) 2016  Sayutin Dmitry, Vasiliy Alferov
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

/* Returns string description of syscall, or NULL */
const char* get_syscall_name(long id);

/* returns string description of signal or NULL */
const char* get_signal_name(long id);

/* returns string description of ptraceevent or NULL */
const char* get_ptraceevent_name(long id);
