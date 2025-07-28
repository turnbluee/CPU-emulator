#include "assemblerHeader.h"
#include "parserHeader.h"

const char* AssemblerErrorMessages[ASSEMBLER_ERROR_COUNT] = {
    "Success",                       // ASSEMBLER_SUCCESS
    "Invalid input file",            // ASSEMBLER_ERROR_INVALID_INPUT
    "Invalid output file",           // ASSEMBLER_ERROR_INVALID_OUTPUT
    "Parser failed",                 // ASSEMBLER_ERROR_PARSER_FAILED
    "Writing failed"                 // ASSEMBLER_ERROR_WRITING_FAILED
};

const char* get_file_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return "";
    }
    return dot + 1;
}

void print_assembler_error(int error_code, const char* custom_message) {
    if (error_code >= 0 && error_code < ASSEMBLER_ERROR_COUNT) {
        if (custom_message) {
            fprintf(stderr, "%s: %s (%d)\n", custom_message, 
                    AssemblerErrorMessages[error_code], error_code);
        } else {
            fprintf(stderr, "Assembler error: %s (%d)\n", 
                    AssemblerErrorMessages[error_code], error_code);
        }
    } else {
        fprintf(stderr, "%s: Unknown error code: %d\n", 
                custom_message ? custom_message : "Assembler error", error_code);
    }
}

int assemble_file(const char* input_filename, const char* output_filename) {
    // Проверка входных параметров
    if (!input_filename || !output_filename) {
        print_assembler_error(ASSEMBLER_ERROR_INVALID_INPUT, "Null filename provided");
        return ASSEMBLER_ERROR_INVALID_INPUT;
    }
    
    // Проверка расширения входного файла (ожидается .asm)
    const char* input_ext = get_file_extension(input_filename);
    if (strcmp(input_ext, "asm") != 0) {
        fprintf(stderr, "Warning: Input file does not have .asm extension: %s\n", input_filename);
        // Это предупреждение, не ошибка
    }
    
    ParseResult parse_result = parse_file(input_filename);
    
    if (parse_result.instruction_count <= 0) {
        print_assembler_error(ASSEMBLER_ERROR_PARSER_FAILED, 
                             "Parser did not produce any instructions");
        return ASSEMBLER_ERROR_PARSER_FAILED;
    }
    
    generate_machine_code_for_all(&parse_result);
    
    FILE* output_file = fopen(output_filename, "wb");
    if (!output_file) {
        print_assembler_error(ASSEMBLER_ERROR_INVALID_OUTPUT, 
                             "Failed to open output file for writing");
        return ASSEMBLER_ERROR_INVALID_OUTPUT;
    }
    
    for (int i = 0; i < parse_result.instruction_count; i++) {
        uint32_t machine_code = parse_result.instructions[i].machine_code;
        
        uint8_t bytes[4];
        bytes[0] = (machine_code >> 24) & 0xFF;
        bytes[1] = (machine_code >> 16) & 0xFF;
        bytes[2] = (machine_code >> 8) & 0xFF;
        bytes[3] = machine_code & 0xFF;
        
        if (fwrite(bytes, sizeof(uint8_t), 4, output_file) != 4) {
            fclose(output_file);
            print_assembler_error(ASSEMBLER_ERROR_WRITING_FAILED, 
                                 "Failed to write machine code to output file");
            return ASSEMBLER_ERROR_WRITING_FAILED;
        }
    }
    
    fclose(output_file);
    printf("Successfully assembled %d instructions to %s\n", 
           parse_result.instruction_count, output_filename);
    
    return ASSEMBLER_SUCCESS;
}
