#include <stdio.h>
#include <unistd.h>

int main()
{
    FILE* f = fopen("/proc/sandboxer", "w");
    if (!f)
    {
        printf("Unable to open file\n");
        return -1;
    }
    fprintf(f, "1");
    fclose(f);

    f = fopen("/proc/sandboxer", "r");
    int d;
    fscanf(f, "%d", &d);
    fclose(f);

    printf("%d\n", d);
    return 0;
}
