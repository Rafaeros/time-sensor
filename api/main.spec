# main.spec
# PyInstaller specification file for building the TCP + Flask realtime log monitor

import os
from PyInstaller.utils.hooks import collect_submodules

# Caminho base do projeto
project_dir = os.path.abspath('.')   # <-- ESSENCIAL! VOCÊ ESQUECEU ISSO

# INCLUSÃO DE TEMPLATES E ESTÁTICOS
datas = [
    (os.path.join('api', 'templates'), 'api/templates'),
    (os.path.join('api', 'static'), 'api/static'),
    (os.path.join('api', 'tmp'), 'api/tmp'),
]

# HIDDEN IMPORTS PARA FLASK E DEPENDÊNCIAS
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
    debug=True,          # VER ERROS NO TERMINAL
    bootloader_ignore_signals=False,
    strip=False,
    upx=False,
    console=True         # MOSTRA TERMINAL
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
