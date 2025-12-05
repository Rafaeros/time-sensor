# main.spec

import os
from PyInstaller.utils.hooks import collect_submodules

# -----------------------------------------------------------
# Diretório raiz do projeto (onde fica main.py)
# -----------------------------------------------------------
project_dir = os.path.abspath('.')  

# -----------------------------------------------------------
# Pastas do Flask
# -----------------------------------------------------------
templates_dir = os.path.join(project_dir, 'templates')
static_dir    = os.path.join(project_dir, 'static')

# -----------------------------------------------------------
# Arquivo de configs
# -----------------------------------------------------------
configs_file = os.path.join(project_dir, 'configs.json')

# -----------------------------------------------------------
# Datas para embutir no .exe
# -----------------------------------------------------------
datas = [
    (templates_dir, 'api/templates'),
    (static_dir, 'api/static'),
    (configs_file, '.')    # <-- adiciona o configs.json na raiz do exe
]

# -----------------------------------------------------------
# Hidden imports (necessários ao Flask/Jinja)
# -----------------------------------------------------------
hiddenimports = (
    collect_submodules("flask") +
    collect_submodules("jinja2") +
    collect_submodules("werkzeug") +
    collect_submodules("itsdangerous") +
    collect_submodules("click") +
    collect_submodules("markupsafe")
)

block_cipher = None

# -----------------------------------------------------------
# Analysis
# -----------------------------------------------------------
a = Analysis(
    ['main.py'],           # seu arquivo principal
    pathex=[project_dir],
    datas=datas,
    hiddenimports=hiddenimports
)

pyz = PYZ(a.pure, a.zipped_data)

# -----------------------------------------------------------
# EXE
# Use console=True para ver logs; pode trocar para False se quiser app silencioso
# -----------------------------------------------------------
exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='monitor_realtime',
    console=True,
    debug=False
)

# -----------------------------------------------------------
# COLLECT (monta pasta final)
# -----------------------------------------------------------
coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    name='monitor_build'
)
