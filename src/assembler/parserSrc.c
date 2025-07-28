#include "parserHeader.h"

const char* ParserErrorMessages[PARSER_ERROR_COUNT] = {
    "Success",                        // PARSER_SUCCESS
    "Invalid instruction",            // PARSER_ERROR_INVALID_INSTRUCTION
    "Invalid operand",                // PARSER_ERROR_INVALID_OPERAND
    "Invalid register",               // PARSER_ERROR_INVALID_REGISTER
    "Invalid immediate value",        // PARSER_ERROR_INVALID_IMMEDIATE
    "Invalid memory access format",   // PARSER_ERROR_INVALID_MEM_ACCESS
    "Too many operands",              // PARSER_ERROR_TOO_MANY_OPERANDS
    "Too few operands",               // PARSER_ERROR_TOO_FEW_OPERANDS
    "Invalid instruction format",     // PARSER_ERROR_INVALID_FORMAT
    "Label already defined",          // PARSER_ERROR_LABEL_ALREADY_DEF
    "Label not found",                // PARSER_ERROR_LABEL_NOT_FOUND
    "File not found",                 // PARSER_ERROR_FILE_NOT_FOUND
    "Line too long",                  // PARSER_ERROR_LINE_TOO_LONG
    "Too many instructions",          // PARSER_ERROR_TOO_MANY_INSTR
    "Too many labels"                 // PARSER_ERROR_TOO_MANY_LABELS
};

const char* get_filename(const char* path) {
    const char* filename = strrchr(path, '/');
    return filename + 1;
}

// Токенизация строки ассемблерного кода
TokenizationResult tokenize_line(const char* line, int line_number) {
    TokenizationResult result = {0};
    char buffer[MAX_LINE_LENGTH] = {0};
    strncpy(buffer, line, MAX_LINE_LENGTH - 1);
    
    // Удаление комментариев
    char* comment = strchr(buffer, ';');
    if (comment) {
        *comment = '\0';
    }
    
    // Обрабатываем квадратные скобки и запятые как отдельные токены
    char processed_buffer[MAX_LINE_LENGTH * 3] = {0};  // В худшем случае будет в 3 раза больше символов
    int proc_pos = 0;
    
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == '[' || buffer[i] == ']' || buffer[i] == ',') {
            // Добавляем пробел перед спецсимволом, если перед ним не пробел
            if (i > 0 && buffer[i-1] != ' ' && buffer[i-1] != '\t') {
                processed_buffer[proc_pos++] = ' ';
            }
            
            // Добавляем специальный символ
            processed_buffer[proc_pos++] = buffer[i];
            
            // Добавляем пробел после спецсимвола, если после него не пробел или не конец строки
            if (buffer[i+1] != '\0' && buffer[i+1] != ' ' && buffer[i+1] != '\t') {
                processed_buffer[proc_pos++] = ' ';
            }
        } else {
            processed_buffer[proc_pos++] = buffer[i];
        }
    }
    
    // Теперь разбиваем строку на токены с помощью пробелов
    char* token_str = strtok(processed_buffer, " \t\n\r");
    unsigned long position = 0;
    
    while (token_str && result.token_count < MAX_TOKENS) {
        Token *token = &result.tokens[result.token_count];
        token->line_number = line_number;
        token->position = position;
        position += strlen(token_str) + 1;
        
        // Определение типа токена
        const unsigned long token_len = strlen(token_str);
        
        if (token_len == 0) {
            continue;  // Пропускаем пустые токены
        }
        
        if (token_str[token_len - 1] == ':') {
            // Метка
            token_str[token_len - 1] = '\0';  // Удаляем ':'
            token->type = TOKEN_LABEL;
        } else if (token_str[0] == 'R' && isdigit(token_str[1])) {
            // Проверяем, что после 'R' следуют только цифры
            int is_valid_register = 1;
            for (int i = 1; token_str[i] != '\0'; i++) {
                if (!isdigit(token_str[i])) {
                    is_valid_register = 0;
                    break;
                }
            }
            
            if (is_valid_register) {
                // Регистр (Rn)
                token->type = TOKEN_REGISTER;
            } else {
                // Невалидный регистр, маркируем как идентификатор
                token->type = TOKEN_IDENTIFIER;
            }
        } else if (isdigit(token_str[0]) || (token_str[0] == '-' && isdigit(token_str[1])) ||
                  (token_str[0] == '0' && token_str[1] == 'x')) {
            // Числовое значение
            token->type = TOKEN_IMMEDIATE;
        } else if (get_opcode_from_mnemonic(token_str) != (OpCode)0xFF) {
            // Инструкция
            token->type = TOKEN_INSTRUCTION;
        } else if (token_len == 1) {
            switch (token_str[0]) {
                case '[':
                    token->type = TOKEN_LBRACKET;
                    break;
                case ']':
                    token->type = TOKEN_RBRACKET;
                    break;
                case ',':
                    token->type = TOKEN_COMMA;
                    break;
                default:
                    token->type = TOKEN_IDENTIFIER;
                    break;
            }
        } else {
            token->type = TOKEN_IDENTIFIER;
        }

        // Копируем значение токена вне зависимости от его типа
        strncpy(token->value, token_str, MAX_TOKEN_LENGTH - 1);
        
        result.token_count++;
        token_str = strtok(NULL, " \t\n\r");
    }
    
    return result;
}

// Получение кода операции из мнемоники
OpCode get_opcode_from_mnemonic(const char* mnemonic) {
    if (strcmp(mnemonic, "nop") == 0) return OPC_NOP;
    else if (strcmp(mnemonic, "add") == 0) return OPC_ADD;
    else if (strcmp(mnemonic, "sub") == 0) return OPC_SUB;
    else if (strcmp(mnemonic, "mul") == 0) return OPC_MUL;
    else if (strcmp(mnemonic, "div") == 0) return OPC_DIV;
    else if (strcmp(mnemonic, "cmpge") == 0) return OPC_CMPGE;
    else if (strcmp(mnemonic, "rshft") == 0) return OPC_RSHFT;
    else if (strcmp(mnemonic, "lshft") == 0) return OPC_LSHFT;
    else if (strcmp(mnemonic, "and") == 0) return OPC_AND;
    else if (strcmp(mnemonic, "or") == 0) return OPC_OR;
    else if (strcmp(mnemonic, "xor") == 0) return OPC_XOR;
    else if (strcmp(mnemonic, "ld") == 0) return OPC_LD;
    else if (strcmp(mnemonic, "set_const") == 0) return OPC_SET_CONST;
    else if (strcmp(mnemonic, "st") == 0) return OPC_ST;
    else if (strcmp(mnemonic, "bnz") == 0) return OPC_BNZ;
    else if (strcmp(mnemonic, "ready") == 0) return OPC_READY;
    else return (OpCode)0xFF;  // Неизвестная инструкция - используем значение 0xFF
}

// Получение формата инструкции по коду операции
InstructionFormat get_format_from_opcode(OpCode opcode) {
    switch (opcode) {
        case OPC_NOP:
        case OPC_ADD:
        case OPC_SUB:
        case OPC_MUL:
        case OPC_DIV:
        case OPC_CMPGE:
        case OPC_RSHFT:
        case OPC_LSHFT:
        case OPC_AND:
        case OPC_OR:
        case OPC_XOR:
        case OPC_LD:
            return FORMAT_F1;  // F1: opc[7:0], src_0[7:0], src_1[7:0], dst[7:0]
            
        case OPC_SET_CONST:
            return FORMAT_F2;  // F2: opc[7:0], const[15:8], const[7:0], dst[7:0]
            
        case OPC_ST:
            return FORMAT_F3;  // F3: opc[7:0], src_0[7:0], src_1[7:0], src_2[7:0]
            
        case OPC_BNZ:
        case OPC_READY:
            return FORMAT_F4;  // F4: opc[7:0], src_0[7:0], target[15:8], target[7:0]
            
        default:
            fprintf(stderr, "Unknown opcode format: %d\n", opcode);
            return (InstructionFormat)0xFF;  // Используем значение 0xFF для неизвестного формата
    }
}

// Парсинг регистра
int parse_register(const Token* token, Operand* operand) {
    // Проверяем, является ли токен регистром
    if (token->type != TOKEN_REGISTER) {
        // Дополнительная проверка для строк вида "Ra", "Rb" и т.д.,
        // которые могут быть распознаны как идентификаторы
        if (token->type == TOKEN_IDENTIFIER && 
            token->value[0] == 'R' && 
            token->value[1] != '\0') {
            fprintf(stderr, "Invalid register format: %s (must be R0-R15)\n", token->value);
            return PARSER_ERROR_INVALID_REGISTER;
        }
        return PARSER_ERROR_INVALID_REGISTER;
    }

    // Строковый анализ - ищем "R" и числовое значение
    if (token->value[0] != 'R' || strlen(token->value) < 2) {
        fprintf(stderr, "Invalid register format: %s (must be R0-R15)\n", token->value);
        return PARSER_ERROR_INVALID_REGISTER;
    }

    // Парсинг номера регистра из Rn
    char* endptr;
    long reg_num = strtol(token->value + 1, &endptr, 10);  // Пропускаем 'R', основание 10

    // Проверка валидности номера регистра
    if (*endptr != '\0' || reg_num < 0 || reg_num > 15) {
        fprintf(stderr, "Invalid register number: %s (must be R0-R15)\n", token->value);
        return PARSER_ERROR_INVALID_REGISTER;
    }

    // Сохраняем номер регистра и устанавливаем флаг валидности
    operand->reg_num = (uint8_t)reg_num;
    operand->is_reg_valid = 1;

    return PARSER_SUCCESS;  // Успешный парсинг
}


// Парсинг непосредственного значения
int parse_immediate(const Token* token, Operand* operand) {
    // Проверяем, является ли токен числом или идентификатором
    RETURN_ERROR_IF(token->type != TOKEN_IMMEDIATE && token->type != TOKEN_IDENTIFIER, 
                   PARSER_ERROR_INVALID_IMMEDIATE);
    
    if (token->type == TOKEN_IMMEDIATE) {
        // Преобразование строки в число
        char* endptr;
        long value;
        
        if (strncmp(token->value, "0x", 2) == 0) {
            // Шестнадцатеричное значение
            value = strtol(token->value + 2, &endptr, 16);
            
            // Проверяем, что все символы шестнадцатеричные
            if (*endptr != '\0') {
                fprintf(stderr, "Invalid hexadecimal format: %s (contains non-hex characters)\n", token->value);
                return PARSER_ERROR_INVALID_IMMEDIATE;
            }
        } else {
            // Десятичное значение
            value = strtol(token->value, &endptr, 10);
            
            // Проверяем, что все символы десятичные
            if (*endptr != '\0') {
                fprintf(stderr, "Invalid decimal format: %s (contains non-decimal characters)\n", token->value);
                return PARSER_ERROR_INVALID_IMMEDIATE;
            }
        }
        
        // Проверка диапазона числа (должно быть 16-битным)
        if (value < -32768 || value > 65535) {
            fprintf(stderr, "Immediate value out of range: %s (must be 16-bit: -32768 to 65535)\n", token->value);
            return PARSER_ERROR_INVALID_IMMEDIATE;
        }
        
        // Сохраняем значение и устанавливаем флаг валидности
        operand->immediate = (uint16_t)value;
        operand->is_immediate_valid = 1;
    } else {
        // Идентификатор (метка для последующего разрешения)
        strncpy(operand->label, token->value, MAX_TOKEN_LENGTH - 1);
        operand->is_label_valid = 1;
    }
    
    return PARSER_SUCCESS;  // Успешный парсинг
}

// Парсинг доступа к памяти [reg+reg]
int parse_memory_access(TokenizationResult* tokens, int* token_idx, Operand* operand) {
    // Определяем формат: [R1,R2] или [R1 R2]
    int has_comma = 0;
    int bracket_offset = 0;
    
    // Вывод для отладки
    #ifdef DEBUG_PARSER
    printf("parse_memory_access: token_idx=%d, token_count=%d\n", *token_idx, tokens->token_count);
    for (int i = 0; i < tokens->token_count; i++) {
        printf("Token %d: type=%d, value=%s\n", i, tokens->tokens[i].type, tokens->tokens[i].value);
    }
    #endif
    
    // Проверка формата [reg,reg]
    if (*token_idx + 4 < tokens->token_count &&
        tokens->tokens[*token_idx].type == TOKEN_LBRACKET &&
        tokens->tokens[*token_idx + 1].type == TOKEN_REGISTER &&
        tokens->tokens[*token_idx + 2].type == TOKEN_COMMA &&
        tokens->tokens[*token_idx + 3].type == TOKEN_REGISTER &&
        tokens->tokens[*token_idx + 4].type == TOKEN_RBRACKET) {
        
        has_comma = 1;
        bracket_offset = 4;
    }
    // Проверка формата [reg reg]
    else if (*token_idx + 3 < tokens->token_count &&
             tokens->tokens[*token_idx].type == TOKEN_LBRACKET &&
             tokens->tokens[*token_idx + 1].type == TOKEN_REGISTER &&
             tokens->tokens[*token_idx + 2].type == TOKEN_REGISTER &&
             tokens->tokens[*token_idx + 3].type == TOKEN_RBRACKET) {
        
        has_comma = 0;
        bracket_offset = 3;
    }
    // Попытка найти более свободную структуру с возможным перемешиванием токенов
    else if (*token_idx < tokens->token_count && tokens->tokens[*token_idx].type == TOKEN_LBRACKET) {
        // Ищем закрывающую скобку
        int rbracket_idx = -1;
        for (int i = *token_idx + 1; i < tokens->token_count; i++) {
            if (tokens->tokens[i].type == TOKEN_RBRACKET) {
                rbracket_idx = i;
                break;
            }
        }
        
        if (rbracket_idx != -1) {
            // Ищем два регистра между скобками
            int reg1_idx = -1, reg2_idx = -1;
            
            for (int i = *token_idx + 1; i < rbracket_idx; i++) {
                if (tokens->tokens[i].type == TOKEN_REGISTER) {
                    if (reg1_idx == -1) {
                        reg1_idx = i;
                    } else {
                        reg2_idx = i;
                        break;
                    }
                }
            }
            
            if (reg1_idx != -1 && reg2_idx != -1) {
                // Нашли два регистра внутри скобок
                // Парсим регистры напрямую
                Operand reg1 = {0};
                int error_code = parse_register(&tokens->tokens[reg1_idx], &reg1);
                if (error_code != PARSER_SUCCESS) {
                    return error_code;
                }
                
                Operand reg2 = {0};
                error_code = parse_register(&tokens->tokens[reg2_idx], &reg2);
                if (error_code != PARSER_SUCCESS) {
                    return error_code;
                }
                
                // Устанавливаем операнды
                operand->reg_num = reg1.reg_num;
                operand->immediate = reg2.reg_num;
                operand->is_reg_valid = 1;
                operand->is_immediate_valid = 1;
                operand->is_memory_access = 1;
                
                // Устанавливаем указатель токена после закрывающей скобки
                *token_idx = rbracket_idx + 1;
                
                return PARSER_SUCCESS;
            }
        }
        
        // Если не нашли нужных токенов, возвращаем ошибку
        return PARSER_ERROR_INVALID_MEM_ACCESS;
    }
    else {
        // Ни один формат не подходит
        return PARSER_ERROR_INVALID_MEM_ACCESS;
    }
    
    // Парсинг первого регистра
    Operand reg1 = {0};
    int error_code = parse_register(&tokens->tokens[*token_idx + 1], &reg1);
    RETURN_IF_ERROR(error_code);
    
    // Парсинг второго регистра
    Operand reg2 = {0};
    int second_reg_idx = has_comma ? (*token_idx + 3) : (*token_idx + 2);
    error_code = parse_register(&tokens->tokens[second_reg_idx], &reg2);
    RETURN_IF_ERROR(error_code);
    
    // Установка операндов и флага доступа к памяти
    operand->reg_num = reg1.reg_num;
    operand->immediate = reg2.reg_num;  // Используем immediate для хранения второго регистра
    operand->is_reg_valid = 1;
    operand->is_immediate_valid = 1;
    operand->is_memory_access = 1;
    
    // В формате [reg,reg] - 5 токенов: [, reg, comma, reg, ]
    // В формате [reg reg] - 4 токена: [, reg, reg, ]
    *token_idx = *token_idx + bracket_offset + 1;  // Перемещаем указатель токена после закрывающей скобки
    
    return PARSER_SUCCESS;  // Успешный парсинг
}

// Парсинг операнда
int parse_operand(TokenizationResult* tokens, int* token_idx, Operand* operand) {
    // Проверка индекса токена
    RETURN_ERROR_IF(*token_idx >= tokens->token_count, PARSER_ERROR_INVALID_OPERAND);
    
    // Инициализация операнда
    memset(operand, 0, sizeof(Operand));
    
    // Пытаемся разобрать доступ к памяти [reg+reg]
    if (tokens->tokens[*token_idx].type == TOKEN_LBRACKET) {
        int error_code = parse_memory_access(tokens, token_idx, operand);
        if (error_code == PARSER_SUCCESS) {
            // Указатель token_idx уже обновлен в parse_memory_access
            return PARSER_SUCCESS;  // Успешный парсинг памяти
        }
        // Если ошибка не связана с форматом доступа к памяти, возвращаем ее
        if (error_code != PARSER_ERROR_INVALID_MEM_ACCESS) {
            return error_code;
        }
        // Иначе возвращаем ошибку неправильного операнда
        return PARSER_ERROR_INVALID_OPERAND;
    }
    
    // Пытаемся разобрать регистр
    int error_code = parse_register(&tokens->tokens[*token_idx], operand);
    if (error_code == PARSER_SUCCESS) {
        (*token_idx)++;
        return PARSER_SUCCESS;  // Успешный парсинг регистра
    }
    
    // Специальная обработка для R16 и других недопустимых регистров
    if (tokens->tokens[*token_idx].type == TOKEN_REGISTER) {
        // Это токен типа REGISTER, но parse_register вернул ошибку
        // Значит, это регистр с недопустимым номером (например, R16)
        return PARSER_ERROR_INVALID_REGISTER;
    }
    
    // Пытаемся разобрать непосредственное значение или метку
    error_code = parse_immediate(&tokens->tokens[*token_idx], operand);
    if (error_code == PARSER_SUCCESS) {
        // Проверяем идентификаторы - если это символ, похожий на регистр (Ra),
        // но не являющийся правильным регистром, возвращаем ошибку
        if (operand->is_label_valid) {
            // Проверяем формат 'Rx', где x - не число
            if (operand->label[0] == 'R' && operand->label[1] != '\0') {
                return PARSER_ERROR_INVALID_REGISTER;
            }
            
            // Проверяем случайные символы, которые не могут быть метками
            int is_valid_label = 1;
            for (int i = 0; operand->label[i] != '\0'; i++) {
                char c = operand->label[i];
                if (!(isalnum(c) || c == '_')) {
                    is_valid_label = 0;
                    break;
                }
            }
            
            if (!is_valid_label) {
                return PARSER_ERROR_INVALID_OPERAND;
            }
        }
        
        (*token_idx)++;
        return PARSER_SUCCESS;  // Успешный парсинг непосредственного значения или метки
    } else {
        // Сохраняем исходную ошибку для непосредственных значений
        return PARSER_ERROR_INVALID_IMMEDIATE;
    }
}

// Парсинг инструкции
int parse_instruction(ParseResult* result, TokenizationResult* tokens, int* token_idx, uint16_t current_address) {
    // Проверка границ
    RETURN_ERROR_IF(*token_idx >= tokens->token_count, PARSER_ERROR_INVALID_INSTRUCTION);
    
    // Проверка, не превышено ли максимальное количество инструкций
    PRINT_ERROR_IF(result->instruction_count >= MAX_INSTRUCTION_COUNT, 
                  "Too many instructions (max %d)\n", MAX_INSTRUCTION_COUNT);
    
    Token* token = &tokens->tokens[*token_idx];
    
    // Проверяем, является ли токен инструкцией
    RETURN_ERROR_IF(token->type != TOKEN_INSTRUCTION, PARSER_ERROR_INVALID_INSTRUCTION);
    
    // Получаем код операции и формат
    OpCode opcode = get_opcode_from_mnemonic(token->value);
    if (opcode == (OpCode)0xFF) {
        fprintf(stderr, "Unknown instruction: %s\n", token->value);
        return PARSER_ERROR_INVALID_INSTRUCTION;
    }
    
    InstructionFormat format = get_format_from_opcode(opcode);
    if (format == (InstructionFormat)0xFF) {
        fprintf(stderr, "Unknown instruction format for opcode: %d\n", opcode);
        return PARSER_ERROR_INVALID_FORMAT;
    }
    
    // Создаем новую инструкцию
    Instruction* instr = &result->instructions[result->instruction_count];
    instr->opcode = opcode;
    instr->format = format;
    instr->address = current_address;
    instr->operand_count = 0;
    
    // Переходим к следующему токену (после инструкции)
    (*token_idx)++;
    
    // Парсинг операндов
    int max_operands = 0;
    switch (format) {
        case FORMAT_F1: max_operands = 3; break;  // src0, src1, dst
        case FORMAT_F2: max_operands = 2; break;  // const, dst
        case FORMAT_F3: max_operands = 3; break;  // src0, src1, src2
        case FORMAT_F4: max_operands = 2; break;  // target, src0
    }
    
    // Специальный случай для nop и ready, которые не имеют операндов
    if (opcode == OPC_NOP || opcode == OPC_READY) {
        max_operands = 0;
    }
    
    // clang-tidy: Недостижимый код
    // После этих проверок код может быть недостижим для определенных опкодов
    
    // Парсинг операндов
    for (int i = 0; i < max_operands; i++) {
        int error_code;
        
        // Специальная обработка для инструкции set_const, где первый операнд - непосредственное значение
        if (format == FORMAT_F2 && i == 0) {
            // Парсим непосредственное значение напрямую
            Token* immediate_token = &tokens->tokens[*token_idx];
            
            // Инициализируем операнд
            memset(&instr->operands[i], 0, sizeof(Operand));
            
            // Проверяем, что токен это непосредственное значение
            if (immediate_token->type != TOKEN_IMMEDIATE && immediate_token->type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Expected immediate value for the first operand of set_const, got %d\n", 
                        immediate_token->type);
                return PARSER_ERROR_INVALID_IMMEDIATE;
            }
            
            // Парсим непосредственное значение
            error_code = parse_immediate(immediate_token, &instr->operands[i]);
            if (error_code == PARSER_SUCCESS) {
                (*token_idx)++;
                
                // Проверяем, есть ли запятая после непосредственного значения
                if (*token_idx < tokens->token_count && tokens->tokens[*token_idx].type == TOKEN_COMMA) {
                    // Запятая обнаружена, пропускаем её
                    (*token_idx)++;
                } else {
                    // Запятая не обнаружена, но это не фатальная ошибка
                    // Некоторые синтаксисы ассемблера могут не требовать запятую между операндами
                    fprintf(stderr, "Warning: No comma found after first operand for set_const instruction at line %d\n", 
                            tokens->tokens[0].line_number);
                }
            }
        } else {
            // Стандартный парсинг операнда для других случаев
            error_code = parse_operand(tokens, token_idx, &instr->operands[i]);
            
            // Если мы успешно разобрали операнд и это не последний операнд,
            // проверим, есть ли запятая после и пропустим её
            if (error_code == PARSER_SUCCESS && i < max_operands - 1) {
                if (*token_idx < tokens->token_count && tokens->tokens[*token_idx].type == TOKEN_COMMA) {
                    // Пропускаем запятую между операндами
                    (*token_idx)++;
                }
            }
        }
        
        if (error_code != PARSER_SUCCESS) {
            // clang-tidy: это выражение может быть всегда истинно для определенных опкодов
            if (i == 0 && (opcode == OPC_NOP || opcode == OPC_READY)) {
                // Для nop и ready операнды не требуются
                break;
            }
            
            // Выводим сообщение об ошибке и возвращаем код ошибки
            if (i == 0) {
                fprintf(stderr, "Failed to parse first operand for instruction %s\n", token->value);
            } else if (i == max_operands - 1) {
                fprintf(stderr, "Failed to parse last operand for instruction %s\n", token->value);
            } else {
                fprintf(stderr, "Failed to parse operand %d for instruction %s\n", i + 1, token->value);
            }
            return error_code;
        }
        instr->operand_count++;
    }
    
    // Увеличиваем счетчик инструкций
    result->instruction_count++;
    
    return PARSER_SUCCESS;  // Успешный парсинг инструкции
}

// Добавление метки
int add_label(ParseResult* result, const char* name, uint16_t address) {
    // Проверка, не превышено ли максимальное количество меток
    PRINT_ERROR_IF(result->label_count >= MAX_LABELS, 
                  "Too many labels (max %d)", MAX_LABELS);
    
    // Проверка на повторное определение метки
    for (int i = 0; i < result->label_count; i++) {
        if (strcmp(result->labels[i].name, name) == 0) {
            fprintf(stderr, "Label already defined: %s\n", name);
            return PARSER_ERROR_LABEL_ALREADY_DEF;
        }
    }
    
    // Добавление новой метки
    Label* label = &result->labels[result->label_count];
    strncpy(label->name, name, MAX_TOKEN_LENGTH - 1);
    label->address = address;
    
    result->label_count++;
    
    return PARSER_SUCCESS;  // Успешное добавление метки
}

// Получение адреса метки
uint16_t get_label_address(const ParseResult* result, const char* name) {
    for (int i = 0; i < result->label_count; i++) {
        if (strcmp(result->labels[i].name, name) == 0) {
            return result->labels[i].address;
        }
    }
    
    // Метка не найдена, выводим сообщение об ошибке
    fprintf(stderr, "Label not found: %s\n", name);
    fprintf(stderr, "Error: %s (%d).\nFile: %s, line: %d.\n", 
            ParserErrorMessages[PARSER_ERROR_LABEL_NOT_FOUND], 
            PARSER_ERROR_LABEL_NOT_FOUND, 
            get_filename(__FILE__), __LINE__);
            
    return 0xFFFF;  // Недопустимый адрес
}

// Парсинг ассемблерного файла
ParseResult parse_file(const char* filename) {
    ParseResult result = {0};
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        fprintf(stderr, "Error: %s (%d).\nFile: %s, line: %d.\n", 
                ParserErrorMessages[PARSER_ERROR_FILE_NOT_FOUND], 
                PARSER_ERROR_FILE_NOT_FOUND, 
                get_filename(__FILE__), __LINE__);
        return result;
    }
    
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    uint16_t current_address = 0;
    
    // Первый проход: сбор меток
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        line_number++;
        
        // Проверка длины строки
        if (strlen(line) >= MAX_LINE_LENGTH - 1) {
            fprintf(stderr, "Line %d is too long (max %d characters)\n", line_number, MAX_LINE_LENGTH - 1);
            fprintf(stderr, "Error: %s (%d).\nFile: %s, line: %d.\n", 
                    ParserErrorMessages[PARSER_ERROR_LINE_TOO_LONG], 
                    PARSER_ERROR_LINE_TOO_LONG, 
                    get_filename(__FILE__), __LINE__);
            fclose(file);
            return result;
        }
        
        // Токенизация строки
        TokenizationResult tokens = tokenize_line(line, line_number);
        
        if (tokens.token_count == 0) {
            continue;  // Пустая строка или комментарий
        }
        
        int token_idx = 0;
        
        // Поиск меток
        while (token_idx < tokens.token_count) {
            Token* token = &tokens.tokens[token_idx];
            
            if (token->type == TOKEN_LABEL) {
                int error_code = add_label(&result, token->value, current_address);
                if (error_code != PARSER_SUCCESS) {
                    fprintf(stderr, "Error in line %d: ", line_number);
                    fprintf(stderr, "Error: %s (%d).\nFile: %s, line: %d.\n", 
                            ParserErrorMessages[error_code], 
                            error_code, 
                            get_filename(__FILE__), __LINE__);
                    fclose(file);
                    return result;
                }
                token_idx++;
            } else if (token->type == TOKEN_INSTRUCTION) {
                // Инструкция занимает 4 байта
                current_address += 4;
                break;
            } else {
                token_idx++;
            }
        }
    }
    
    // Сброс файла и счетчиков для второго прохода
    rewind(file);
    line_number = 0;
    current_address = 0;
    
    // Второй проход: парсинг инструкций
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        line_number++;
        
        // Токенизация строки
        TokenizationResult tokens = tokenize_line(line, line_number);
        
        if (tokens.token_count == 0) {
            continue;  // Пустая строка или комментарий
        }
        
        int token_idx = 0;
        
        // Пропуск меток
        while (token_idx < tokens.token_count && tokens.tokens[token_idx].type == TOKEN_LABEL) {
            token_idx++;
        }
        
        // Парсинг инструкций
        if (token_idx < tokens.token_count && tokens.tokens[token_idx].type == TOKEN_INSTRUCTION) {
            int error_code = parse_instruction(&result, &tokens, &token_idx, current_address);
            if (error_code != PARSER_SUCCESS) {
                fprintf(stderr, "Failed to parse instruction at line %d\n", line_number);
                fprintf(stderr, "Error: %s (%d).\nFile: %s, line: %d.\n", 
                        ParserErrorMessages[error_code], 
                        error_code, 
                        get_filename(__FILE__), __LINE__);
                fclose(file);
                return result;
            }
            
            // Инструкция занимает 4 байта
            current_address += 4;
        }
    }
    
    fclose(file);
    
    // Генерация машинного кода для всех инструкций
    generate_machine_code_for_all(&result);
    
    return result;
}

// Генерация машинного кода для инструкции
uint32_t generate_machine_code(const Instruction* instruction, const ParseResult* parse_result) {
    uint32_t machine_code = 0;
    
    // Установка кода операции в старшие биты
    machine_code |= ((uint32_t)instruction->opcode) << 24;
    
    switch (instruction->format) {
        case FORMAT_F1:  // opc[7:0], src_0[7:0], src_1[7:0], dst[7:0]
            if (instruction->operand_count >= 3) {
                machine_code |= ((uint32_t)instruction->operands[0].reg_num) << 16;
                machine_code |= ((uint32_t)instruction->operands[1].reg_num) << 8;
                machine_code |= instruction->operands[2].reg_num;
            }
            break;
            
        case FORMAT_F2:  // opc[7:0], const[15:8], const[7:0], dst[7:0]
            if (instruction->operand_count >= 2) {
                uint16_t constant = instruction->operands[0].immediate;
                machine_code |= ((uint32_t)(constant >> 8)) << 16;
                machine_code |= ((uint32_t)(constant & 0xFF)) << 8;
                machine_code |= instruction->operands[1].reg_num;
            }
            break;
            
        case FORMAT_F3:  // opc[7:0], src_0[7:0], src_1[7:0], src_2[7:0]
            if (instruction->operand_count >= 3) {
                machine_code |= ((uint32_t)instruction->operands[0].reg_num) << 16;
                machine_code |= ((uint32_t)instruction->operands[1].reg_num) << 8;
                machine_code |= instruction->operands[2].reg_num;
            }
            break;
            
        case FORMAT_F4:  // opc[7:0], src_0[7:0], target[15:8], target[7:0]
            if (instruction->operand_count >= 2) {
                uint16_t target;
                
                // Если используется метка, получаем ее адрес
                if (instruction->operands[0].is_label_valid) {
                    target = get_label_address(parse_result, instruction->operands[0].label);
                } else {
                    target = instruction->operands[0].immediate;
                }
                
                machine_code |= ((uint32_t)instruction->operands[1].reg_num) << 16;
                machine_code |= ((uint32_t)(target >> 8)) << 8;
                machine_code |= (target & 0xFF);
            }
            break;
            
        default:
            fprintf(stderr, "Unknown instruction format: %d\n", instruction->format);
            return 0;
    }
    
    return machine_code;
}

// Генерация машинного кода для всех инструкций
void generate_machine_code_for_all(ParseResult* result) {
    for (int i = 0; i < result->instruction_count; i++) {
        result->instructions[i].machine_code = generate_machine_code(&result->instructions[i], result);
    }
}

// Запись машинного кода в файл
void write_machine_code_to_file(const ParseResult* result, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing: %s\n", filename);
        return;
    }
    
    for (int i = 0; i < result->instruction_count; i++) {
        uint32_t machine_code = result->instructions[i].machine_code;
        
        // Запись машинного кода в формате Big Endian
        uint8_t bytes[4];
        bytes[0] = (machine_code >> 24) & 0xFF;
        bytes[1] = (machine_code >> 16) & 0xFF;
        bytes[2] = (machine_code >> 8) & 0xFF;
        bytes[3] = machine_code & 0xFF;
        
        fwrite(bytes, sizeof(uint8_t), 4, file);
    }
    
    fclose(file);
}

// Функции печати для отладки

// Печать токена
void print_token(const Token* token) {
    const char* type_str;
    
    switch (token->type) {
        case TOKEN_NONE: type_str = "NONE"; break;
        case TOKEN_LABEL: type_str = "LABEL"; break;
        case TOKEN_INSTRUCTION: type_str = "INSTRUCTION"; break;
        case TOKEN_REGISTER: type_str = "REGISTER"; break;
        case TOKEN_IMMEDIATE: type_str = "IMMEDIATE"; break;
        case TOKEN_LBRACKET: type_str = "LBRACKET"; break;
        case TOKEN_RBRACKET: type_str = "RBRACKET"; break;
        case TOKEN_COMMA: type_str = "COMMA"; break;
        case TOKEN_IDENTIFIER: type_str = "IDENTIFIER"; break;
        case TOKEN_EOF: type_str = "EOF"; break;
        default: type_str = "UNKNOWN";
    }
    
    printf("Token{type=%s, value=\"%s\", line=%d, pos=%lu}\n",
           type_str, token->value, token->line_number, token->position);
}

// Печать всех токенов
void print_all_tokens(const TokenizationResult* result) {
    printf("TokenizationResult{count=%d, tokens=[\n", result->token_count);
    
    for (int i = 0; i < result->token_count; i++) {
        printf("  ");
        print_token(&result->tokens[i]);
    }
    
    printf("]}\n");
}

// Печать инструкции
void print_instruction(const Instruction* instruction) {
    const char* opcode_str;
    
    switch (instruction->opcode) {
        case OPC_NOP: opcode_str = "NOP"; break;
        case OPC_ADD: opcode_str = "ADD"; break;
        case OPC_SUB: opcode_str = "SUB"; break;
        case OPC_MUL: opcode_str = "MUL"; break;
        case OPC_DIV: opcode_str = "DIV"; break;
        case OPC_CMPGE: opcode_str = "CMPGE"; break;
        case OPC_RSHFT: opcode_str = "RSHFT"; break;
        case OPC_LSHFT: opcode_str = "LSHFT"; break;
        case OPC_AND: opcode_str = "AND"; break;
        case OPC_OR: opcode_str = "OR"; break;
        case OPC_XOR: opcode_str = "XOR"; break;
        case OPC_LD: opcode_str = "LD"; break;
        case OPC_SET_CONST: opcode_str = "SET_CONST"; break;
        case OPC_ST: opcode_str = "ST"; break;
        case OPC_BNZ: opcode_str = "BNZ"; break;
        case OPC_READY: opcode_str = "READY"; break;
        default: opcode_str = "UNKNOWN";
    }
    
    const char* format_str;
    
    switch (instruction->format) {
        case FORMAT_F1: format_str = "F1"; break;
        case FORMAT_F2: format_str = "F2"; break;
        case FORMAT_F3: format_str = "F3"; break;
        case FORMAT_F4: format_str = "F4"; break;
        default: format_str = "UNKNOWN";
    }
    
    printf("Instruction{opcode=%s, format=%s, address=0x%04X, machine_code=0x%08X, operand_count=%d, operands=[\n",
           opcode_str, format_str, instruction->address, instruction->machine_code, instruction->operand_count);
    
    for (int i = 0; i < instruction->operand_count; i++) {
        const Operand* op = &instruction->operands[i];
        
        printf("  Operand{reg_num=%d, immediate=0x%04X, label=\"%s\", is_memory_access=%d, is_reg_valid=%d, is_immediate_valid=%d, is_label_valid=%d}\n",
               op->reg_num, op->immediate, op->label, op->is_memory_access,
               op->is_reg_valid, op->is_immediate_valid, op->is_label_valid);
    }
    
    printf("]}\n");
}

// Печать результата парсинга
void print_parse_result(const ParseResult* result) {
    printf("ParseResult{instruction_count=%d, label_count=%d, instructions=[\n", 
           result->instruction_count, result->label_count);
    
    for (int i = 0; i < result->instruction_count; i++) {
        printf("  ");
        print_instruction(&result->instructions[i]);
    }
    
    printf("], labels=[\n");
    
    for (int i = 0; i < result->label_count; i++) {
        printf("  Label{name=\"%s\", address=0x%04X}\n", 
               result->labels[i].name, result->labels[i].address);
    }
    
    printf("]}\n");
}