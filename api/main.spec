# main.spec

import os
from PyInstaller.utils.hooks import collect_submodules

project_dir = os.path.abspath('.')  # pasta /api/
templates_dir = os.path.join(project_dir, 'templates')
static_dir    = os.path.join(project_dir, 'static')
core_dir      = os.path.join(project_dir, 'core')
configs_file  = os.path.join(project_dir, 'configs.json')

datas = [
    (templates_dir, "api/templates"),
    (static_dir, "api/static"),
    (core_dir, "api/core"),
    (configs_file, ".")  # configs.json vai para a raiz do EXE
]

hiddenimports = (
    collect_submodules("flask") +
    collect_submodules("jinja2") +
    collect_submodules("werkzeug") +
    collect_submodules("itsdangerous") +
    collect_submodules("click") +
    collect_submodules("markupsafe")
)

block_cipher = None

a = Analysis(
    ['main.py'],
    pathex=[project_dir],
    datas=datas,
    hiddenimports=hiddenimports
)

pyz = PYZ(a.pure, a.zipped_data)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='monitor_realtime',
    console=True,
    debug=False  # coloque False para evitar janelas de debug
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    name='monitor_build'
)
