import os
import sys


def resource_path(relative):
    """Retorna caminhos tanto no EXE quanto em modo normal."""
    if hasattr(sys, "_MEIPASS"):  # rodando no .exe
        base = sys._MEIPASS
    else:
        base = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(base, relative)


def get_logs_dir():
    """
    Define uma pasta persistente para armazenar logs.
    No EXE do Windows → %APPDATA%/MonitorRealtime
    No Linux / modo normal → ./tmp
    """
    # Quando é EXE no Windows (PyInstaller)
    if sys.platform == "win32" and hasattr(sys, "_MEIPASS"):
        base = os.path.join(os.getenv("APPDATA"), "MonitorRealtime")
        os.makedirs(base, exist_ok=True)
        return base

    # Rodando normal
    base = resource_path("tmp")
    os.makedirs(base, exist_ok=True)
    return base
