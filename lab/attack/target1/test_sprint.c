#include <stdio.h>
#include <math.h>

int main()
{
    long val = 0x59b997fa;
    char cbuf[110];
    char *s = cbuf + random() % 100;
    sprintf(s, "%.8x", val);
    printf("%s\n", s);
    return 0;
}