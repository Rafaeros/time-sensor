# main.spec
# PyInstaller specification file for building the TCP + Flask realtime log monitor

import os
from PyInstaller.utils.hooks import collect_submodules, collect_data_files

# Caminhos importantes
project_dir = os.path.abspath(".")
api_dir = os.path.join(project_dir, "api")
templates_dir = os.path.join(api_dir, "templates")
static_dir = os.path.join(api_dir, "static")
tmp_dir = os.path.join(api_dir, "tmp")

# Inclui templates e arquivos estáticos
datas = []

if os.path.isdir(templates_dir):
    datas.append((templates_dir, "templates"))

if os.path.isdir(static_dir):
    datas.append((static_dir, "static"))

if os.path.isdir(tmp_dir):
    datas.append((tmp_dir, "tmp"))  # logs

# --- IMPORTANTÍSSIMO ---
# FLASK + DEPENDÊNCIAS INTERNAS
hiddenimports = (
    collect_submodules("flask") +
    collect_submodules("jinja2") +
    collect_submodules("werkzeug") +
    collect_submodules("itsdangerous") +
    collect_submodules("click") +
    collect_submodules("markupsafe")
)

# ---
# BUILD
# ---
block_cipher = None

a = Analysis(
    ['main.py'],
    pathex=[project_dir],
    binaries=[],
    datas=datas,
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='monitor_realtime',
    debug=True,          # EXIBE TODOS ERROS
    bootloader_ignore_signals=False,
    strip=False,
    upx=False,
    console=True         # MOSTRA O TERMINAL!
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    strip=False,
    upx=False,
    upx_exclude=[],
    name='monitor_build'
)
