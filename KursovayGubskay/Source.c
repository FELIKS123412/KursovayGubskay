#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <locale.h>
#include <math.h>

#define SERIES_CAP 100
#define INCIDENT_CAP 100
#define INITIAL_STEP_K 3
#define ZERO_EPS 1e-6f
#define INCIDENT_MAJORITY_CODE 1

typedef enum {
    NODE_DSK = 0,
    NODE_NET = 1,
    NODE_API = 2,
    NODE_COUNT = 3,
    NODE_NONE = -1
} NodeId;

typedef enum {
    CALC_VALID = 0,
    CALC_NO_PREV = 1,
    CALC_ZERO_DIV = 2,
    CALC_BAD_ARG = 3,
    CALC_NAN_VAL = 4
} CalcCode;

typedef enum {
    NODE_OK = 0,
    NODE_ANOMALY = 1,
    NODE_FAULT = -1,
    NODE_MISSING = -2
} NodeStatus;

typedef enum {
    SRC_EMPTY = 0,
    SRC_VARIANT9 = 1,
    SRC_MANUAL = 2
} DataSource;

typedef enum {
    CMD_EXIT = 0,
    CMD_SET_K = 1,
    CMD_LOAD_PRESET = 2,
    CMD_MANUAL_INPUT = 3,
    CMD_RUN_ANALYSIS = 4,
    CMD_SHOW_INCIDENTS = 5,
    CMD_SHOW_RAW = 6,
    CMD_INVALID = -1
} MenuCommand;

typedef struct {
    int tick;
    float value;
    float q;
    CalcCode code;
    NodeStatus status;
} Sample;

typedef struct {
    const char* tag;
    Sample items[SERIES_CAP];
    int len;
} NodeSeries;

typedef struct {
    int time;
    int type;
    int first;
    int second;
} Incident;

/* Прототипы функций */
void flush_stdin_line(void);
void init_nodes(NodeSeries nodes[NODE_COUNT]);
int fill_variant_preset(NodeSeries nodes[NODE_COUNT]);
int read_manual_telemetry(NodeSeries nodes[NODE_COUNT], int cap);
int ask_step_k(int current_k, int max_len, int has_data);
CalcCode evaluate_log_ratio(float curr_val, float prev_val, float* out_q);
NodeStatus classify_sample(const Sample items[], int pos, int len);
int process_node_series(NodeSeries* series, int step_k);
NodeStatus node_status_at_tick(const NodeSeries* series, int tick);
int majority_rule(NodeStatus dsk_st, NodeStatus net_st, NodeStatus api_st);
int detect_incidents(const NodeSeries nodes[NODE_COUNT], int max_tick, Incident log_buf[]);
int longest_series_len(const NodeSeries nodes[NODE_COUNT]);
const char* status_caption(NodeStatus st);
const char* node_tag_by_id(int id);
void render_banner(void);
void render_menu(DataSource src, int max_tick, const NodeSeries nodes[NODE_COUNT], int step_k, int analyzed);
void render_raw_series(const NodeSeries nodes[NODE_COUNT]);
void render_node_table(const NodeSeries* series, int step_k);
void render_timeline_summary(const NodeSeries nodes[NODE_COUNT], int max_tick);
void render_incident_table(const Incident log_buf[], int inc_total);

int main(void) {
    NodeSeries nodes[NODE_COUNT];
    Incident incident_log[INCIDENT_CAP];
    DataSource source = SRC_EMPTY;
    int max_tick = 0;
    int step_k = INITIAL_STEP_K;
    int analyzed = 0;
    int inc_total = 0;
    int err_total = 0;
    int cmd = CMD_INVALID;
    int scan_rc = 0;
    int idx = 0;
    int new_ticks = 0;
    int new_k = 0;

    setlocale(LC_CTYPE, "RUS");
    init_nodes(nodes);
    render_banner();

    do {
        render_menu(source, max_tick, nodes, step_k, analyzed);
        printf("> Выберите команду: ");
        scan_rc = scanf("%d", &cmd);
        if (scan_rc == EOF) {
            printf("\nВходной поток закрыт. Выход из программы.\n");
            break;
        }
        if (scan_rc != 1) {
            flush_stdin_line();
            cmd = CMD_INVALID;
        }

        switch (cmd) {
        case CMD_SET_K:
            new_k = ask_step_k(step_k, longest_series_len(nodes), source != SRC_EMPTY);
            if (new_k > 0) {
                step_k = new_k;
                analyzed = 0;
            }
            break;

        case CMD_LOAD_PRESET:
            max_tick = fill_variant_preset(nodes);
            source = SRC_VARIANT9;
            analyzed = 0;
            printf("\n[OK] Загружен контрольный набор варианта 9 (%d отсчетов, %d измерений).\n",
                max_tick, nodes[NODE_DSK].len + nodes[NODE_NET].len + nodes[NODE_API].len);
            render_raw_series(nodes);
            break;

        case CMD_MANUAL_INPUT:
            new_ticks = read_manual_telemetry(nodes, SERIES_CAP);
            if (new_ticks > 0) {
                max_tick = new_ticks;
                source = SRC_MANUAL;
                analyzed = 0;
                printf("\n[OK] Принято отсчетов с клавиатуры: %d.\n", max_tick);
                render_raw_series(nodes);
            }
            break;

        case CMD_RUN_ANALYSIS:
            if (source == SRC_EMPTY) {
                printf("\n[!] Сначала загрузите тестовый набор [2] или введите данные вручную [3].\n\n");
                break;
            }
            err_total = 0;
            for (idx = 0; idx < NODE_COUNT; idx++) {
                err_total += process_node_series(&nodes[idx], step_k);
            }
            inc_total = detect_incidents(nodes, max_tick, incident_log);
            analyzed = 1;

            printf("\n====================================================================\n");
            printf("  ОТЧЕТ ПО АНАЛИЗУ ДИНАМИКИ  (отсчетов: %d, шаг сравнения k = %d)\n",
                max_tick, step_k);
            printf("====================================================================\n");
            for (idx = 0; idx < NODE_COUNT; idx++) {
                render_node_table(&nodes[idx], step_k);
            }
            render_timeline_summary(nodes, max_tick);
            printf("\n  Всего ошибок вычисления формулы Q: %d\n", err_total);
            render_incident_table(incident_log, inc_total);
            printf("\n");
            break;

        case CMD_SHOW_INCIDENTS:
            if (source == SRC_EMPTY) {
                printf("\n[!] Данные телеметрии еще не загружены.\n\n");
                break;
            }
            if (!analyzed) {
                printf("\n[!] Расчет еще не выполнялся. Выберите пункт [4].\n\n");
                break;
            }
            render_incident_table(incident_log, inc_total);
            printf("\n");
            break;

        case CMD_SHOW_RAW:
            if (source == SRC_EMPTY) {
                printf("\n[!] Исходные ряды пусты. Выберите пункт [2] или [3].\n\n");
                break;
            }
            render_raw_series(nodes);
            break;

        case CMD_EXIT:
            printf("\nРабота программы завершена.\n");
            break;

        default:
            printf("\n[!] Неизвестная команда. Введите цифру от 0 до 6.\n\n");
            break;
        }
    } while (cmd != CMD_EXIT);

    return 0;
}

/* Очистка остатка строки в буфере стандартного ввода */
void flush_stdin_line(void) {
    int ch = 0;
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

/* Начальная инициализация массивов измерений трех узлов */
void init_nodes(NodeSeries nodes[NODE_COUNT]) {
    const char* tags[NODE_COUNT] = { "DSK", "NET", "API" };
    int idx = 0;
    int j = 0;

    for (idx = 0; idx < NODE_COUNT; idx++) {
        nodes[idx].tag = tags[idx];
        nodes[idx].len = 0;
        for (j = 0; j < SERIES_CAP; j++) {
            nodes[idx].items[j].tick = 0;
            nodes[idx].items[j].value = 0.0f;
            nodes[idx].items[j].q = 0.0f;
            nodes[idx].items[j].code = CALC_NO_PREV;
            nodes[idx].items[j].status = NODE_OK;
        }
    }
}

/* Загрузка всех 17 измерений контрольного набора варианта 9 (упорядочены по отсчетам) */
int fill_variant_preset(NodeSeries nodes[NODE_COUNT]) {
    const int dsk_ticks[7] = { 1, 2, 2, 3, 4, 4, 5 };
    const float dsk_vals[7] = { 45.5f, 48.3f, 46.0f, 46.5f, 47.0f, 49.3f, 52.9f };
    const int net_ticks[5] = { 1, 2, 3, 4, 5 };
    const float net_vals[5] = { 55.6f, 53.6f, 57.0f, 55.0f, 53.0f };
    const int api_ticks[5] = { 1, 2, 3, 4, 5 };
    const float api_vals[5] = { 33.9f, 33.1f, 32.4f, 31.6f, 30.9f };
    int i = 0;

    init_nodes(nodes);

    nodes[NODE_DSK].len = 7;
    for (i = 0; i < 7; i++) {
        nodes[NODE_DSK].items[i].tick = dsk_ticks[i];
        nodes[NODE_DSK].items[i].value = dsk_vals[i];
    }

    nodes[NODE_NET].len = 5;
    for (i = 0; i < 5; i++) {
        nodes[NODE_NET].items[i].tick = net_ticks[i];
        nodes[NODE_NET].items[i].value = net_vals[i];
    }

    nodes[NODE_API].len = 5;
    for (i = 0; i < 5; i++) {
        nodes[NODE_API].items[i].tick = api_ticks[i];
        nodes[NODE_API].items[i].value = api_vals[i];
    }

    return 5;
}

/* Интерактивный ввод телеметрии по отсчетам (по одному значению на каждый узел) */
int read_manual_telemetry(NodeSeries nodes[NODE_COUNT], int cap) {
    int count = 0;
    int i = 0;
    int rc = 0;
    float v_dsk = 0.0f;
    float v_net = 0.0f;
    float v_api = 0.0f;
    NodeSeries draft[NODE_COUNT];

    printf("\nВведите число отсчетов (1..%d): ", cap);
    rc = scanf("%d", &count);
    if (rc != 1 || count < 1 || count > cap) {
        flush_stdin_line();
        printf("[!] Ошибка: нужно целое число от 1 до %d.\n\n", cap);
        return 0;
    }

    init_nodes(draft);
    i = 0;
    while (i < count) {
        printf("  Отсчет #%d -> введите DSK NET API через пробел: ", i + 1);
        rc = scanf("%f %f %f", &v_dsk, &v_net, &v_api);
        if (rc == EOF) {
            printf("\n[!] Ввод прерван (конец потока).\n\n");
            return 0;
        }
        if (rc != 3) {
            flush_stdin_line();
            printf("  [!] Некорректный формат. Повторите ввод трех чисел.\n");
            continue;
        }
        draft[NODE_DSK].items[i].tick = i + 1;
        draft[NODE_DSK].items[i].value = v_dsk;
        draft[NODE_NET].items[i].tick = i + 1;
        draft[NODE_NET].items[i].value = v_net;
        draft[NODE_API].items[i].tick = i + 1;
        draft[NODE_API].items[i].value = v_api;
        i++;
    }

    for (i = 0; i < NODE_COUNT; i++) {
        draft[i].len = count;
        nodes[i] = draft[i];
    }

    return count;
}

/* Запрос и проверка нового шага сравнения k */
int ask_step_k(int current_k, int max_len, int has_data) {
    int candidate = 0;
    int rc = 0;

    printf("\nТекущий шаг сравнения: k = %d\n", current_k);
    printf("Введите новый шаг k (1..%d): ", SERIES_CAP - 1);
    rc = scanf("%d", &candidate);
    if (rc != 1 || candidate < 1 || candidate >= SERIES_CAP) {
        flush_stdin_line();
        printf("[!] Недопустимое значение k. Оставлено k = %d.\n\n", current_k);
        return 0;
    }

    printf("[OK] Установлен шаг сравнения k = %d.\n", candidate);
    if (has_data) {
        if (candidate >= max_len) {
            printf("     Внимание: при k >= %d ни для одной пары не хватит предыстории.\n", max_len);
        } else if (max_len < candidate + 3) {
            printf("     Внимание: для выявления локальных аномалий требуется не менее %d измерений.\n",
                candidate + 3);
        }
    }
    printf("\n");
    return candidate;
}

/* Безопасное вычисление Q = ln(curr_val / prev_val) с проверкой /0, ln<=0 и конечности */
CalcCode evaluate_log_ratio(float curr_val, float prev_val, float* out_q) {
    float ratio = 0.0f;
    float res = 0.0f;

    *out_q = 0.0f;

    if (!isfinite(curr_val) || !isfinite(prev_val)) {
        return CALC_NAN_VAL;
    }

    if (fabsf(prev_val) < ZERO_EPS) {
        return CALC_ZERO_DIV;
    }

    ratio = curr_val / prev_val;
    if (!isfinite(ratio)) {
        return CALC_NAN_VAL;
    }

    if (ratio <= 0.0f) {
        return CALC_BAD_ARG;
    }

    res = logf(ratio);
    if (!isfinite(res)) {
        return CALC_NAN_VAL;
    }

    *out_q = res;
    return CALC_VALID;
}

/* Определение состояния измерения по правилу локальной аномалии (Q[i] > Q[i-1] && Q[i] > Q[i+1]) */
NodeStatus classify_sample(const Sample items[], int pos, int len) {
    if (pos < 0 || pos >= len) {
        return NODE_FAULT;
    }

    if (items[pos].code == CALC_NO_PREV) {
        return NODE_OK;
    }

    if (items[pos].code != CALC_VALID) {
        return NODE_FAULT;
    }

    /* Крайние элементы не имеют пары соседей */
    if (pos == 0 || pos == len - 1) {
        return NODE_OK;
    }

    /* Оба соседних Q должны быть корректно вычислены */
    if (items[pos - 1].code != CALC_VALID || items[pos + 1].code != CALC_VALID) {
        return NODE_OK;
    }

    if (items[pos].q > items[pos - 1].q && items[pos].q > items[pos + 1].q) {
        return NODE_ANOMALY;
    }

    return NODE_OK;
}

/* Двухпроходная обработка временного ряда узла: 1) расчет Q, 2) классификация состояний */
int process_node_series(NodeSeries* series, int step_k) {
    int i = 0;
    int faults = 0;

    for (i = 0; i < series->len; i++) {
        if (i < step_k) {
            series->items[i].code = CALC_NO_PREV;
            series->items[i].q = 0.0f;
        } else {
            series->items[i].code = evaluate_log_ratio(
                series->items[i].value,
                series->items[i - step_k].value,
                &series->items[i].q);
            if (series->items[i].code != CALC_VALID) {
                faults++;
            }
        }
    }

    for (i = 0; i < series->len; i++) {
        series->items[i].status = classify_sample(series->items, i, series->len);
    }

    return faults;
}

/* Агрегированное состояние узла на отсчете tick (учитывает несколько замеров на одном отсчете) */
NodeStatus node_status_at_tick(const NodeSeries* series, int tick) {
    int i = 0;
    int seen = 0;
    int had_fault = 0;

    for (i = 0; i < series->len; i++) {
        if (series->items[i].tick == tick) {
            seen = 1;
            if (series->items[i].status == NODE_ANOMALY) {
                return NODE_ANOMALY;
            }
            if (series->items[i].status == NODE_FAULT) {
                had_fault = 1;
            }
        }
    }

    if (!seen) {
        return NODE_MISSING;
    }
    return had_fault ? NODE_FAULT : NODE_OK;
}

/* Логическое правило инцидента большинства: (A && B) || (A && C) || (B && C) */
int majority_rule(NodeStatus dsk_st, NodeStatus net_st, NodeStatus api_st) {
    int a = (dsk_st == NODE_ANOMALY);
    int b = (net_st == NODE_ANOMALY);
    int c = (api_st == NODE_ANOMALY);
    int present = (dsk_st != NODE_MISSING) + (net_st != NODE_MISSING) + (api_st != NODE_MISSING);

    if (present == 3) {
        return (a && b) || (a && c) || (b && c);
    }
    if (present == 2) {
        return (a + b + c) == 2;
    }
    return 0;
}

/* Сбор инцидентов по отсчетам 1..max_tick в массив структур Incident */
int detect_incidents(const NodeSeries nodes[NODE_COUNT], int max_tick, Incident log_buf[]) {
    int tick = 0;
    int total = 0;
    int id = 0;
    NodeStatus st[NODE_COUNT];
    int first_id = NODE_NONE;
    int second_id = NODE_NONE;

    for (tick = 1; tick <= max_tick && total < INCIDENT_CAP; tick++) {
        for (id = 0; id < NODE_COUNT; id++) {
            st[id] = node_status_at_tick(&nodes[id], tick);
        }

        if (majority_rule(st[NODE_DSK], st[NODE_NET], st[NODE_API])) {
            first_id = NODE_NONE;
            second_id = NODE_NONE;
            for (id = 0; id < NODE_COUNT; id++) {
                if (st[id] == NODE_ANOMALY) {
                    if (first_id == NODE_NONE) {
                        first_id = id;
                    } else if (second_id == NODE_NONE) {
                        second_id = id;
                    }
                }
            }
            log_buf[total].time = tick;
            log_buf[total].type = INCIDENT_MAJORITY_CODE;
            log_buf[total].first = first_id;
            log_buf[total].second = second_id;
            total++;
        }
    }

    return total;
}

/* Наибольшая длина ряда среди трех компонентов */
int longest_series_len(const NodeSeries nodes[NODE_COUNT]) {
    int best = 0;
    int i = 0;

    for (i = 0; i < NODE_COUNT; i++) {
        if (nodes[i].len > best) {
            best = nodes[i].len;
        }
    }
    return best;
}

/* Текстовая метка состояния узла */
const char* status_caption(NodeStatus st) {
    if (st == NODE_ANOMALY) {
        return "АНОМАЛИЯ";
    }
    if (st == NODE_FAULT) {
        return "ОШИБКА";
    }
    if (st == NODE_MISSING) {
        return "нет данных";
    }
    return "НОРМА";
}

/* Имя узла по его числовому индексу */
const char* node_tag_by_id(int id) {
    if (id == NODE_DSK) {
        return "DSK";
    }
    if (id == NODE_NET) {
        return "NET";
    }
    if (id == NODE_API) {
        return "API";
    }
    return "-";
}

/* Заголовок программы */
void render_banner(void) {
    printf("====================================================================\n");
    printf("  СИСТЕМА МОНИТОРИНГА ОБЛАЧНОЙ ИНФРАСТРУКТУРЫ  |  Вариант 9 (КТ2)\n");
    printf("  Узлы: DSK / NET / API   |   Формула: Q[i] = ln(X[i] / X[i - k])\n");
    printf("====================================================================\n\n");
}

/* Вывод панели текущего состояния и интерактивного меню */
void render_menu(DataSource src, int max_tick, const NodeSeries nodes[NODE_COUNT], int step_k, int analyzed) {
    const char* src_name = "не выбран";
    int total_samples = nodes[NODE_DSK].len + nodes[NODE_NET].len + nodes[NODE_API].len;

    if (src == SRC_VARIANT9) {
        src_name = "вариант 9";
    } else if (src == SRC_MANUAL) {
        src_name = "ручной ввод";
    }

    printf("+------------------------------------------------------------------+\n");
    printf("| Источник: %-11s | Отсч.: %-2d (изм.: %-3d) | k = %-2d | %-7s |\n",
        src_name, max_tick, total_samples, step_k, analyzed ? "готово" : "не расч");
    printf("+------------------------------------------------------------------+\n");
    printf("  [1] Задать шаг сравнения k\n");
    printf("  [2] Загрузить контрольный набор варианта 9 (17 измерений)\n");
    printf("  [3] Ввести данные вручную с клавиатуры\n");
    printf("  [4] Запустить анализ динамики и вывести отчет\n");
    printf("  [5] Показать журнал инцидентов\n");
    printf("  [6] Показать загруженные исходные ряды\n");
    printf("  [0] Выход из программы\n");
}

/* Вывод загруженных исходных рядов телеметрии */
void render_raw_series(const NodeSeries nodes[NODE_COUNT]) {
    int idx = 0;
    int j = 0;

    printf("\n+------------------------------------------------------------------+\n");
    printf("|                  ИСХОДНЫЕ ДАННЫЕ ТЕЛЕМЕТРИИ                      |\n");
    printf("+------------------------------------------------------------------+\n");
    for (idx = 0; idx < NODE_COUNT; idx++) {
        printf("  %s (%d изм.): ", nodes[idx].tag, nodes[idx].len);
        for (j = 0; j < nodes[idx].len; j++) {
            printf("[t=%d: %.2f] ", nodes[idx].items[j].tick, nodes[idx].items[j].value);
        }
        printf("\n");
    }
    printf("\n");
}

/* Табличный отчет по одному узлу с отображением пары X[i] и X[i-k] */
void render_node_table(const NodeSeries* series, int step_k) {
    int i = 0;

    printf("\n  Узел %s (измерений: %d):\n", series->tag, series->len);
    printf("  +----+-------+----------+----------+----------------+----------+\n");
    printf("  | No | Время |   X[i]   |  X[i-k]  |      Q[i]      |  Статус  |\n");
    printf("  +----+-------+----------+----------+----------------+----------+\n");
    for (i = 0; i < series->len; i++) {
        printf("  | %-2d |  %-4d | %8.2f | ", i + 1, series->items[i].tick, series->items[i].value);
        if (i < step_k) {
            printf("   ---   | ");
        } else {
            printf("%8.2f | ", series->items[i - step_k].value);
        }

        if (series->items[i].code == CALC_NO_PREV) {
            printf("%-14s | ", "--- (i < k)");
        } else if (series->items[i].code == CALC_ZERO_DIV) {
            printf("%-14s | ", "ОШИБКА (/0)");
        } else if (series->items[i].code == CALC_BAD_ARG) {
            printf("%-14s | ", "ОШИБКА (ln<=0)");
        } else if (series->items[i].code == CALC_NAN_VAL) {
            printf("%-14s | ", "ОШИБКА (NaN)");
        } else {
            printf("%+14.5f | ", series->items[i].q);
        }

        printf("%-8s |\n", status_caption(series->items[i].status));
    }
    printf("  +----+-------+----------+----------+----------------+----------+\n");
}

/* Сводная таблица состояний узлов по отсчетам времени */
void render_timeline_summary(const NodeSeries nodes[NODE_COUNT], int max_tick) {
    int t = 0;
    NodeStatus s0 = NODE_OK;
    NodeStatus s1 = NODE_OK;
    NodeStatus s2 = NODE_OK;

    printf("\n  Сводка состояний по отсчетам:\n");
    printf("  +-------+----------+----------+----------+----------+\n");
    printf("  | Время |   DSK    |   NET    |   API    | Инцидент |\n");
    printf("  +-------+----------+----------+----------+----------+\n");
    for (t = 1; t <= max_tick; t++) {
        s0 = node_status_at_tick(&nodes[NODE_DSK], t);
        s1 = node_status_at_tick(&nodes[NODE_NET], t);
        s2 = node_status_at_tick(&nodes[NODE_API], t);
        printf("  |  %-4d | %-8s | %-8s | %-8s |   %-6s |\n",
            t,
            status_caption(s0),
            status_caption(s1),
            status_caption(s2),
            majority_rule(s0, s1, s2) ? "ДА" : "нет");
    }
    printf("  +-------+----------+----------+----------+----------+\n");
}

/* Вывод таблицы зарегистрированных инцидентов */
void render_incident_table(const Incident log_buf[], int inc_total) {
    int i = 0;

    printf("\n  Журнал инцидентов (правило большинства: >= 2 из 3 узлов в АНОМАЛИЯ):\n");
    if (inc_total == 0) {
        printf("  -> Инцидентов не зарегистрировано.\n");
        return;
    }

    printf("  +-------+-----+--------------+--------------+\n");
    printf("  | Время | Тип |    first     |    second    |\n");
    printf("  +-------+-----+--------------+--------------+\n");
    for (i = 0; i < inc_total; i++) {
        printf("  |  %-4d |  %-2d | [%d] %-8s | [%d] %-8s |\n",
            log_buf[i].time,
            log_buf[i].type,
            log_buf[i].first, node_tag_by_id(log_buf[i].first),
            log_buf[i].second, node_tag_by_id(log_buf[i].second));
    }
    printf("  +-------+-----+--------------+--------------+\n");
}
