from __future__ import annotations

import sys

from PySide6.QtWidgets import QApplication

from app.main_window import MainWindow
from app.paths import DataPaths
from app.ui_styling import apply_style


def main() -> int:
    app = QApplication(sys.argv)
    apply_style(app)

    paths = DataPaths.from_runtime()
    window = MainWindow(paths)
    app.aboutToQuit.connect(window.save_state)
    window.show_initial()

    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
