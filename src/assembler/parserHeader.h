#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Максимальные размеры
#define MAX_LINE_LENGTH 256
#define MAX_TOKENS 32
#define MAX_TOKEN_LENGTH 64
#define MAX_LABELS 256
#define MAX_INSTRUCTION_COUNT 1024

// Коды ошибок парсера
typedef enum {
    PARSER_SUCCESS = 0,
    PARSER_ERROR_INVALID_INSTRUCTION,
    PARSER_ERROR_INVALID_OPERAND,
    PARSER_ERROR_INVALID_REGISTER,
    PARSER_ERROR_INVALID_IMMEDIATE,
    PARSER_ERROR_INVALID_MEM_ACCESS,
    PARSER_ERROR_TOO_MANY_OPERANDS,
    PARSER_ERROR_TOO_FEW_OPERANDS,
    PARSER_ERROR_INVALID_FORMAT,
    PARSER_ERROR_LABEL_ALREADY_DEF,
    PARSER_ERROR_LABEL_NOT_FOUND,
    PARSER_ERROR_FILE_NOT_FOUND,
    PARSER_ERROR_LINE_TOO_LONG,
    PARSER_ERROR_TOO_MANY_INSTR,
    PARSER_ERROR_TOO_MANY_LABELS,
    PARSER_ERROR_COUNT               // Количество кодов ошибок
} ParserErrorCode;

extern const char* ParserErrorMessages[PARSER_ERROR_COUNT];

// Функция извлечения имени файла из пути
const char* get_filename(const char* path);

// Макрос для проверки условия и возврата кода ошибки
#define RETURN_ERROR_IF(condition, error_code) \
do { \
if (condition) { \
return (error_code); \
} \
} while (0)

// Макрос для проверки кода ошибки и возврата из функции, если произошла ошибка
#define RETURN_IF_ERROR(error_code) \
do { \
int _err = (error_code); \
if (_err != PARSER_SUCCESS) { \
return _err; \
} \
} while (0)

// Макрос для проверки кода ошибки и вывода сообщения, если произошла ошибка
#define PRINT_ERROR_IF(error_code, ...) \
do { \
int _err = (error_code); \
if (_err != PARSER_SUCCESS) { \
fprintf(stderr, __VA_ARGS__); \
fprintf(stderr, " Error: %s (%d).\nFile: %s, line: %d.\n", \
ParserErrorMessages[_err], _err, get_filename(__FILE__), __LINE__); \
return _err; \
} \
} while (0)

// Типы токенов
typedef enum {
    TOKEN_NONE,
    TOKEN_LABEL,
    TOKEN_INSTRUCTION,
    TOKEN_REGISTER,
    TOKEN_IMMEDIATE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_IDENTIFIER,
    TOKEN_EOF
} TokenType;

// Коды операций
typedef enum {
    OPC_NOP = 0x00,
    OPC_ADD = 0x01,
    OPC_SUB = 0x02,
    OPC_MUL = 0x03,
    OPC_DIV = 0x04,
    OPC_CMPGE = 0x05,
    OPC_RSHFT = 0x06,
    OPC_LSHFT = 0x07,
    OPC_AND = 0x08,
    OPC_OR = 0x09,
    OPC_XOR = 0x0A,
    OPC_LD = 0x0B,
    OPC_SET_CONST = 0x0C,
    OPC_ST = 0x0D,
    OPC_BNZ = 0x0E,
    OPC_READY = 0x0F
} OpCode;

// Форматы команд
typedef enum {
    FORMAT_F1,  // opc[7:0], src_0[7:0],  src_1[7:0],   dst[7:0]
    FORMAT_F2,  // opc[7:0], const[15:8], const[7:0],   dst[7:0]
    FORMAT_F3,  // opc[7:0], src_0[7:0],  src_1[7:0],   src_2[7:0]
    FORMAT_F4   // opc[7:0], src_0[7:0],  target[15:8], target[7:0]
} InstructionFormat;

// Структура токена
typedef struct {
    TokenType type;
    char value[MAX_TOKEN_LENGTH];
    int line_number;
    unsigned long position;
} Token;

// Структура метки
typedef struct {
    char name[MAX_TOKEN_LENGTH];
    uint16_t address;
} Label;

// Структура для хранения результатов токенизации
typedef struct {
    Token tokens[MAX_TOKENS];
    int token_count;
} TokenizationResult;

// Структура операнда
typedef struct {
    uint8_t reg_num;      // Номер регистра
    uint16_t immediate;   // Непосредственное значение
    char label[MAX_TOKEN_LENGTH];  // Имя метки (если используется)
    int is_memory_access; // Флаг доступа к памяти [reg+reg]
    int is_reg_valid;     // Флаг валидности номера регистра
    int is_immediate_valid; // Флаг валидности непосредственного значения
    int is_label_valid;   // Флаг валидности метки
} Operand;

// Структура инструкции
typedef struct {
    OpCode opcode;
    InstructionFormat format;
    Operand operands[3];  // До 3 операндов
    uint16_t address;     // Адрес инструкции в памяти
    uint32_t machine_code; // Машинный код инструкции
    int operand_count;    // Количество операндов
} Instruction;

// Структура, представляющая результат парсинга ассемблерного файла
typedef struct {
    Instruction instructions[MAX_INSTRUCTION_COUNT];
    int instruction_count;
    Label labels[MAX_LABELS];
    int label_count;
} ParseResult;

// Функции для работы с токенами
TokenizationResult tokenize_line(const char* line, int line_number);
void print_token(const Token* token);
void print_all_tokens(const TokenizationResult* result);

// Функции для работы с метками
int add_label(ParseResult* result, const char* name, uint16_t address);
uint16_t get_label_address(const ParseResult* result, const char* name);

// Функции для парсинга
ParseResult parse_file(const char* filename);
int parse_instruction(ParseResult* result, TokenizationResult* tokens, int* token_idx, uint16_t current_address);
OpCode get_opcode_from_mnemonic(const char* mnemonic);
InstructionFormat get_format_from_opcode(OpCode opcode);
int parse_operand(TokenizationResult* tokens, int* token_idx, Operand* operand);
int parse_register(const Token* token, Operand* operand);
int parse_immediate(const Token* token, Operand* operand);
int parse_memory_access(TokenizationResult* tokens, int* token_idx, Operand* operand);

// Функции для генерации машинного кода
uint32_t generate_machine_code(const Instruction* instruction, const ParseResult* parse_result);
void generate_machine_code_for_all(ParseResult* result);
void write_machine_code_to_file(const ParseResult* result, const char* filename);

// Функции для печати и отладки
void print_instruction(const Instruction* instruction);
void print_parse_result(const ParseResult* result);

#endif // PARSER_H