#include "emulatorHeader.h"

// Массив строк с сообщениями об ошибках эмулятора
const char* EmulatorErrorMessages[EMULATOR_ERROR_COUNT] = {
    "Success",                        // EMULATOR_SUCCESS
    "Invalid instruction",            // EMULATOR_INVALID_INSTRUCTION
    "Memory error",                   // EMULATOR_MEMORY_ERROR
    "Division by zero",               // EMULATOR_DIVISION_BY_ZERO
    "Invalid register",               // EMULATOR_INVALID_REGISTER
    "Emulator halted"                 // EMULATOR_HALT
};

// Вспомогательные функции для вывода ошибок
void emulator_print_error(int error_code, const char* custom_message) {
    if (error_code >= 0 && error_code < EMULATOR_ERROR_COUNT) {
        if (custom_message) {
            fprintf(stderr, "%s: %s (%d)\n", custom_message, 
                    EmulatorErrorMessages[error_code], error_code);
        } else {
            fprintf(stderr, "Emulator error: %s (%d)\n", 
                    EmulatorErrorMessages[error_code], error_code);
        }
    } else {
        fprintf(stderr, "%s: Unknown error code: %d\n", 
                custom_message ? custom_message : "Emulator error", error_code);
    }
}

// Инициализация CPU с указанным потоком вывода и режимом отладки
int emulator_init(CPU* cpu, FILE* output_stream, int debug_mode) {
    if (!cpu) {
        return EMULATOR_INVALID_INSTRUCTION;
    }
    
    // Инициализация памяти с размерами по умолчанию
    int memory_result = memory_init_default(&cpu->memory);
    if (memory_result != MEMORY_SUCCESS) {
        return EMULATOR_MEMORY_ERROR;
    }
    
    // Инициализация регистров (все нули)
    cpu->IP = 0;
    memset(cpu->RF, 0, sizeof(cpu->RF));
    
    cpu->running = 0;
    
    // Если поток вывода не указан, используем stdout
    cpu->output_stream = output_stream ? output_stream : stdout;
    
    // Установка режима отладки
    cpu->debug_mode = debug_mode;
    
    return EMULATOR_SUCCESS;
}

// Инициализация CPU со стандартным выводом и без отладки
int emulator_init_default(CPU* cpu) {
    return emulator_init(cpu, stdout, 0);
}

// Инициализация CPU со стандартным выводом и указанным режимом отладки
int emulator_init_with_debug(CPU* cpu, int debug_mode) {
    return emulator_init(cpu, stdout, debug_mode);
}

// Освобождение ресурсов CPU
void emulator_free(CPU* cpu) {
    if (!cpu) {
        return;
    }
    
    // Освобождение памяти
    memory_free(&cpu->memory);
    
    // Сброс регистров
    cpu->IP = 0;
    memset(cpu->RF, 0, sizeof(cpu->RF));
    
    cpu->running = 0;
    cpu->output_stream = NULL;
}

// Получение значения регистра
uint16_t emulator_get_register(CPU* cpu, uint8_t reg_num) {
    if (!cpu || reg_num >= NUM_REGISTERS) {
        return 0;
    }
    
    return cpu->RF[reg_num];
}

// Установка значения регистра
int emulator_set_register(CPU* cpu, uint8_t reg_num, uint16_t value) {
    if (!cpu) {
        return EMULATOR_INVALID_INSTRUCTION;
    }
    
    if (reg_num >= NUM_REGISTERS) {
        return EMULATOR_INVALID_REGISTER;
    }
    
    cpu->RF[reg_num] = value;
    return EMULATOR_SUCCESS;
}

// Вывод содержимого регистров
void emulator_dump_registers(CPU* cpu, FILE* output) {
    if (!cpu) {
        return;
    }
    
    FILE* out = output ? output : cpu->output_stream;
    
    fprintf(out, "CPU Registers:\n");
    fprintf(out, "IP = 0x%04X\n", cpu->IP);
    
    for (int i = 0; i < NUM_REGISTERS; i++) {
        fprintf(out, "R%d = 0x%04X\n", i, cpu->RF[i]);
    }
    
    fprintf(out, "\n");
}

// Загрузка программы из файла
int emulator_load_program(CPU* cpu, const char* filename) {
    if (!cpu || !filename) {
        return EMULATOR_INVALID_INSTRUCTION;
    }
    
    int result = memory_load_program(&cpu->memory, filename);
    if (result != MEMORY_SUCCESS) {
        emulator_print_error(EMULATOR_MEMORY_ERROR, "Failed to load program");
        return EMULATOR_MEMORY_ERROR;
    }
    
    // Сброс указателя команд
    cpu->IP = 0;
    
    return EMULATOR_SUCCESS;
}

// Декодирование и выполнение инструкции
int emulator_decode_instruction(CPU* cpu, uint32_t instruction) {
    if (!cpu) {
        return EMULATOR_INVALID_INSTRUCTION;
    }
    
    // Декодирование полей инструкции
    uint8_t opcode = (instruction >> 24) & 0xFF;
    uint8_t src0 = (instruction >> 16) & 0xFF;
    uint8_t src1_or_const_hi = (instruction >> 8) & 0xFF;
    uint8_t dst_or_const_lo_or_src2 = instruction & 0xFF;
    
    if (cpu->debug_mode) {
        fprintf(cpu->output_stream, "[ОТЛАДКА] IP=0x%04X: Инструкция=0x%08X, опкод=%d, операнды: %d, %d, %d\n",
               cpu->IP, instruction, opcode, src0, src1_or_const_hi, dst_or_const_lo_or_src2);
    }
    
    // Проверка валидности регистров src0
    if (src0 >= NUM_REGISTERS && opcode != OPC_SET_CONST && opcode != OPC_READY) {
        emulator_print_error(EMULATOR_INVALID_REGISTER, "Invalid src0 register");
        return EMULATOR_INVALID_REGISTER;
    }
    
    // Дополнительная проверка валидности регистров src1/dst в зависимости от формата
    if ((opcode <= OPC_LD || opcode == OPC_ST) && src1_or_const_hi >= NUM_REGISTERS) {
        emulator_print_error(EMULATOR_INVALID_REGISTER, "Invalid src1 register");
        return EMULATOR_INVALID_REGISTER;
    }
    
    if (opcode <= OPC_XOR && dst_or_const_lo_or_src2 >= NUM_REGISTERS) {
        emulator_print_error(EMULATOR_INVALID_REGISTER, "Invalid dst register");
        return EMULATOR_INVALID_REGISTER;
    }
    
    if (opcode == OPC_BNZ && src0 >= NUM_REGISTERS) {
        emulator_print_error(EMULATOR_INVALID_REGISTER, "Invalid src0 register for BNZ");
        return EMULATOR_INVALID_REGISTER;
    }
    
    if (opcode == OPC_ST && dst_or_const_lo_or_src2 >= NUM_REGISTERS) {
        emulator_print_error(EMULATOR_INVALID_REGISTER, "Invalid src2 register for ST");
        return EMULATOR_INVALID_REGISTER;
    }
    
    // Выполнение операции в зависимости от кода
    switch (opcode) {
        case OPC_NOP:
            // Ничего не делаем
            break;
            
        case OPC_ADD:
            // RF[dst] <- RF[src_0] + RF[src_1]
            cpu->RF[dst_or_const_lo_or_src2] = cpu->RF[src0] + cpu->RF[src1_or_const_hi];
            break;
            
        case OPC_SUB:
            // RF[dst] <- RF[src_0] - RF[src_1]
            cpu->RF[dst_or_const_lo_or_src2] = cpu->RF[src0] - cpu->RF[src1_or_const_hi];
            break;
            
        case OPC_MUL:
            // {RF[dst+1], RF[dst]} <- RF[src_0] * RF[src_1]
            {
                uint32_t result = (uint32_t)cpu->RF[src0] * (uint32_t)cpu->RF[src1_or_const_hi];
                cpu->RF[dst_or_const_lo_or_src2] = result & 0xFFFF;  // Младшие 16 бит
                
                // Если dst=15, dst+1=0 (циклический переход)
                uint8_t next_reg = (dst_or_const_lo_or_src2 == 15) ? 0 : dst_or_const_lo_or_src2 + 1;
                cpu->RF[next_reg] = (result >> 16) & 0xFFFF;  // Старшие 16 бит
            }
            break;
            
        case OPC_DIV:
            // RF[dst] <- RF[src_0] / RF[src_1]
            if (cpu->RF[src1_or_const_hi] == 0) {
                emulator_print_error(EMULATOR_DIVISION_BY_ZERO, "Division by zero");
                return EMULATOR_DIVISION_BY_ZERO;
            }
            cpu->RF[dst_or_const_lo_or_src2] = cpu->RF[src0] / cpu->RF[src1_or_const_hi];
            break;
            
        case OPC_CMPGE:
            // RF[dst] <- RF[src_0] >= RF[src_1]
            cpu->RF[dst_or_const_lo_or_src2] = (cpu->RF[src0] >= cpu->RF[src1_or_const_hi]) ? 1 : 0;
            break;
            
        case OPC_RSHFT:
            // RF[dst] <- RF[src_0] >> src_1
            cpu->RF[dst_or_const_lo_or_src2] = cpu->RF[src0] >> cpu->RF[src1_or_const_hi];
            break;
            
        case OPC_LSHFT:
            // RF[dst] <- RF[src_0] << src_1
            cpu->RF[dst_or_const_lo_or_src2] = cpu->RF[src0] << cpu->RF[src1_or_const_hi];
            break;
            
        case OPC_AND:
            // RF[dst] <- RF[src_0] & RF[src_1]
            cpu->RF[dst_or_const_lo_or_src2] = cpu->RF[src0] & cpu->RF[src1_or_const_hi];
            break;
            
        case OPC_OR:
            // RF[dst] <- RF[src_0] | RF[src_1]
            cpu->RF[dst_or_const_lo_or_src2] = cpu->RF[src0] | cpu->RF[src1_or_const_hi];
            break;
            
        case OPC_XOR:
            // RF[dst] <- RF[src_0] ^ RF[src_1]
            cpu->RF[dst_or_const_lo_or_src2] = cpu->RF[src0] ^ cpu->RF[src1_or_const_hi];
            break;
            
        case OPC_LD:
            // RF[dst] <- MEM[ADDR][15:0]], где ADDR[15:0] = RF[src_0][15:0] + RF[src_1][15:0]
            // По комментариям из примера: ld R0, R0, R7 = R7 = array[i] (операнды: базовый регистр, смещение, целевой регистр)
            {
                uint8_t base_reg = src0;
                uint8_t offset_reg = src1_or_const_hi;
                uint8_t target_reg = dst_or_const_lo_or_src2;
                
                uint16_t addr = cpu->RF[base_reg] + cpu->RF[offset_reg];
                uint16_t value;
                int result = memory_read_word(&cpu->memory, addr, &value);
                
                if (result != MEMORY_SUCCESS) {
                    emulator_print_error(EMULATOR_MEMORY_ERROR, "Failed to read memory");
                    return EMULATOR_MEMORY_ERROR;
                }
                
                if (cpu->debug_mode) {
                    fprintf(cpu->output_stream, "[ОТЛАДКА LD] IP=0x%04X: Чтение из памяти по адресу 0x%04X (R%d[0x%04X] + R%d[0x%04X]), значение=0x%04X -> R%d\n",
                           cpu->IP, addr, base_reg, cpu->RF[base_reg], offset_reg, cpu->RF[offset_reg], value, target_reg);
                }
                
                cpu->RF[target_reg] = value;
            }
            break;
            
        case OPC_SET_CONST:
            // RF[dst] <- {const_[15:8], const[7:0]}
            {
                // Последний байт инструкции содержит номер регистра-назначения
                uint8_t dst_reg = dst_or_const_lo_or_src2;
                // Константа формируется из двух средних байтов в правильном порядке
                uint16_t constant = ((uint16_t)src0 << 8) | src1_or_const_hi;
                
                cpu->RF[dst_reg] = constant;
            }
            break;
            
        case OPC_ST:
            // MEM[ADDR[15:0]] <- RF[src_0], где ADDR[15:0] = RF[src_1][15:0] + RF[src_2][15:0]
            // По комментариям из примера: st R0, R3, R4 => MEM[R3 + R4] = R0
            {
                uint8_t value_reg = src0;
                uint8_t base_reg = src1_or_const_hi;
                uint8_t offset_reg = dst_or_const_lo_or_src2;
                
                
                uint16_t addr = cpu->RF[base_reg] + cpu->RF[offset_reg];
                uint16_t value = cpu->RF[value_reg];
                
                if (cpu->debug_mode) {
                    fprintf(cpu->output_stream, "[ОТЛАДКА ST] IP=0x%04X: Запись в память по адресу 0x%04X (R%d[0x%04X] + R%d[0x%04X]), значение R%d[0x%04X]\n",
                           cpu->IP, addr, base_reg, cpu->RF[base_reg], offset_reg, cpu->RF[offset_reg], value_reg, value);
                }
                
                int result = memory_write_word(&cpu->memory, addr, value);
                
                if (result != MEMORY_SUCCESS) {
                    emulator_print_error(EMULATOR_MEMORY_ERROR, "Failed to write memory");
                    return EMULATOR_MEMORY_ERROR;
                }
            }
            break;
            
        case OPC_BNZ:
            // if(src0 != 0) IP <- target[15:0] else IP <- IP + instr_size
            {
                uint16_t target = ((uint16_t)src1_or_const_hi << 8) | dst_or_const_lo_or_src2;
                
                if (cpu->debug_mode) {
                    fprintf(cpu->output_stream, "[ОТЛАДКА BNZ] Проверка условия: R%d[0x%04X] != 0, target=0x%04X\n",
                           src0, cpu->RF[src0], target);
                }
                
                if (cpu->RF[src0] != 0) {
                    // Переход на указанный адрес
                    // В соответствии с документацией, IP <- target[15:0]
                    cpu->IP = target;
                    
                    if (cpu->debug_mode) {
                        fprintf(cpu->output_stream, "[ОТЛАДКА BNZ] Переход выполнен: новый IP=0x%04X\n", cpu->IP);
                    }
                    
                    return EMULATOR_SUCCESS;  // Досрочный выход, IP уже обновлен
                } else {
                    if (cpu->debug_mode) {
                        fprintf(cpu->output_stream, "[ОТЛАДКА BNZ] Условие не выполнено, переход не выполняется\n");
                    }
                }
                // Иначе IP будет увеличен стандартным образом после выполнения
            }
            break;
            
        case OPC_READY:
            // IP<-0; конец работы
            cpu->IP = 0;
            cpu->running = 0;  // Остановка эмулятора
            return EMULATOR_HALT;
            
        default:
            emulator_print_error(EMULATOR_INVALID_INSTRUCTION, "Unknown opcode");
            return EMULATOR_INVALID_INSTRUCTION;
    }
    
    // Увеличиваем IP для перехода к следующей инструкции
    cpu->IP += INSTRUCTION_SIZE;  // Инкремент на размер инструкции (4 байта)
    
    return EMULATOR_SUCCESS;
}

// Цикл выборки-декодирования-исполнения
int emulator_fetch_execute_cycle(CPU* cpu) {
    if (!cpu) {
        return EMULATOR_INVALID_INSTRUCTION;
    }
    
    // Вычисляем индекс инструкции из IP (который теперь измеряется в байтах)
    uint16_t instr_index = cpu->IP / INSTRUCTION_SIZE;
    
    // Проверка достигнут ли конец программы
    if (instr_index >= (cpu->memory.instruction_size / INSTRUCTION_SIZE)) {
        cpu->running = 0;  // Остановка эмулятора
        return EMULATOR_HALT;
    }
    
    // Выборка инструкции из памяти
    uint32_t instruction;
    int result = memory_read_instruction(&cpu->memory, instr_index, &instruction);
    
    if (result != MEMORY_SUCCESS) {
        emulator_print_error(EMULATOR_MEMORY_ERROR, "Failed to fetch instruction");
        return EMULATOR_MEMORY_ERROR;
    }
    
    // Декодирование и выполнение инструкции
    return emulator_decode_instruction(cpu, instruction);
}

// Запуск программы
int emulator_run(CPU* cpu) {
    if (!cpu) {
        return EMULATOR_INVALID_INSTRUCTION;
    }
    
    // Установка флага работы
    cpu->running = 1;
    
    // Цикл выполнения программы
    while (cpu->running) {
        int result = emulator_fetch_execute_cycle(cpu);
        
        // Если произошла ошибка или остановка эмулятора
        if (result != EMULATOR_SUCCESS) {
            if (result == EMULATOR_HALT) {
                fprintf(cpu->output_stream, "Program execution completed\n");
                return EMULATOR_SUCCESS;
            } else {
                emulator_print_error(result, "Execution error");
                return result;
            }
        }
    }
    
    return EMULATOR_SUCCESS;
}