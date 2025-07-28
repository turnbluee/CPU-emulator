#include "memoryHeader.h"

// Массив строк с сообщениями об ошибках памяти
const char* MemoryErrorMessages[MEMORY_ERROR_COUNT] = {
    "Success",                         // MEMORY_SUCCESS
    "Invalid memory address",          // MEMORY_INVALID_ADDRESS
    "Memory access out of bounds",     // MEMORY_OUT_OF_BOUNDS
    "Memory allocation error",         // MEMORY_ALLOCATION_ERROR
    "Memory is not initialized"        // MEMORY_NOT_INITIALIZED
};

// Инициализация памяти с заданными размерами
int memory_init(Memory* memory, size_t instruction_size, size_t data_size) {
    if (!memory) {
        return MEMORY_INVALID_ADDRESS;
    }
    
    // Выделение памяти для инструкций
    memory->instruction_memory = (uint8_t*)malloc(instruction_size);
    if (!memory->instruction_memory) {
        return MEMORY_ALLOCATION_ERROR;
    }
    
    // Выделение памяти для данных
    memory->data_memory = (uint8_t*)malloc(data_size);
    if (!memory->data_memory) {
        free(memory->instruction_memory);
        memory->instruction_memory = NULL;
        return MEMORY_ALLOCATION_ERROR;
    }
    
    // Инициализация параметров памяти
    memory->instruction_size = instruction_size;
    memory->data_size = data_size;
    memory->initialized = 1;
    
    // Очистка памяти
    memset(memory->instruction_memory, 0, instruction_size);
    memset(memory->data_memory, 0, data_size);
    
    return MEMORY_SUCCESS;
}

// Инициализация памяти с размерами по умолчанию
int memory_init_default(Memory* memory) {
    return memory_init(memory, DEFAULT_INSTRUCTION_MEMORY_SIZE, DEFAULT_DATA_MEMORY_SIZE);
}

// Освобождение памяти
void memory_free(Memory* memory) {
    if (!memory || !memory->initialized) {
        return;
    }
    
    // Освобождаем выделенную память
    free(memory->instruction_memory);
    free(memory->data_memory);
    
    // Сбрасываем указатели и флаг инициализации
    memory->instruction_memory = NULL;
    memory->data_memory = NULL;
    memory->instruction_size = 0;
    memory->data_size = 0;
    memory->initialized = 0;
}

// Проверка инициализации памяти
static int check_memory_initialized(Memory* memory) {
    if (!memory) {
        return MEMORY_INVALID_ADDRESS;
    }
    
    if (!memory->initialized) {
        return MEMORY_NOT_INITIALIZED;
    }
    
    return MEMORY_SUCCESS;
}

// Считывание 8-битного значения из памяти данных
int memory_read_byte(Memory* memory, uint16_t address, uint8_t* value) {
    int result = check_memory_initialized(memory);
    if (result != MEMORY_SUCCESS) {
        return result;
    }
    
    // Проверка границ памяти
    if (address >= memory->data_size) {
        return MEMORY_OUT_OF_BOUNDS;
    }
    
    *value = memory->data_memory[address];
    return MEMORY_SUCCESS;
}

// Считывание 16-битного значения из памяти данных
int memory_read_word(Memory* memory, uint16_t address, uint16_t* value) {
    int result = check_memory_initialized(memory);
    if (result != MEMORY_SUCCESS) {
        return result;
    }
    
    // Проверка границ памяти (нужно два байта)
    // Важно: address уже должен быть выровнен по границе слова (чётное число)
    if (address + 1 >= memory->data_size) {
        return MEMORY_OUT_OF_BOUNDS;
    }
    
    // Адрес должен быть выровнен по границе слова (нечётные адреса не допускаются)
    if (address % 2 != 0) {
        fprintf(stderr, "ВНИМАНИЕ: Чтение слова по невыровненному адресу 0x%04X\n", address);
        // Продолжаем выполнение, чтобы обеспечить обратную совместимость
    }
    
    // Чтение младшего и старшего байта и объединение в 16-битное слово
    uint8_t low_byte = memory->data_memory[address];
    uint8_t high_byte = memory->data_memory[address + 1];
    
    *value = (high_byte << 8) | low_byte;
    
    return MEMORY_SUCCESS;
}

// Запись 8-битного значения в память данных
int memory_write_byte(Memory* memory, uint16_t address, uint8_t value) {
    int result = check_memory_initialized(memory);
    if (result != MEMORY_SUCCESS) {
        return result;
    }
    
    // Проверка границ памяти
    if (address >= memory->data_size) {
        return MEMORY_OUT_OF_BOUNDS;
    }
    
    memory->data_memory[address] = value;
    return MEMORY_SUCCESS;
}

// Запись 16-битного значения в память данных
int memory_write_word(Memory* memory, uint16_t address, uint16_t value) {
    int result = check_memory_initialized(memory);
    if (result != MEMORY_SUCCESS) {
        return result;
    }
    
    // Проверка границ памяти (нужно два байта)
    if (address + 1 >= memory->data_size) {
        return MEMORY_OUT_OF_BOUNDS;
    }
    
    // Адрес должен быть выровнен по границе слова (нечётные адреса не допускаются)
    if (address % 2 != 0) {
        fprintf(stderr, "ВНИМАНИЕ: Запись слова по невыровненному адресу 0x%04X\n", address);
        // Продолжаем выполнение, чтобы обеспечить обратную совместимость
    }
    
    // Разбиение 16-битного слова на старший и младший байты
    uint8_t low_byte = value & 0xFF;
    uint8_t high_byte = (value >> 8) & 0xFF;
    
    memory->data_memory[address] = low_byte;
    memory->data_memory[address + 1] = high_byte;
    
    return MEMORY_SUCCESS;
}

// Считывание 32-битной инструкции из памяти инструкций
int memory_read_instruction(Memory* memory, uint16_t address, uint32_t* instruction) {
    int result = check_memory_initialized(memory);
    if (result != MEMORY_SUCCESS) {
        return result;
    }
    
    // Инструкции имеют размер 4 байта, смещение в байтах
    uint16_t byte_address = address * 4;
    
    // Проверка границ памяти (нужно 4 байта)
    if (byte_address + 3 >= memory->instruction_size) {
        return MEMORY_OUT_OF_BOUNDS;
    }
    
    // Чтение четырех байтов и формирование 32-битной инструкции (big-endian)
    uint8_t byte0 = memory->instruction_memory[byte_address];     // Старший байт
    uint8_t byte1 = memory->instruction_memory[byte_address + 1];
    uint8_t byte2 = memory->instruction_memory[byte_address + 2];
    uint8_t byte3 = memory->instruction_memory[byte_address + 3]; // Младший байт
    
    // Сборка 32-битного слова
    *instruction = ((uint32_t)byte0 << 24) | 
                   ((uint32_t)byte1 << 16) | 
                   ((uint32_t)byte2 << 8) | 
                    (uint32_t)byte3;
    
    return MEMORY_SUCCESS;
}

// Запись 32-битной инструкции в память инструкций
int memory_write_instruction(Memory* memory, uint16_t address, uint32_t instruction) {
    int result = check_memory_initialized(memory);
    if (result != MEMORY_SUCCESS) {
        return result;
    }
    
    // Инструкции имеют размер 4 байта, смещение в байтах
    uint16_t byte_address = address * 4;
    
    // Проверка границ памяти (нужно 4 байта)
    if (byte_address + 3 >= memory->instruction_size) {
        return MEMORY_OUT_OF_BOUNDS;
    }
    
    // Разбиение 32-битной инструкции на отдельные байты (big-endian)
    uint8_t byte0 = (instruction >> 24) & 0xFF; // Старший байт
    uint8_t byte1 = (instruction >> 16) & 0xFF;
    uint8_t byte2 = (instruction >> 8) & 0xFF;
    uint8_t byte3 = instruction & 0xFF;         // Младший байт
    
    // Запись байтов в память инструкций
    memory->instruction_memory[byte_address] = byte0;
    memory->instruction_memory[byte_address + 1] = byte1;
    memory->instruction_memory[byte_address + 2] = byte2;
    memory->instruction_memory[byte_address + 3] = byte3;
    
    return MEMORY_SUCCESS;
}

// Загрузка программы (машинного кода) из файла в память инструкций
int memory_load_program(Memory* memory, const char* filename) {
    int result = check_memory_initialized(memory);
    if (result != MEMORY_SUCCESS) {
        return result;
    }
    
    // Открытие бинарного файла
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return MEMORY_INVALID_ADDRESS; // Ошибка открытия файла
    }
    
    // Определение размера файла
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Проверка, что размер файла не превышает размер памяти инструкций
    if (file_size > memory->instruction_size) {
        fclose(file);
        return MEMORY_OUT_OF_BOUNDS;
    }
    
    // Чтение файла в память инструкций
    size_t bytes_read = fread(memory->instruction_memory, 1, file_size, file);
    fclose(file);
    
    // Проверка успешности чтения
    if (bytes_read != file_size) {
        return MEMORY_INVALID_ADDRESS; // Ошибка чтения файла
    }
    
    return MEMORY_SUCCESS;
}

// Очистка памяти (заполнение нулями)
void memory_clear(Memory* memory) {
    int result = check_memory_initialized(memory);
    if (result != MEMORY_SUCCESS) {
        return;
    }
    
    // Очистка памяти инструкций и данных
    memset(memory->instruction_memory, 0, memory->instruction_size);
    memset(memory->data_memory, 0, memory->data_size);
}

// Дамп содержимого памяти инструкций для отладки
void memory_dump_instructions(Memory* memory, FILE* output, size_t count) {
    int result = check_memory_initialized(memory);
    if (result != MEMORY_SUCCESS) {
        fprintf(output, "Memory not initialized\n");
        return;
    }
    
    // Ограничение количества выводимых инструкций
    size_t max_instructions = memory->instruction_size / 4;
    if (count > max_instructions) {
        count = max_instructions;
    }
    
    fprintf(output, "Instruction Memory Dump (showing %zu instructions):\n", count);
    fprintf(output, "Address  | Machine Code | Bytes\n");
    fprintf(output, "---------+-------------+------------------\n");
    
    for (size_t i = 0; i < count; i++) {
        uint32_t instruction;
        if (memory_read_instruction(memory, i, &instruction) == MEMORY_SUCCESS) {
            uint8_t byte0 = (instruction >> 24) & 0xFF;
            uint8_t byte1 = (instruction >> 16) & 0xFF;
            uint8_t byte2 = (instruction >> 8) & 0xFF;
            uint8_t byte3 = instruction & 0xFF;
            
            fprintf(output, "0x%06zX | 0x%08X | %02X %02X %02X %02X\n", 
                    i, instruction, byte0, byte1, byte2, byte3);
        }
    }
}

// Дамп содержимого памяти данных для отладки
void memory_dump_data(Memory* memory, FILE* output, size_t offset, size_t count) {
    int result = check_memory_initialized(memory);
    if (result != MEMORY_SUCCESS) {
        fprintf(output, "Memory not initialized\n");
        return;
    }
    
    // Проверка и ограничение параметров
    if (offset >= memory->data_size) {
        fprintf(output, "Offset out of bounds\n");
        return;
    }
    
    if (offset + count > memory->data_size) {
        count = memory->data_size - offset;
    }
    
    fprintf(output, "Data Memory Dump (offset: 0x%04zX, count: %zu bytes):\n", offset, count);
    fprintf(output, "Address  | Bytes (hex)         | ASCII\n");
    fprintf(output, "---------+---------------------+------------------\n");
    
    // Вывод по 16 байт в строке
    for (size_t i = 0; i < count; i += 16) {
        fprintf(output, "0x%06zX | ", offset + i);
        
        // Вывод шестнадцатеричных значений
        for (size_t j = 0; j < 16 && (i + j) < count; j++) {
            fprintf(output, "%02X ", memory->data_memory[offset + i + j]);
        }
        
        // Выравнивание для ASCII-части
        for (size_t j = (count - i < 16 ? count - i : 16); j < 16; j++) {
            fprintf(output, "   ");
        }
        
        fprintf(output, "| ");
        
        // Вывод ASCII-представления
        for (size_t j = 0; j < 16 && (i + j) < count; j++) {
            uint8_t byte = memory->data_memory[offset + i + j];
            if (byte >= 32 && byte <= 126) { // Печатаемый ASCII-символ
                fprintf(output, "%c", byte);
            } else {
                fprintf(output, ".");
            }
        }
        
        fprintf(output, "\n");
    }
}