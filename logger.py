import datetime

class ConsoleColors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def get_time_str():
    return datetime.datetime.now().strftime("%H:%M:%S")

def log_info(module_name, message):
    """Logs standard info to console."""
    print(f"[{get_time_str()}] {ConsoleColors.OKCYAN}[{module_name}]{ConsoleColors.ENDC} {message}")

def log_error(module_name, error, details=""):
    """Logs an error to console with module name in red."""
    print(f"\n{ConsoleColors.FAIL}[" + "="*40 + f"]{ConsoleColors.ENDC}")
    print(f"{ConsoleColors.FAIL}[ERROR]{ConsoleColors.ENDC} A problem occurred in module: {ConsoleColors.BOLD}{module_name}{ConsoleColors.ENDC}")
    print(f"{ConsoleColors.FAIL}[ERROR TYPE]{ConsoleColors.ENDC} {type(error).__name__}")
    print(f"{ConsoleColors.FAIL}[DESCRIPTION]{ConsoleColors.ENDC} {str(error)}")
    if details:
        print(f"{ConsoleColors.WARNING}[DETAILS]{ConsoleColors.ENDC} {details}")
    print(f"{ConsoleColors.FAIL}[" + "="*40 + f"]{ConsoleColors.ENDC}\n")
