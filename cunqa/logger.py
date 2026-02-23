import logging
import sys

RESET = "\033[0m"          
RED = "\033[31m"               
YELLOW = "\033[33m"     
BLUE = "\033[34m"               
GREEN = "\033[32m"              
BRIGHT_RED = "\033[31m\033[1m" 

class ColoredFormatter(logging.Formatter):

    LEVEL_COLOR = {
        logging.DEBUG: BLUE,
        logging.INFO: GREEN,
        logging.WARNING: YELLOW,
        logging.ERROR: BRIGHT_RED,
        logging.CRITICAL: RED,
    }

    def format(self, record):
        color = self.LEVEL_COLOR.get(record.levelno, RESET)
        levelname_lower = record.levelname.lower()

        original_levelname = record.levelname
        record.levelname = levelname_lower

        message = super().format(record)

        record.levelname = original_levelname

        if record.levelno >= logging.ERROR:
            file_info = f"{record.pathname}:{record.lineno}\n"
            colored_message = f"{color}{message}{RESET}"
            message = f"{file_info}{colored_message}"
        else:
            if color:
                message = f"{color}{message}{RESET}"

        return message

logger = logging.getLogger('custom_logger')
logger.setLevel(logging.WARN)
logger.propagate = False
logger.handlers.clear()

console_handler = logging.StreamHandler(sys.stdout)
console_handler.setLevel(logging.WARN)

formatter = ColoredFormatter('\t%(levelname)s: [%(filename)s] %(message)s\n')
console_handler.setFormatter(formatter)

logger.addHandler(console_handler)

logger.info('Logger created.')
