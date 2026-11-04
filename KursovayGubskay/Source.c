#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <locale.h>
#include <math.h>

int analiz(const char* name);

int main(void)
{
    setlocale(LC_ALL, "RUS");
    analiz("DSK");
    analiz("NET");
    analiz("API");

    return 0;
}

int analiz(const char* name)
{
    printf("%s:\n", name);

    double x1, x2, x3, x4, x5;
    double q1, q2, q3, q4, q5;

    printf("X[1] = ");
    scanf("%lf", &x1);
    printf("X[2] = ");
    scanf("%lf", &x2);
    printf("X[3] = ");
    scanf("%lf", &x3);
    printf("X[4] = ");
    scanf("%lf", &x4);
    printf("X[5] = ");
    scanf("%lf", &x5);


    q1 = 0.0;
    q2 = (x1 != 0.0) ? log(x2 / x1) : 0.0;
    q3 = (x2 != 0.0) ? log(x3 / x2) : 0.0;
    q4 = (x3 != 0.0) ? log(x4 / x3) : 0.0;
    q5 = (x4 != 0.0) ? log(x5 / x4) : 0.0;

    if (q2 > q1 && q2 > q3)
        printf("Q[2] = %.2f юмнлюкхъ\n", q2);
    else
        printf("Q[2] = %.2f мнплю\n", q2);


    if (q3 > q2 && q3 > q4)
        printf("Q[3] = %.2f юмнлюкхъ\n", q3);
    else
        printf("Q[3] = %.2f мнплю\n", q3);

    if (q4 > q3 && q4 > q5)
        printf("Q[4] = %.2f юмнлюкхъ\n", q4);
    else
        printf("Q[4] = %.2f мнплю\n", q4);

    printf("\n");
    return 0;
}