import logging
import sys

# Códigos de colores ANSI
RESET = "\033[0m"              # Restablece el color al predeterminado
RED = "\033[31m"                # Rojo para ERROR
YELLOW = "\033[33m"             # Amarillo para WARNING
BLUE = "\033[34m"               # Azul para DEBUG
GREEN = "\033[32m"              # Verde para INFO
BRIGHT_RED = "\033[31m\033[1m"  # Rojo brillante para CRITICAL

class ColoredFormatter(logging.Formatter):
    """Formatter que añade colores, convierte niveles a minúsculas y añade
    ruta completa del archivo y línea para errores."""

    # Mapeo de niveles a colores
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

logger = logging.getLogger('mi_logger_coloreado')
logger.setLevel(logging.WARN)
logger.propagate = False
logger.handlers.clear()

console_handler = logging.StreamHandler(sys.stdout)
console_handler.setLevel(logging.WARN)

formatter = ColoredFormatter('\t%(levelname)s: [%(filename)s] %(message)s\n')
console_handler.setFormatter(formatter)

logger.addHandler(console_handler)

logger.info('Logger created.')
