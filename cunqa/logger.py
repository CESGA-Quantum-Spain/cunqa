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
        # Obtener el color correspondiente al nivel de log
        color = self.LEVEL_COLOR.get(record.levelno, RESET)

        # Convertir el nivel de log a minúsculas
        levelname_lower = record.levelname.lower()

        # Reemplazar temporalmente el nivel de log original con el minúsculo
        original_levelname = record.levelname
        record.levelname = levelname_lower

        # Formatear el mensaje original según el formato especificado
        message = super().format(record)

        # Restaurar el nivel de log original para evitar efectos secundarios
        record.levelname = original_levelname

        # Incluir ruta completa del archivo y número de línea solo para ERROR y CRITICAL
        if record.levelno >= logging.ERROR:
            # Obtener la ruta completa y número de línea
            file_info = f"{record.pathname}:{record.lineno}\n"
            # Colorear solo el mensaje de error
            colored_message = f"{color}{message}{RESET}"
            # Combinar file_info y colored_message
            message = f"{file_info}{colored_message}"
        else:
            # Para otros niveles, solo colorear el mensaje
            if color:
                message = f"{color}{message}{RESET}"

        return message

# Crear el logger
logger = logging.getLogger('mi_logger_coloreado')
logger.setLevel(logging.DEBUG)  # Establece el nivel mínimo de log

# Crear el handler para la consola
console_handler = logging.StreamHandler(sys.stdout)
console_handler.setLevel(logging.ERROR)

# Crear y asignar el formatter con colores, niveles en minúsculas y ruta completa para errores
formatter = ColoredFormatter('\t%(levelname)s: %(message)s')
console_handler.setFormatter(formatter)

# Añadir el handler al logger
logger.addHandler(console_handler)


logger.info('Logger created.')
