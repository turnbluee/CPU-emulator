#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memoryHeader.h"
#include "../assembler/parserHeader.h"

// Размеры и константы
#define REGISTERFILESIZE 2           // Размер регистра в байтах (16 бит)
#define NUM_REGISTERS 16            // Количество регистров
#define INSTRUCTION_SIZE 4          // Размер инструкции в байтах
#define INSTR_ADDR_MASK 0x0000FFFC  // Маска для выравнивания адреса инструкции (кратно 4)

// Коды ошибок эмулятора
typedef enum {
    EMULATOR_SUCCESS = 0,           // Успешная операция
    EMULATOR_INVALID_INSTRUCTION,   // Неверная инструкция
    EMULATOR_MEMORY_ERROR,          // Ошибка доступа к памяти
    EMULATOR_DIVISION_BY_ZERO,      // Деление на ноль
    EMULATOR_INVALID_REGISTER,      // Неверный регистр
    EMULATOR_HALT,                  // Остановка эмулятора (не ошибка)
    EMULATOR_ERROR_COUNT            // Количество кодов ошибок (всегда последний)
} EmulatorErrorCode;

// Массив строк с сообщениями об ошибках эмулятора
extern const char* EmulatorErrorMessages[EMULATOR_ERROR_COUNT];

// Структура CPU
typedef struct {
    uint16_t IP;                   // Instruction Pointer (указатель команды)
    uint16_t RF[NUM_REGISTERS];    // Register File (регистровый файл)
    Memory memory;                 // Память процессора (Гарвардская архитектура)
    int running;                   // Флаг работы процессора
    FILE* output_stream;           // Поток вывода для результатов
    int debug_mode;                // Флаг включения отладочного вывода
} CPU;

// Функции инициализации
int emulator_init(CPU* cpu, FILE* output_stream, int debug_mode);
int emulator_init_default(CPU* cpu);
int emulator_init_with_debug(CPU* cpu, int debug_mode);
void emulator_free(CPU* cpu);

// Функции для работы с регистрами
uint16_t emulator_get_register(CPU* cpu, uint8_t reg_num);
int emulator_set_register(CPU* cpu, uint8_t reg_num, uint16_t value);
void emulator_dump_registers(CPU* cpu, FILE* output);

// Декодирование и выполнение инструкций
int emulator_decode_instruction(CPU* cpu, uint32_t instruction);
int emulator_fetch_execute_cycle(CPU* cpu);

// Выполнение программы
int emulator_load_program(CPU* cpu, const char* filename);
int emulator_run(CPU* cpu);

// Вспомогательные функции
void emulator_print_error(int error_code, const char* custom_message);

#endif //EMULATOR_H
