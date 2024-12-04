import os
import subprocess
import unittest

class TestIFJ24Compiler(unittest.TestCase):
    def setUp(self):
        """Настройка тестовой среды."""
        self.compiler_path = "./ifj24_compiler"  # Путь к компилятору
        self.tests_dir = "tests/"               # Папка с тестовыми файлами
        self.output_file = "output.txt"         # Имя выходного файла

    def run_compiler(self, input_file):
        """Запускает компилятор с указанием входного и фиксированного выходного файлов."""
        try:
            result = subprocess.run(
                [self.compiler_path, input_file, self.output_file],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            return result.returncode, result.stderr
        except FileNotFoundError:
            self.fail(f"Компилятор {self.compiler_path} не найден")

    def tearDown(self):
        """Удаляет временный выходной файл после тестов."""
        if os.path.exists(self.output_file):
            os.remove(self.output_file)

    def test_files(self):
        """Тестирует компилятор на всех тестовых файлах."""
        test_files = [f for f in os.listdir(self.tests_dir) if f.endswith(".ifj24")]

        for test_file in test_files:
            with self.subTest(test_file=test_file):
                input_path = os.path.join(self.tests_dir, test_file)

                # Определяем ожидаемый код возврата
                if "_OK" in test_file:
                    expected_code = 0
                elif "_ERR" in test_file:
                    expected_code = int(test_file.split("_ERR")[-1].split(".")[0])
                else:
                    self.fail(f"Некорректное название файла: {test_file}")

                # Запуск компилятора
                return_code, compiler_errors = self.run_compiler(input_path)

                # Проверка кода возврата
                self.assertEqual(
                    return_code, expected_code,
                    f"Ожидался код {expected_code}, но получен {return_code} для {test_file}. Ошибка: {compiler_errors}"
                )

if __name__ == "__main__":
    unittest.main()
