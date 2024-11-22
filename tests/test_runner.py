import os
import sys
import subprocess
import re
import logging
from tqdm import tqdm
from concurrent.futures import ThreadPoolExecutor, as_completed

# Unicode color and emoji definitions
USE_THREADING = True
COLOR_GREEN = "\033[92m"  # Green
COLOR_RED = "\033[91m"    # Red
COLOR_YELLOW = "\033[93m" # Yellow
COLOR_BLUE = "\033[94m"   # Blue
COLOR_RESET = "\033[0m"   # Reset
EMOJI_PASS = "✅"
EMOJI_FAIL = "❌"
EMOJI_ERROR = "✖"
EMOJI_SEPARATOR = "🔹"
NEW_LINE_CHAR = '\n'


# Initialize logging
logger = logging.getLogger("helix_tester")

def setup_logging(enable_logging):
    """Set up the logger with console output."""
    log_level = logging.DEBUG if enable_logging else logging.WARNING
    logging.basicConfig(
        level=log_level,
        format="[%(levelname)s] %(message)s",
        handlers=[logging.StreamHandler(sys.stdout)],
    )
    logger.info("Logging enabled")

def validate_folder(folder_path):
    """Validate that the folder exists and contains .hlx files."""
    logger.debug(f"Validating folder: {folder_path}")
    if not os.path.isdir(folder_path):
        logger.error(f"Provided path '{folder_path}' is not a directory.")
        raise ValueError(f"The provided path '{folder_path}' is not a valid directory.")
    hlx_files = [f for f in os.listdir(folder_path) if f.endswith('.hlx')]
    if not hlx_files:
        logger.error(f"No .hlx files found in the directory '{folder_path}'.")
        raise ValueError(f"No .hlx files found in the directory '{folder_path}'.")
    logger.debug(f"Found .hlx files: {hlx_files}")
    return hlx_files

def parse_expected_output(file_path):
    """Extract the expected output or error expectations from the file."""
    logger.debug(f"Parsing expected output from file: {file_path}")
    with open(file_path, 'r') as file:
        content = file.read()
    
    test_match = re.search(r'/\*.*?---------.*?// START TEST(.*?)// END TEST', content, re.DOTALL)
    error_check = re.search(r'//\s*ERRORS', content) is not None

    if test_match:
        expected_lines = test_match.group(1).strip().split('\n')
        filtered_lines = [line for line in expected_lines if line.strip() != '/-ignore-/']
        logger.debug(f"Extracted expected lines: {filtered_lines}, error check: {error_check}")
        return filtered_lines, error_check
    elif error_check:
        logger.debug(f"Error check only (no expected output).")
        return [], True
    else:
        logger.error(f"File '{file_path}' does not contain valid test markers.")
        raise ValueError(f"File '{file_path}' does not contain valid test markers.")

def compile_and_execute(compiler_path, file_path, output_path):
    """Compile and execute the .hlx file."""
    logger.debug(f"Compiling file: {file_path}")
    try:
        # Compile the file
        compile_cmd = [compiler_path, file_path, "--error", "-o", output_path]
        logger.debug(f"Compile command: {' '.join(compile_cmd)}")
        compile_process = subprocess.run(compile_cmd, capture_output=True, text=True)

        if compile_process.returncode != 0:
            logger.error(f"Compilation failed for {file_path}. Error: {compile_process.stderr}")
            return compile_process.stdout, compile_process.stderr, False
        
        if not os.path.exists(output_path):
            # see if theres any error by rerunning the command with --lsp-mode and --emit-ir
            compile_cmd2 = [compiler_path, file_path, "--lsp-mode", "--emit-ir", "-o", output_path]
            logger.debug(f"Compile command: {' '.join(compile_cmd2)}")

            if subprocess.run(compile_cmd2, capture_output=True, text=True).returncode != 0:
                return compile_process.stdout, compile_process.stderr, False
            
            return "", "No output file found", False

        logger.debug(f"Compilation successful for {file_path}")
        # Execute the compiled output
        exec_process = subprocess.run([output_path], capture_output=True, text=True)
        logger.debug(f"Execution output: {exec_process.stdout}")
        return exec_process.stdout.strip(), exec_process.stderr.strip(), True
    finally:
        # Clean up the executable
        if os.path.exists(output_path):
            os.remove(output_path)
            logger.debug(f"Cleaned up executable: {output_path}")

def run_test(compiler_path, folder_path, file_name):
    """Run a single test case."""
    file_path = os.path.join(folder_path, file_name)
    output_path = os.path.join(folder_path, f"{os.path.splitext(file_name)[0]}.out")
    
    logger.info(f"Running test for file: {file_name}")
    expected_output, is_error_check = parse_expected_output(file_path)
    stdout, stderr, compiled = compile_and_execute(compiler_path, file_path, output_path)

    if not compiled:
        logger.debug(f"Test failed during compilation: {stderr}")
        return file_name, False, f"Compilation failed [{' '.join([compiler_path, file_path, '-o', output_path])}]:\n        {NEW_LINE_CHAR + '        '.join(stderr.splitlines())}"

    if is_error_check:
        # Make sure there no stdout and there is only stderr
        if stdout.strip() == "" and stderr.strip() != "":
            logger.info(f"Error check passed for file: {file_name}")
            return file_name, True, "Error check passed."
        else:
            logger.debug(f"Error check failed for file: {file_name}")
            return file_name, False, f"Error check failed.\n" \
                                     f"      {COLOR_YELLOW}Expected:{COLOR_RESET}\n" \
                                     f"        {COLOR_GREEN}Error{COLOR_RESET}\n" \
                                     f"      {COLOR_YELLOW}Got:{COLOR_RESET}\n" \
                                     f"        {COLOR_GREEN}Output{COLOR_RESET}"
    else:
        # Validate standard output
        actual_lines = stdout.split('\n')
        for exp_line, act_line in zip(expected_output, actual_lines):
            if exp_line != act_line:
                logger.debug(f"Output mismatch for {file_name}.")
                string_char = '"'
                return file_name, False, (
                    f"Output mismatch.\n"
                    f"      {COLOR_YELLOW}Expected:{COLOR_RESET}\n"
                    f"        {(NEW_LINE_CHAR + ',        ').join((COLOR_GREEN + string_char + x + string_char + COLOR_RESET) for x in expected_output)},\n"
                    f"      {COLOR_YELLOW}Got:{COLOR_RESET}\n"
                    f"        {(NEW_LINE_CHAR + ',        ').join((COLOR_GREEN + string_char + x + string_char + COLOR_RESET) for x in actual_lines)}"
                )


    logger.info(f"Test passed for file: {file_name}")
    return file_name, True, "Test passed."

def pretty_print_results(results):
    """Pretty print the test results in a TypeScript CLI-like format."""
    total_tests = len(results)
    failed_tests = sum(1 for _, success, _ in results if not success)
    
    print(f"{COLOR_BLUE}[INFO]{COLOR_RESET} Test Results Summary")
    print(f"{'-' * 10}\n")
    
    for file_name, success, message in results:
        if success:
            print(f"{COLOR_GREEN}{EMOJI_PASS} {file_name}{COLOR_RESET}: {message}\n")
        else:
            print(f"{COLOR_RED}{EMOJI_FAIL} {file_name}{COLOR_RESET}: {message}\n")
    
    print(f"{'-' * 10}\n")
    
    if failed_tests > 0:
        print(f"{COLOR_RED}[ERROR]{COLOR_RESET} {failed_tests}/{total_tests} tests failed.")
    else:
        print(f"{COLOR_GREEN}[SUCCESS]{COLOR_RESET} All tests passed!")

def main():
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print("Usage: python test_helix.py <helix-compiler-path> <folder-with-hlx-files> [--log]")
        sys.exit(1)
    
    compiler_path = sys.argv[1]
    folder_path = sys.argv[2]
    enable_logging = len(sys.argv) == 4 and sys.argv[3] == "--log"

    setup_logging(enable_logging)

    if not os.path.isfile(compiler_path):
        logger.error(f"Compiler path '{compiler_path}' is not valid.")
        print(f"Error: Compiler path '{compiler_path}' is not valid.")
        sys.exit(1)

    try:
        hlx_files = validate_folder(folder_path)
    except ValueError as e:
        logger.error(e)
        print(e)
        sys.exit(1)

    results = []
    
    if USE_THREADING:
        with ThreadPoolExecutor() as executor:
            futures = {executor.submit(run_test, compiler_path, folder_path, file): file for file in hlx_files}
            with tqdm(total=len(hlx_files), desc="Processing Tests", unit="file") as pbar:
                for future in as_completed(futures):
                    results.append(future.result())
                    pbar.update(1)
    else:
        with tqdm(total=len(hlx_files), desc="Running Tests", unit="f") as pbar:
            for file in hlx_files:
                results.append(run_test(compiler_path, folder_path, file))
                pbar.update(1)

    # Pretty-print results
    pretty_print_results(results)

    # Exit with appropriate code
    if all(success for _, success, _ in results):
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
