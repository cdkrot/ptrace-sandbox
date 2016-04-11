//  Demos to test sandboxing stuff on
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void recursive(int cnt) {
    if (cnt != 0) {
        fork();
        recursive(cnt - 1);
    }
}

int main(int argc, char** argv) {
    assert(argc == 2);
    recursive(atoi(argv[1]));
    printf("Done\n");
}
