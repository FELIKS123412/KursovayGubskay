#include <stdio.h>
#include <locale.h>
#include <math.h>

#define POINTS_COUNT 5
#define K_PARAM 3

typedef struct {
    int time;
    int type;
    int first;
    int second;
} Incident;

int main(void) {
    setlocale(LC_CTYPE, "Russian");

    float dsk[POINTS_COUNT] = { 45.5f, 47.15f, 46.5f, 48.15f, 52.9f };
    float net[POINTS_COUNT] = { 55.6f, 53.6f, 57.0f, 55.0f, 53.0f };
    float api[POINTS_COUNT] = { 33.9f, 33.1f, 32.4f, 31.6f, 30.9f };

    float q_dsk[POINTS_COUNT] = { 0.0f };
    float q_net[POINTS_COUNT] = { 0.0f };
    float q_api[POINTS_COUNT] = { 0.0f };

    int state_dsk[POINTS_COUNT] = { 0 };
    int state_net[POINTS_COUNT] = { 0 };
    int state_api[POINTS_COUNT] = { 0 };

    Incident incidents[POINTS_COUNT];
    int incident_count = 0;

    int i;

    printf("Êîíòðîëü îáëà÷íîé èíôðàñòðóêòóðû (Âàðèàíò 9, ÊÒ1)\n\n");

    printf("Èñõîäíûå äàííûå òåëåìåòðèè:\n");
    printf("Îòñ÷åò\tDSK\tNET\tAPI\n");
    for (i = 0; i < POINTS_COUNT; i++) {
        printf("%d\t%.2f\t%.1f\t%.1f\n", i + 1, dsk[i], net[i], api[i]);
    }
    printf("\n");

    for (i = K_PARAM; i < POINTS_COUNT; i++) {
        if (dsk[i - K_PARAM] > 0.0f && dsk[i] > 0.0f) {
            q_dsk[i] = logf(dsk[i] / dsk[i - K_PARAM]);
        }
        if (net[i - K_PARAM] > 0.0f && net[i] > 0.0f) {
            q_net[i] = logf(net[i] / net[i - K_PARAM]);
        }
        if (api[i - K_PARAM] > 0.0f && api[i] > 0.0f) {
            q_api[i] = logf(api[i] / api[i - K_PARAM]);
        }
    }

    printf("Ëîãàðèôì îòíîøåíèÿ Q ïðè k = %d:\n", K_PARAM);
    printf("Îòñ÷åò\tQ(DSK)\t\tQ(NET)\t\tQ(API)\n");
    for (i = K_PARAM; i < POINTS_COUNT; i++) {
        printf("%d\t%+7.4f\t\t%+7.4f\t\t%+7.4f\n", i + 1, q_dsk[i], q_net[i], q_api[i]);
    }
    printf("\n");

    for (i = 0; i < POINTS_COUNT; i++) {
        if (i - 1 >= K_PARAM && i + 1 < POINTS_COUNT) {
            if (q_dsk[i] > q_dsk[i - 1] && q_dsk[i] > q_dsk[i + 1]) {
                state_dsk[i] = 1;
            }
            if (q_net[i] > q_net[i - 1] && q_net[i] > q_net[i + 1]) {
                state_net[i] = 1;
            }
            if (q_api[i] > q_api[i - 1] && q_api[i] > q_api[i + 1]) {
                state_api[i] = 1;
            }
        }

        int prob_count = state_dsk[i] + state_net[i] + state_api[i];
        if (prob_count >= 2) {
            incidents[incident_count].time = i + 1;
            incidents[incident_count].type = 1;
            if (state_dsk[i] && state_net[i]) {
                incidents[incident_count].first = 1;
                incidents[incident_count].second = 2;
            } else if (state_dsk[i] && state_api[i]) {
                incidents[incident_count].first = 1;
                incidents[incident_count].second = 3;
            } else if (state_net[i] && state_api[i]) {
                incidents[incident_count].first = 2;
                incidents[incident_count].second = 3;
            }
            incident_count++;
        }
    }

    printf("Ñîñòîÿíèå êîìïîíåíòîâ (ïðàâèëî ëîêàëüíîé àíîìàëèè):\n");
    printf("Îòñ÷åò\tDSK\t\tNET\t\tAPI\n");
    for (i = 0; i < POINTS_COUNT; i++) {
        printf("%d\t%s\t%s\t%s\n",
            i + 1,
            state_dsk[i] ? "ÀÍÎÌÀËÈß" : "ÍÎÐÌÀ  ",
            state_net[i] ? "ÀÍÎÌÀËÈß" : "ÍÎÐÌÀ  ",
            state_api[i] ? "ÀÍÎÌÀËÈß" : "ÍÎÐÌÀ  ");
    }
    printf("\n");

    printf("Æóðíàë èíöèäåíòîâ (ïðàâèëî Áîëüøèíñòâî):\n");
    if (incident_count == 0) {
        printf("Èíöèäåíòîâ íå îáíàðóæåíî.\n");
    } else {
        for (i = 0; i < incident_count; i++) {
            printf("Îòñ÷åò %d: çàôèêñèðîâàí èíöèäåíò (òèï %d, ïðîáëåìíûå óçëû: %d è %d)\n",
                incidents[i].time, incidents[i].type, incidents[i].first, incidents[i].second);
        }
    }

    return 0;
}
