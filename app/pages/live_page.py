from __future__ import annotations

from PySide6.QtWidgets import QMainWindow


class LivePage:
    def __init__(self, window: QMainWindow) -> None:
        self.window = window

    def init(self) -> None:
        _ = self.window
