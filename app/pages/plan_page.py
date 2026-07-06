from __future__ import annotations

import json
from typing import Any

from PySide6.QtCore import QDate
from PySide6.QtGui import QColor, QTextCharFormat
from PySide6.QtWidgets import QCalendarWidget, QLabel, QLineEdit, QMainWindow, QProgressBar, QTextEdit

from app.paths import DataPaths


class PlanPage:
    def __init__(self, window: QMainWindow, paths: DataPaths) -> None:
        self.window = window
        self.paths = paths

        self.calendar = self._req("calendarWidget", QCalendarWidget)
        self.memo_box = self._req("Memo_box", QTextEdit)
        self.cycle_time = self._req("Cycle_time", QLineEdit)
        self.aging_time = self._req("Aging_time", QLineEdit)
        self.goal_time = self._req("goal_time", QLabel)
        self.current_time = self._req("Current_time", QLabel)
        self.progress = self._req("progressBar", QProgressBar)

        self.note_map: dict[str, dict[str, Any]] = {}
        self.selected_date = QDate()
        self.target_hours = 0

    def init(self) -> None:
        self.calendar.clicked.connect(self.on_date_selected)
        self.load_from_json()
        self.update_calendar_marks()
        self.load_config()
        self.update_current_time()

    def save_memo(self) -> None:
        if not self.selected_date.isValid():
            return

        key = self.selected_date.toString("yyyy-MM-dd")
        self.note_map[key] = {
            "note": self.memo_box.toPlainText(),
            "cycle": int(self.cycle_time.text() or 0),
            "aging_hours": int(self.aging_time.text() or 0),
        }

        self.save_to_json()
        self.update_current_time()
        self.update_calendar_marks()

    def on_date_selected(self, date: QDate) -> None:
        self.selected_date = date
        key = date.toString("yyyy-MM-dd")

        obj = self.note_map.get(key)
        if obj is None:
            self.memo_box.clear()
            self.cycle_time.clear()
            self.aging_time.clear()
            return

        self.memo_box.setPlainText(str(obj.get("note", "")))
        self.cycle_time.setText(str(int(obj.get("cycle", 0))))
        self.aging_time.setText(str(int(obj.get("aging_hours", 0))))

    def update_calendar_marks(self) -> None:
        self.calendar.setDateTextFormat(QDate(), QTextCharFormat())

        for key, obj in self.note_map.items():
            date = QDate.fromString(key, "yyyy-MM-dd")
            if not date.isValid() or "aging_hours" not in obj:
                continue

            aging = int(obj.get("aging_hours", 0))
            if aging == 0:
                continue

            fmt = QTextCharFormat()
            if aging < 8:
                fmt.setBackground(QColor(255, 100, 100, 60))
            else:
                fmt.setBackground(QColor(100, 180, 255, 50))
            self.calendar.setDateTextFormat(date, fmt)

    def save_to_json(self) -> None:
        self.paths.schedule_path.parent.mkdir(parents=True, exist_ok=True)
        with self.paths.schedule_path.open("w", encoding="utf-8") as f:
            json.dump(self.note_map, f, ensure_ascii=False, indent=2)

    def load_from_json(self) -> None:
        if not self.paths.schedule_path.exists():
            return

        try:
            with self.paths.schedule_path.open("r", encoding="utf-8") as f:
                loaded = json.load(f)
            if isinstance(loaded, dict):
                self.note_map = {str(k): dict(v) for k, v in loaded.items() if isinstance(v, dict)}
        except (OSError, json.JSONDecodeError):
            self.note_map = {}

    def load_config(self) -> None:
        if not self.paths.config_path.exists():
            return

        try:
            with self.paths.config_path.open("r", encoding="utf-8") as f:
                obj = json.load(f)
            self.target_hours = int(obj.get("goal", {}).get("target_hours", 0))
            self.goal_time.setText(f"{self.target_hours} h")
        except (OSError, json.JSONDecodeError, ValueError, TypeError):
            self.target_hours = 0
            self.goal_time.setText("0 h")

    def calculate_total_hours(self) -> int:
        total = 0
        for obj in self.note_map.values():
            try:
                total += int(obj.get("aging_hours", 0))
            except (ValueError, TypeError):
                pass
        return total

    def update_current_time(self) -> None:
        total = self.calculate_total_hours()
        self.current_time.setText(f"{total} h")

        if self.target_hours > 0:
            progress = (total * 100) // self.target_hours
            self.progress.setValue(progress)
        else:
            self.progress.setValue(0)

    def _req(self, name: str, cls: type[Any]) -> Any:
        widget = self.window.findChild(cls, name)
        if widget is None:
            raise RuntimeError(f"Required widget not found: {name}")
        return widget
