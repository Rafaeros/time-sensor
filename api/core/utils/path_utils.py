import os
import sys
import pathlib

def resource_path(relative):
    """Retorna caminhos tanto no EXE quanto em modo normal."""
    if hasattr(sys, "_MEIPASS"):  # rodando PyInstaller
        base = pathlib.Path(sys.executable).parent
    else:
        base = pathlib.Path(__file__).parent
    return str(base / relative)


def get_logs_dir():
    """
    Retorna a pasta tmp fixa ao lado do main.py,
    mesmo no Windows (exe) ou Linux.
    """
    # Base do executável OU do script Python
    if hasattr(sys, "_MEIPASS"):
        base = pathlib.Path(sys.executable).parent
    else:
        base = pathlib.Path(__file__).parent.parent  # sobe para /api

    tmp = base / "tmp"
    tmp.mkdir(exist_ok=True)
    return str(tmp)