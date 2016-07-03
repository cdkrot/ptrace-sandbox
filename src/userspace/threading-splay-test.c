//  threading-splay-test.c: utilite for testing sandboxer module through
//  /proc/sbsplay_test file
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>


#define WRITE_QINT(FORMAT)                \
    f = fopen("/proc/sbsplay_test", "w"); \
    assert(f);                            \
    fprintf(f, FORMAT, qint);             \
    fclose(f);

int main(int argc, char** argv) {
    int qtype, qint, ans, maxn;
    FILE *f;

    if (argc == 1)
        /* If we set up this value to 2e9 it will rapidly eat out all the memory! */
        maxn = 2 * 1000 * 1000;
    else
        assert(sscanf(argv[1], "%d", &maxn) == 1);

    printf("maxn=%d\n", maxn);

    /* cdkrot, I love that idea! */
    usleep((rand() % 1000 + 100) * 1000);

    srand(time(0));
    for (;;) {
        qtype = rand() % 3;
        qint = rand() % maxn;
        if (qtype == 0) {
            WRITE_QINT("+ %d")
        } else if (qint == 1) {
            WRITE_QINT("- %d")
        } else {
            WRITE_QINT("? %d")

            f = fopen("/proc/sbsplay_test", "r");
            assert(f);
            fscanf(f, "%d", &ans);
            fclose(f);

            printf("got answer %d\n", ans);
        }
    }
    return 0;
}
