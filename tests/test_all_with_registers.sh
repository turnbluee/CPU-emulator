#!/bin/bash
# Установка цветов для вывода
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Проверка наличия ассемблера и эмулятора
if [ ! -f ../assembler ] || [ ! -f ../emulator ]; then
    echo -e "${RED}Ошибка: Ассемблер или эмулятор не найден${NC}"
    echo "Убедитесь, что вы находитесь в правильной директории, и проект скомпилирован"
    exit 1
fi

echo -e "${YELLOW}Компиляция и запуск всех тестов CPU эмулятора с выводом регистров${NC}"
echo "======================================================="

# Функция для компиляции и запуска теста
run_test() {
    local test_name=$(basename "$1" .asm)
    local asm_file="$1"
    local bin_file="${test_name}.bin"
    
    echo -e "${YELLOW}Тест: ${test_name}${NC}"
    
    # Компиляция
    echo -n "  Компиляция... "
    ../assembler "$asm_file" "$bin_file" > /dev/null
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}ОШИБКА: Не удалось скомпилировать $asm_file${NC}"
        return 1
    else
        echo -e "${GREEN}OK${NC}"
    fi
    
    # Запуск с ограничением по времени (5 секунд) и получением значений регистров
    echo -n "  Запуск... "
    
    # Создаем временный файл для вывода
    LOG_FILE=$(mktemp)
    
    # Запускаем с флагом -v (verbose) для получения значений регистров
    timeout 5s ../emulator -v "$bin_file" > "$LOG_FILE" 2>&1
    EXIT_CODE=$?
    
    if [ $EXIT_CODE -eq 124 ]; then
        echo -e "${RED}ОШИБКА: Тест превысил лимит времени (5 секунд)${NC}"
        echo "  Возможно, присутствует бесконечный цикл"
        cat "$LOG_FILE"
        rm "$LOG_FILE"
        return 1
    elif [ $EXIT_CODE -ne 0 ] && [ $EXIT_CODE -ne 5 ]; then
        # Код 5 (EMULATOR_HALT) - это нормальное завершение программы по инструкции READY
        echo -e "${RED}ОШИБКА: Тест завершился с кодом $EXIT_CODE${NC}"
        echo "  Запустите тест с отладкой для получения подробной информации:"
        echo "  ../emulator $bin_file"
        cat "$LOG_FILE"
        rm "$LOG_FILE"
        return 1
    else
        echo -e "${GREEN}УСПЕХ${NC}"
        
        # Извлекаем и показываем состояние регистров из лога
        echo -e "  ${BLUE}Финальное состояние регистров:${NC}"
        REGISTERS=$(grep -A20 "CPU Registers:" "$LOG_FILE" | grep -v -E "^$|^--$")
        if [ -n "$REGISTERS" ]; then
            echo "$REGISTERS" | sed 's/^/    /'
        else
            echo "    Информация о регистрах недоступна"
        fi
        
        # Удаляем временный файл
        rm "$LOG_FILE"
    fi
    
    return 0
}

# Поиск всех тестов
TEST_FILES=$(ls *.asm)
TOTAL_TESTS=$(echo "$TEST_FILES" | wc -l)
PASSED_TESTS=0

echo "Найдено тестов: $TOTAL_TESTS"
echo "======================================================="

# Обработка каждого теста
for test in $TEST_FILES; do
    run_test "$test"
    
    if [ $? -eq 0 ]; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi
    
    echo ""
done

# Вывод итогов
echo "======================================================="
echo -e "${YELLOW}Итоги тестирования:${NC}"
echo "  Всего тестов: $TOTAL_TESTS"
echo "  Успешно пройдено: $PASSED_TESTS"

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo -e "${GREEN}Все тесты успешно пройдены!${NC}"
else
    echo -e "${RED}Не все тесты пройдены успешно.${NC}"
    echo "Проверьте логи и устраните ошибки в непройденных тестах."
fi

exit 0