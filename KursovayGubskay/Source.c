#define _CRT_SECURE_NO_WARNINGS
#include <locale.h>
#include <stdio.h>
#include <math.h>

int analiz(const char* name);

int main()
{
    setlocale(LC_ALL, ".utf-8");

    analiz("DSK");
    analiz("API");
    analiz("NET");

    return 0;
}

int analiz(const char* name)
{
    double x[6];
    double q[6];
    int run = 0;
    int warning = 0;

    printf("\n%s:\n", name);

    for (int i = 1; i <= 5; i++)
    {
        printf("X[%d] = ", i);

        if (scanf("%lf", &x[i]) != 1 || x[i] <= 0)
        {
            printf("Ошибка ввода!\n");
            return 1;
        }
    }

    q[4] = log(x[4] / x[1]);
    q[5] = log(x[5] / x[2]);

    printf("\nQ = {%.4f, %.4f}\n\n", q[4], q[5]);


    if (q[4] > 1.0)
    {
        run++;
        printf("Q[4] = %.4f норма для двух измерений\n", q[4]);
    }
    else
    {
        run = 0;
        printf("Q[4] = %.4f НОРМА\n", q[4]);
    }

    if (q[5] > 1.0)
    {
        run++;

        if (run >= 2)
        {
            warning = 1;
            printf("Q[5] = %.4f ПРЕДУПРЕЖДЕНИЕ\n", q[5]);
        }
        else
        {
            printf("Q[5] = %.4f норма для двух измерений\n", q[5]);
        }
    }
    else
    {
        run = 0;
        printf("Q[5] = %.4f НОРМА\n", q[5]);
    }


    if (warning)
        printf("Состояние %s = ПРЕДУПРЕЖДЕНИЕ\n", name);
    else
        printf("Состояние %s = НОРМА\n", name);

    printf("\n");

    return 0;
}