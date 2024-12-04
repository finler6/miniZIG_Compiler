import os
import subprocess
import sys
import argparse


def run_command(command):
    try:
        result = subprocess.run(
            command,
            shell=True,
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

        # Проверяем наличие файла с входными данными
        input_file = test_file.replace('.ifj24', '.input')
        input_path = os.path.join(TESTS_DIR, input_file)
        if os.path.exists(input_path):
            with open(input_path, 'r') as f:
                input_data = f.read()
        else:
            input_data = '4'

        # Шаг 1: Компиляция
        compile_cmd = f'./ifj24_compiler {test_path} output.txt'
        compile_returncode, compile_stdout, compile_stderr = run_command(compile_cmd)

        if compile_returncode != 0:
            print(f'Ошибка компиляции файла {test_file}:\n{compile_stderr}')
            failed_tests.append((test_file, 'Compilation Error', compile_stderr))
            continue  # Переходим к следующему тесту

        # Шаг 2: Запуск интерпретатора
        interpret_cmd = f'./ic24int output.txt'
        if VERBOSE:
            interpret_cmd += ' -v'
        try:
            result = subprocess.run(
                interpret_cmd,
                shell=True,
                input=input_data,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            interpret_returncode = result.returncode
            interpret_stdout = result.stdout
            interpret_stderr = result.stderr
        except Exception as e:
            interpret_returncode = -1
            interpret_stdout = ''
            interpret_stderr = str(e)

        if interpret_returncode != 0:
            print(f'Ошибка интерпретации файла {test_file}:\n{interpret_stderr}')
            failed_tests.append((test_file, 'Interpretation Error', interpret_stderr))
        else:
            print(f'Тест {test_file} пройден успешно.')
            if interpret_stdout.strip():
                print(f'Вывод программы:\n{interpret_stdout}')
            passed_tests += 1

    # Вывод итогового отчета
    print('\n=== Итоговый отчет ===')
    print(f'Всего тестов: {total_tests}')
    print(f'Пройдено успешно: {passed_tests}')
    print(f'Не пройдено: {total_tests - passed_tests}')

    if failed_tests:
        print('\nДетали неудачных тестов:')
        for test_file, error_type, error_msg in failed_tests:
            print(f'\nФайл: {test_file}')
            print(f'Тип ошибки: {error_type}')
            print(f'Сообщение об ошибке:\n{error_msg}')


if __name__ == '__main__':
    main()
