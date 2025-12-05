import sys
import json
from pathlib import Path

class ConfigManager:
    def __init__(self, file_path="configs.json"):

        # Se rodando como EXE → usar pasta do executável
        if getattr(sys, 'frozen', False):
            base_dir = Path(sys.executable).parent
        else:
            base_dir = Path('.').resolve()

        self.file_path = base_dir / file_path
        self.data = {}
        self.load()

    def load(self):
        if self.file_path.exists():
            with open(self.file_path, "r", encoding="utf-8") as f:
                self.data = json.load(f)
        else:
            self.data = {
                "username": "",
                "password": "",
            }
            self.save()

    def save(self):
        print("Configs saved to ", self.file_path)
        with open(self.file_path, "w", encoding="utf-8") as f:
            json.dump(self.data, f, indent=4, ensure_ascii=False)

    def get(self, key, default=None):
        return self.data.get(key, default)

    def set(self, key, value):
        self.data[key] = value
        self.save()

    def remove(self, key):
        if key in self.data:
            del self.data[key]
            self.save()


class Configs(ConfigManager):
    def __init__(self):
        super().__init__()

        self.username = self.get("username")
        self.password = self.get("password")
        self.printer = self.get("printer")
        ops = self.data.get("operators", [])
        self.data["operators"] = [
            o if isinstance(o, dict) else {"name": o} for o in ops
        ]
        self.save()

    @property
    def operators(self):
        return self.data["operators"]

    def add_operator(self, name: str):
        self.data["operators"].append({"name": name})
        self.save()

    def remove_operator(self, name: str):
        self.data["operators"] = [
            o for o in self.data["operators"] if o["name"] != name
        ]
        self.save()
