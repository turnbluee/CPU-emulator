#ifndef MEMORYHEADER_H
#define MEMORYHEADER_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Коды ошибок памяти
typedef enum {
    MEMORY_SUCCESS = 0,         // Успешная операция
    MEMORY_INVALID_ADDRESS,     // Неверный адрес памяти
    MEMORY_OUT_OF_BOUNDS,       // Выход за границы памяти
    MEMORY_ALLOCATION_ERROR,    // Ошибка выделения памяти
    MEMORY_NOT_INITIALIZED,     // Память не инициализирована
    MEMORY_ERROR_COUNT          // Количество кодов ошибок (всегда последний)
} MemoryErrorCode;

// Размеры памяти по умолчанию
#define DEFAULT_INSTRUCTION_MEMORY_SIZE 1024  // 1KB для инструкций (256 инструкций по 4 байта)
#define DEFAULT_DATA_MEMORY_SIZE        4096  // 4KB для данных

// Массив описаний ошибок памяти
extern const char* MemoryErrorMessages[MEMORY_ERROR_COUNT];

// Структура памяти процессора (Гарвардская архитектура - разделение кода и данных)
typedef struct {
    uint8_t* instruction_memory;  // Память для хранения инструкций
    size_t instruction_size;      // Размер памяти инструкций
    
    uint8_t* data_memory;         // Память для хранения данных
    size_t data_size;             // Размер памяти данных
    
    int initialized;              // Флаг инициализации памяти
} Memory;

// Функции для работы с памятью

// Инициализация памяти с заданными размерами
int memory_init(Memory* memory, size_t instruction_size, size_t data_size);

// Инициализация памяти с размерами по умолчанию
int memory_init_default(Memory* memory);

// Освобождение памяти
void memory_free(Memory* memory);

// Считывание 8-битного значения из памяти данных
int memory_read_byte(Memory* memory, uint16_t address, uint8_t* value);

// Считывание 16-битного значения из памяти данных
int memory_read_word(Memory* memory, uint16_t address, uint16_t* value);

// Запись 8-битного значения в память данных
int memory_write_byte(Memory* memory, uint16_t address, uint8_t value);

// Запись 16-битного значения в память данных
int memory_write_word(Memory* memory, uint16_t address, uint16_t value);

// Считывание 32-битной инструкции из памяти инструкций
int memory_read_instruction(Memory* memory, uint16_t address, uint32_t* instruction);

// Запись 32-битной инструкции в память инструкций
int memory_write_instruction(Memory* memory, uint16_t address, uint32_t instruction);

// Загрузка программы (машинного кода) из файла в память инструкций
int memory_load_program(Memory* memory, const char* filename);

// Очистка памяти (заполнение нулями)
void memory_clear(Memory* memory);

// Дамп содержимого памяти для отладки
void memory_dump_instructions(Memory* memory, FILE* output, size_t count);
void memory_dump_data(Memory* memory, FILE* output, size_t offset, size_t count);

#endif //MEMORYHEADER_H