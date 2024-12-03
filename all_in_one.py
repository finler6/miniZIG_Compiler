import os
import subprocess
import sys
import argparse

# Добавляем поддержку цвета
class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'  # Сброс цвета
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def run_command(command, input_data=None):
    try:
        result = subprocess.run(
            command,
            shell=True,
            input=input_data,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        return result.returncode, result.stdout, result.stderr
    except Exception as e:
        return -1, '', str(e)

def parse_args():
    parser = argparse.ArgumentParser(description='IFJ24 Test Runner')
    parser.add_argument('--tests-dir', default='./ifj24', help='Directory containing .ifj24 test files')
    parser.add_argument('--verbose', action='store_true', help='Enable verbose output from the interpreter')
    return parser.parse_args()

def main():
    args = parse_args()
    TESTS_DIR = args.tests_dir
    VERBOSE = args.verbose

    # Проверка наличия компилятора и интерпретатора
    if not os.path.isfile('./ifj24_compiler'):
        print('Ошибка: Компилятор ./ifj24_compiler не найден.')
        sys.exit(1)
    if not os.path.isfile('./ic24int'):
        print('Ошибка: Интерпретатор ./ic24int не найден.')
        sys.exit(1)

    # Список файлов с тестами
    test_files = [f for f in os.listdir(TESTS_DIR) if f.endswith('.ifj24')]
    total_tests = len(test_files)
    passed_tests = 0
    failed_tests = []

    for test_file in test_files:
        print(f'\nТестирование файла: {test_file}')
        test_path = os.path.join(TESTS_DIR, test_file)

        # Определяем, ожидается ли ошибка для этого теста
        expects_error = 'ERR' in test_file.upper()

        # Проверяем наличие файла с входными данными
        input_file = test_file.replace('.ifj24', '.input')
        input_path = os.path.join(TESTS_DIR, input_file)
        if os.path.exists(input_path):
            with open(input_path, 'r') as f:
                input_data = f.read()
        else:
            input_data = '4'  # По умолчанию вводим '4' при отсутствии файла с входными данными

        # Шаг 1: Компиляция
        compile_cmd = f'./ifj24_compiler {test_path} output.txt'
        compile_returncode, compile_stdout, compile_stderr = run_command(compile_cmd)

        if compile_returncode != 0:
            if expects_error:
                print(f"{bcolors.OKGREEN}Ожидаемая ошибка компиляции файла {test_file}.{bcolors.ENDC}")
                passed_tests += 1
            else:
                print(f"{bcolors.FAIL}Неожиданная ошибка компиляции файла {test_file}:{bcolors.ENDC}\n{compile_stderr}")
                failed_tests.append((test_file, 'Compilation Error', compile_stderr))
            continue  # Переходим к следующему тесту

        if expects_error:
            print(f"{bcolors.FAIL}Ожидалась ошибка компиляции файла {test_file}, но компиляция прошла успешно.{bcolors.ENDC}")
            failed_tests.append((test_file, 'Expected Compilation Error', 'Compilation succeeded but was expected to fail.'))
            continue  # Переходим к следующему тесту

        # Шаг 2: Запуск интерпретатора
        interpret_cmd = f'./ic24int output.txt'
        if VERBOSE:
            interpret_cmd += ' -v'
        interpret_returncode, interpret_stdout, interpret_stderr = run_command(interpret_cmd, input_data=input_data)

        if interpret_returncode != 0:
            print(f"{bcolors.FAIL}Ошибка интерпретации файла {test_file}:{bcolors.ENDC}\n{interpret_stderr}")
            failed_tests.append((test_file, 'Interpretation Error', interpret_stderr))
        else:
            print(f"{bcolors.OKGREEN}Тест {test_file} пройден успешно.{bcolors.ENDC}")
            if interpret_stdout.strip():
                print(f'Вывод программы:\n{interpret_stdout}')
            passed_tests += 1

    # Вывод итогового отчета
    print('\n=== Итоговый отчет ===')
    print(f'Всего тестов: {total_tests}')
    print(f'Пройдено успешно: {bcolors.OKGREEN}{passed_tests}{bcolors.ENDC}')
    print(f'Не пройдено: {bcolors.FAIL}{total_tests - passed_tests}{bcolors.ENDC}')

    if failed_tests:
        print('\nДетали неудачных тестов:')
        for test_file, error_type, error_msg in failed_tests:
            print(f'\n{bcolors.FAIL}Файл: {test_file}{bcolors.ENDC}')
            print(f'Тип ошибки: {error_type}')
            print(f'Сообщение об ошибке:\n{error_msg}')

if __name__ == '__main__':
    main()
