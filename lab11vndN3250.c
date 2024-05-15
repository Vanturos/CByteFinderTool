#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>

// Структура для хранения аргументов командной строки
struct Arguments {
    char *directory;
    unsigned char *search_bytes;
    size_t search_length;
    int help;
    int version;
};

// Структура для хранения пути к директории
struct DirectoryPath {
    char *path;
    struct DirectoryPath *next;
};

void print_help();
void print_version();
struct Arguments parse_arguments(int argc, char *argv[]);
void debug_info(const char *message, const char *path);
void replace_all(char *str, const char *find, const char *replace);
void search_bytes_in_file(const char *filename, const unsigned char *search_bytes, size_t search_length);
void traverse_directory(const char *directory, const unsigned char *search_bytes, size_t search_length);

int main(int argc, char *argv[]) {
    // Парсинг аргументов командной строки
    struct Arguments args = parse_arguments(argc, argv);

    // Вывод справки или информации о версии
    if (args.help) {
        print_help();
        free(args.search_bytes);
        return 0;
    } else if (args.version) {
        print_version();
        free(args.search_bytes);
        return 0;
    }

    // Обход директории и поиск файлов
    traverse_directory(args.directory, args.search_bytes, args.search_length);

    free(args.search_bytes);
    return 0;
}

void print_help() {
    printf("Usage: program_name [options] directory search_bytes\n");
    printf("Options:\n");
    printf("  -h, --help     Display this help message\n");
    printf("  -v, --version  Display version information\n");
}

// Вывод информации о версии программы и авторе
void print_version() {
    printf("Program Name version 1.0\n");
    printf("Author: Novikov Vladislav Dmitrievich\n");
}

// Функция для обработки аргументов командной строки
struct Arguments parse_arguments(int argc, char *argv[]) {
    struct Arguments args;
    args.directory = NULL;
    args.search_bytes = NULL;
    args.search_length = 0;
    args.help = 0;
    args.version = 0;

    // Опции командной строки
    const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    // Парсинг опций командной строки
    int option;
    while ((option = getopt_long(argc, argv, "hv", long_options, NULL)) != -1) {
        switch (option) {
            case 'h':
                args.help = 1;
                return args; // Вывод справки и выход из программы
            case 'v':
                args.version = 1;
                return args; // Вывод информации о версии и выход из программы
            default:
                fprintf(stderr, "Invalid option\n");
                exit(-1);
        }
    }

    // Проверка наличия обязательных аргументов
    if (optind + 2 != argc) {
        fprintf(stderr, "Invalid number of arguments\n");
        exit(-1);
    }

    // Присваивание значений аргументам
    args.directory = argv[optind];

    // Проверка формата ввода шестнадцатеричного числа
    char *search_string = argv[optind + 1];
    size_t search_string_length = strlen(search_string);
    if (search_string_length < 2 || strncmp(search_string, "0x", 2) != 0 || search_string_length % 2 != 0) {
        fprintf(stderr, "Invalid byte format\n");
        exit(-1);
    }
    
    search_string += 2; // Пропускаем префикс "0x"
    search_string_length -= 2;
    
    args.search_length = search_string_length / 2;
    args.search_bytes = (unsigned char *)malloc(args.search_length);
    if (args.search_bytes == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(-1);
    }
    
    // Преобразование шестнадцатеричной строки в массив байтов
    for (size_t i = 0; i < args.search_length; i++) {
        if (sscanf(&search_string[i * 2], "%2hhx", &args.search_bytes[i]) != 1) {
            fprintf(stderr, "Invalid byte format\n");
            exit(-1);
        }
    }

    return args;
}


// Функция для вывода отладочной информации
void debug_info(const char *message, const char *path) {
    if (getenv("LAB11DEBUG") != NULL) {
        fprintf(stderr, "(-_-) DEBUG: %s: %s\n", path, message);
    }
}

// Функция для замены всех вхождений одной строки в другую для //
void replace_all(char *str, const char *find, const char *replace) {
    char *pos = strstr(str, find);
    while (pos != NULL) {
        memmove(pos + strlen(replace), pos + strlen(find), strlen(pos + strlen(find)) + 1);
        memcpy(pos, replace, strlen(replace));
        pos = strstr(pos + strlen(replace), find);
    }
}

// Функция для поиска последовательности байтов в файле
void search_bytes_in_file(const char *filename, const unsigned char *search_bytes, size_t search_length) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return;
    }
    debug_info("Opened file", filename);

    // Выделение буфера для чтения файла
    size_t buffer_size = search_length;
    unsigned char *buffer = (unsigned char *)malloc(buffer_size);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return;
    }

    // Чтение файла и поиск последовательности байтов
    while (fread(buffer, buffer_size, 1, file) == 1) {
        if (memcmp(buffer, search_bytes, search_length) == 0) {
            printf("%s\n", filename);
            break;
        }
    }

    free(buffer);
    fclose(file);
}


// Функция для обхода директории без рекурсии
void traverse_directory(const char *directory, const unsigned char *search_bytes, size_t search_length) {
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", directory);
        return;
    }
    debug_info("Opened directory", directory);

    struct DirectoryPath *stack = NULL;

    // Первоначальное добавление корневой директории в стек
    struct DirectoryPath *root = (struct DirectoryPath *)malloc(sizeof(struct DirectoryPath));
    if (root == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        closedir(dir);
        return;
    }
    root->path = strdup(directory);
    if (root->path == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(root);
        closedir(dir);
        return;
    }
    root->next = NULL;
    stack = root;

    while (stack != NULL) {
        // Извлечение директории из стека
        struct DirectoryPath *current = stack;
        stack = stack->next;

        // Открытие текущей директории
        DIR *current_dir = opendir(current->path);
        if (current_dir == NULL) {
            fprintf(stderr, "Error opening directory: %s\n", current->path);
            free(current->path);
            free(current);
            continue;
        }
        debug_info("Opened current directory", current->path);

        struct dirent *entry;
        while ((entry = readdir(current_dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                // Игнорируем . и ..
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                // Полный путь к вложенной директории
                char subdir[1024];
                snprintf(subdir, sizeof(subdir), "%s/%s", current->path, entry->d_name);
                // Замена всех вхождений "//" на "/"
                replace_all(subdir, "//", "/");
                // Добавление вложенной директории в стек
                struct DirectoryPath *subdir_node = (struct DirectoryPath *)malloc(sizeof(struct DirectoryPath));
                if (subdir_node == NULL) {
                    fprintf(stderr, "Memory allocation failed\n");
                    closedir(current_dir);
                    free(current->path);
                    free(current);
                    while (stack != NULL) {
                        struct DirectoryPath *temp = stack;
                        stack = stack->next;
                        free(temp->path);
                        free(temp);
                    }
                    return;
                }
                subdir_node->path = strdup(subdir);
                if (subdir_node->path == NULL) {
                    fprintf(stderr, "Memory allocation failed\n");
                    free(subdir_node);
                    closedir(current_dir);
                    free(current->path);
                    free(current);
                    while (stack != NULL) {
                        struct DirectoryPath *temp = stack;
                        stack = stack->next;
                        free(temp->path);
                        free(temp);
                    }
                    return;
                }
                subdir_node->next = stack;
                stack = subdir_node;
                debug_info("Pushed directory to stack", subdir);
            } else if (entry->d_type == DT_REG) {
                // Полный путь к файлу
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/%s", current->path, entry->d_name);
                // Замена всех вхождений "//" на "/"
                replace_all(filepath, "//", "/");
                // Поиск последовательности байтов в файле
                search_bytes_in_file(filepath, search_bytes, search_length);
                debug_info("Searched bytes in file", filepath);
            }
        }

        closedir(current_dir);
        debug_info("Closed current directory", current->path);
        free(current->path);
        free(current);
    }

    // Освобождение памяти, выделенной для стека
    while (stack != NULL) {
        struct DirectoryPath *temp = stack;
        stack = stack->next;
        free(temp->path);
        free(temp);
    }

    closedir(dir);
}
