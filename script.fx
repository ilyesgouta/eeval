main (int a, int b)
{
    int i, x;
    int f;

    i = a;
    x = b;
    f = 1.0;

    while (i > 0) {
        f = f * x;
        x = x * i;
        i = i - 1;
    }
}
