# main.spec
# PyInstaller specification file for building the TCP + Flask realtime log monitor

import os
from PyInstaller.utils.hooks import collect_submodules
from PyInstaller.utils.hooks import collect_data_files

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
    datas.append((tmp_dir, "tmp"))  # opcional

# Coleta módulos do Flask para evitar erro de import
hiddenimports = collect_submodules("flask")

# ---
# BUILD
# ---
block_cipher = None

a = Analysis(
    ['main.py'],      # seu arquivo principal
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
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=True     # MOSTRA O TERMINAL !!!
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='monitor_build'
)
