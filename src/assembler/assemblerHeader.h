#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parserHeader.h"

typedef enum {
    ASSEMBLER_SUCCESS = 0,              // Успешное выполнение
    ASSEMBLER_ERROR_INVALID_INPUT,      // Неверный входной файл
    ASSEMBLER_ERROR_INVALID_OUTPUT,     // Неверный выходной файл
    ASSEMBLER_ERROR_PARSER_FAILED,      // Ошибка парсера
    ASSEMBLER_ERROR_WRITING_FAILED,     // Ошибка записи
    ASSEMBLER_ERROR_COUNT               // Количество кодов ошибок (всегда последний)
} AssemblerErrorCode;

extern const char* AssemblerErrorMessages[ASSEMBLER_ERROR_COUNT];

int assemble_file(const char* input_filename, const char* output_filename);

void print_assembler_error(int error_code, const char* custom_message);
const char* get_file_extension(const char* filename);

#endif //ASSEMBLER_H
