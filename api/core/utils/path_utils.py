import os
import sys
from pathlib import Path

def resource_path(relative: str) -> str:
    """
    Resolve um path tanto no PyInstaller (onefile) quanto em modo normal / one-folder.
    - Se extraído em tempo de execução: usa sys._MEIPASS
    - Senão se for pasta coletada (one-folder) usa pasta do executável
    - Senão usa pasta do arquivo atual (dev)
    """
    if getattr(sys, "frozen", False):
        # Preferir sys._MEIPASS (onefile) — se não existir, fallback para pasta do exe (one-folder)
        base = Path(getattr(sys, "_MEIPASS", Path(sys.executable).parent))
    else:
        base = Path(__file__).parent
    return str(base / relative)


def get_logs_dir() -> str:
    """
    Retorna a pasta tmp ao lado do script (dev) ou ao lado do exe / sys._MEIPASS (prod).
    """
    if getattr(sys, "frozen", False):
        base = Path(getattr(sys, "_MEIPASS", Path(sys.executable).parent))
    else:
        base = Path(__file__).parent  # /api

    tmp = base / "tmp"
    tmp.mkdir(exist_ok=True, parents=True)
    return str(tmp)
