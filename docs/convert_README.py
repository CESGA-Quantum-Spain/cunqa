import re
import sys
from pathlib import Path

# Mapeo de tipos a emoji y título
ADMONITION_MAP = {
    "NOTE": ("📘", "Note"),
    "WARNING": ("⚠️", "Warning"),
    "IMPORTANT": ("❗", "Important"),
    "TIP": ("💡", "Tip"),
    "CAUTION": ("🚧", "Caution"),
    "INFO": ("ℹ️", "Info"),
}

def convert_gfm_admonitions_to_simple(text):
    lines = text.splitlines()
    output = []
    in_admonition = False
    header = ""
    indent = "> "  # Markdown blockquote style

    for i, line in enumerate(lines):
        match = re.match(r"^>\s*\[!(\w+)\]\s*$", line.strip(), re.IGNORECASE)
        if match:
            admonition_type = match.group(1).upper()
            emoji, title = ADMONITION_MAP.get(admonition_type, ("💬", admonition_type.capitalize()))
            header = f"> {emoji} **{title}:**"
            output.append(header)
            in_admonition = True
        elif in_admonition:
            if line.strip().startswith(">"):
                content = line.strip()[1:].lstrip()
                output.append(f"> {content}")
            else:
                # End of admonition block
                output.append(line)
                in_admonition = False
        else:
            output.append(line)

    return "\n".join(output)


def convert_file(filepath):
    path = Path(filepath)
    if not path.exists():
        print(f"❌ Archivo no encontrado: {filepath}")
        return

    original = path.read_text(encoding="utf-8")
    converted = convert_gfm_admonitions_to_simple(original)
    path.write_text(converted, encoding="utf-8")
    print(f"✅ Admoniciones convertidas en: {filepath}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python convert_gfm_to_simple_admonitions.py archivo.md")
    else:
        convert_file(sys.argv[1])
