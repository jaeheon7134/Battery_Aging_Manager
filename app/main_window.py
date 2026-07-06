from __future__ import annotations

from typing import Any

from PySide6.QtCore import QFile, QIODevice, QSettings
from PySide6.QtWidgets import QMainWindow, QPushButton, QStackedWidget, QTextEdit, QWidget
from PySide6.QtUiTools import QUiLoader

from app.pages.live_page import LivePage
from app.pages.log_analyze import LogAnalyze
from app.pages.plan_page import PlanPage
from app.paths import DataPaths
from app.ui_styling import apply_shadow


class MainWindow:
    def __init__(self, paths: DataPaths) -> None:
        self.paths = paths

        self.window = self._load_main_window(paths.ui_path)
        self.window.setFixedSize(self.window.size())

        apply_shadow(self.window)

        memo_box = self._req("Memo_box", QTextEdit)
        memo_box.setPlaceholderText("메모를 입력하세요")

        self.stacked = self._req("stackedWidget", QStackedWidget)
        self.page_log = self._req("page_3", QWidget)
        self.page_plan = self._req("page_4", QWidget)
        self.page_live = self._req("page_5", QWidget)

        self.btn_log = self._req("Log_Analyze_Btn", QPushButton)
        self.btn_schedule = self._req("Schedule_Btn", QPushButton)
        self.btn_live = self._req("Live_Btn", QPushButton)
        self.btn_memo_save = self._req("Memo_Save", QPushButton)

        self.btn_log.clicked.connect(lambda: self.stacked.setCurrentWidget(self.page_log))
        self.btn_schedule.clicked.connect(lambda: self.stacked.setCurrentWidget(self.page_plan))
        self.btn_live.clicked.connect(lambda: self.stacked.setCurrentWidget(self.page_live))

        self.plan_page = PlanPage(self.window, self.paths)
        self.plan_page.init()

        self.log_page = LogAnalyze(self.window)
        self.log_page.init()

        self.live_page = LivePage(self.window)
        self.live_page.init()

        self.btn_memo_save.clicked.connect(self.plan_page.save_memo)

        self.settings = QSettings("battery_aging", "ui_runner")

    def show_initial(self) -> None:
        self.window.show()

    def save_state(self) -> None:
        self.settings.setValue("window/geometry", self.window.saveGeometry())
        self.settings.setValue("window/state", self.window.saveState())

    def _load_main_window(self, ui_path) -> QMainWindow:
        loader = QUiLoader()
        ui_file = QFile(str(ui_path))
        if not ui_file.open(QIODevice.OpenModeFlag.ReadOnly):
            raise RuntimeError(f"UI file open failed: {ui_path}")

        try:
            widget = loader.load(ui_file)
        finally:
            ui_file.close()

        if not isinstance(widget, QMainWindow):
            raise RuntimeError("Loaded root widget is not QMainWindow")
        return widget

    def _req(self, name: str, cls: type[Any]) -> Any:
        widget = self.window.findChild(cls, name)
        if widget is None:
            raise RuntimeError(f"Required widget not found: {name}")
        return widget
