from __future__ import annotations

import locale
import math
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from PySide6.QtCore import QObject, QSignalBlocker, Qt
from PySide6.QtGui import QFont, QMouseEvent, QPainter, QWheelEvent
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QFileDialog,
    QGraphicsView,
    QGroupBox,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QPushButton,
    QSlider,
    QVBoxLayout,
    QWidget,
)
from PySide6.QtCharts import (
    QAbstractBarSeries,
    QAbstractSeries,
    QBarCategoryAxis,
    QBarSet,
    QChart,
    QChartView,
    QLineSeries,
    QStackedBarSeries,
    QValueAxis,
)


@dataclass
class FileFilterMeta:
    parsed: bool = False
    has_first_temp: bool = False
    first_temp: float = 0.0
    has_repeat_column: bool = False
    repeat_times: set[int] = field(default_factory=set)


@dataclass
class InterResData:
    valid: bool = False
    value: float = 0.0


@dataclass
class ParsedCsvData:
    voltage_x: list[float] = field(default_factory=list)
    voltage_y: list[float] = field(default_factory=list)
    temp_x: list[float] = field(default_factory=list)
    temp_y: list[float] = field(default_factory=list)
    inter_res: InterResData = field(default_factory=InterResData)


class ChartViewEx(QChartView):
    def wheelEvent(self, event: QWheelEvent) -> None:  # noqa: N802
        if event.angleDelta().y() > 0:
            self.chart().zoom(1.2)
        else:
            self.chart().zoom(0.8)
        event.accept()

    def mouseDoubleClickEvent(self, event: QMouseEvent) -> None:  # noqa: N802
        self.chart().zoomReset()
        super().mouseDoubleClickEvent(event)


class LogAnalyze(QObject):
    def __init__(self, window: QMainWindow) -> None:
        super().__init__(window)
        self.window = window

        self.btn_csv = self._req("CSV_Load_Btn", QPushButton)
        self.btn_csv_folder = self._req("CSV_Folder_Load_Btn", QPushButton)
        self.btn_clear = self._req("Graph_Clear_Btn", QPushButton)
        self.file_list = self._req("File_List", QListWidget)

        self.cond_group = self._req("groupBox_Log_Condition", QGroupBox)
        self.cond_onoff = self._req("CBox_Condition_ONOFF", QCheckBox)
        self.temp_slider = self._req("Temp_Slider", QSlider)
        self.temp_label = self._req("label_7", QLabel)
        self.cycle_combo = self._req("CbBox_Cycle_Count", QComboBox)

        self.cbox_charge = self._req("CBox_Charge", QCheckBox)
        self.cbox_discharge = self._req("CBox_Discharge", QCheckBox)
        self.cbox_all = self._req("CBox_All", QCheckBox)

        self.log_graph_container = self._req("Log_Graph", QWidget)
        self.temp_graph_container = self._req("Temp_Graph", QWidget)
        self.inter_graph_container = self._req("InterRes_Graph", QWidget)

        self.selected_files: set[str] = set()
        self.file_map: dict[str, str] = {}
        self.filter_meta_map: dict[str, FileFilterMeta] = {}

        self.chart: QChart | None = None
        self.view: QChartView | None = None
        self.temp_chart: QChart | None = None
        self.temp_view: QChartView | None = None
        self.inter_res_chart: QChart | None = None
        self.inter_res_view: QChartView | None = None

    def init(self) -> None:
        self.btn_csv.clicked.connect(self.open_csv)
        self.btn_csv_folder.clicked.connect(self.open_csv_folder)
        self.btn_clear.clicked.connect(self.clear_graph)

        self.file_list.itemClicked.connect(self.toggle_item_check)
        self.file_list.itemChanged.connect(self.on_item_changed)

        self.cond_group.setEnabled(self.cond_onoff.isChecked())
        self.cond_onoff.toggled.connect(self.on_condition_toggled)

        self.temp_label.setText(f"{self.temp_slider.value()} °C")
        self.temp_slider.valueChanged.connect(self.on_temp_changed)
        self.temp_slider.sliderReleased.connect(self.on_temp_released)

        self.cycle_combo.currentIndexChanged.connect(self.on_cycle_changed)

        if not self.cbox_charge.isChecked() and not self.cbox_discharge.isChecked() and not self.cbox_all.isChecked():
            self.cbox_all.setChecked(True)

        self.cbox_charge.toggled.connect(self.on_mode_changed)
        self.cbox_discharge.toggled.connect(self.on_mode_changed)
        self.cbox_all.toggled.connect(self.on_mode_changed)

        self.update_graph()

    def toggle_item_check(self, item: QListWidgetItem) -> None:
        item.setCheckState(Qt.Unchecked if item.checkState() == Qt.Checked else Qt.Checked)

    def on_item_changed(self, item: QListWidgetItem) -> None:
        name = item.text()
        if item.checkState() == Qt.Checked:
            self.selected_files.add(name)
        else:
            self.selected_files.discard(name)
        self.update_graph()

    def on_condition_toggled(self, checked: bool) -> None:
        self.cond_group.setEnabled(checked)
        self.update_list_filter()
        self.update_graph()

    def on_temp_changed(self, value: int) -> None:
        self.temp_label.setText(f"{value} °C")

    def on_temp_released(self) -> None:
        self.update_list_filter()
        self.update_graph()

    def on_cycle_changed(self) -> None:
        self.update_list_filter()
        self.update_graph()

    def on_mode_changed(self) -> None:
        b1 = QSignalBlocker(self.cbox_charge)
        b2 = QSignalBlocker(self.cbox_discharge)
        b3 = QSignalBlocker(self.cbox_all)
        _ = (b1, b2, b3)

        changed = self.sender()
        if changed is self.cbox_all and self.cbox_all.isChecked():
            self.cbox_charge.setChecked(False)
            self.cbox_discharge.setChecked(False)
        elif changed is self.cbox_charge and self.cbox_charge.isChecked():
            self.cbox_discharge.setChecked(False)
            self.cbox_all.setChecked(False)
        elif changed is self.cbox_discharge and self.cbox_discharge.isChecked():
            self.cbox_charge.setChecked(False)
            self.cbox_all.setChecked(False)

        if not self.cbox_charge.isChecked() and not self.cbox_discharge.isChecked() and not self.cbox_all.isChecked():
            self.cbox_all.setChecked(True)

        self.update_list_filter()
        self.update_graph()

    def open_csv(self) -> None:
        path, _ = QFileDialog.getOpenFileName(
            self.window,
            "CSV 선택",
            "",
            "CSV Files (*.csv)",
        )
        if not path:
            return

        self.add_file_to_list(path)
        self.update_list_filter()
        self.update_graph()

    def open_csv_folder(self) -> None:
        folder_path = QFileDialog.getExistingDirectory(self.window, "CSV 폴더 선택", "")
        if not folder_path:
            return

        root = Path(folder_path)
        for csv_file in root.rglob("*.csv"):
            self.add_file_to_list(str(csv_file))

        self.update_list_filter()
        self.update_graph()

    def update_list_filter(self) -> None:
        condition_enabled = self.cond_onoff.isChecked()
        target_temp = float(self.temp_slider.value())
        try:
            target_cycle = int(self.cycle_combo.currentText())
        except ValueError:
            target_cycle = 0

        blocker = QSignalBlocker(self.file_list)
        _ = blocker

        for i in range(self.file_list.count()):
            item = self.file_list.item(i)
            path = self.file_map.get(item.text(), "")

            if not condition_enabled:
                item.setHidden(False)
                continue

            meta = self.get_filter_meta(path)
            temp_match = (not meta.has_first_temp) or abs(meta.first_temp - target_temp) < 1.0
            cycle_match = meta.has_repeat_column and (target_cycle in meta.repeat_times)
            match = temp_match and cycle_match
            item.setHidden(not match)

            if not match and item.checkState() == Qt.Checked:
                item.setCheckState(Qt.Unchecked)
                self.selected_files.discard(item.text())

    def get_filter_meta(self, file_path: str) -> FileFilterMeta:
        meta = self.filter_meta_map.get(file_path)
        if meta is None:
            meta = FileFilterMeta()
            self.filter_meta_map[file_path] = meta

        if meta.parsed:
            return meta

        meta.parsed = True
        meta.has_first_temp = False
        meta.first_temp = 0.0
        meta.has_repeat_column = False
        meta.repeat_times.clear()

        if not file_path:
            return meta

        lines = self._read_lines(file_path)
        if len(lines) < 3:
            return meta

        headers = [self._normalize_header(h) for h in lines[2].split(",")]
        temp_idx = -1
        repeat_idx = -1

        for i, header in enumerate(headers):
            if "battery" in header and "temp" in header:
                temp_idx = i
            if "repeat" in header and "time" in header:
                repeat_idx = i

        meta.has_repeat_column = repeat_idx >= 0

        for line in lines[3:]:
            line = line.strip()
            if not line:
                continue

            parts = line.split(",")

            if temp_idx >= 0 and not meta.has_first_temp:
                temp_raw = self._cell(parts, temp_idx).replace("C", "").replace("c", "").replace("°", "")
                try:
                    meta.first_temp = float(temp_raw)
                    meta.has_first_temp = True
                except ValueError:
                    pass

            if repeat_idx >= 0:
                repeat_raw = self._cell(parts, repeat_idx)
                try:
                    meta.repeat_times.add(int(round(float(repeat_raw))))
                except ValueError:
                    pass

        return meta

    def matches_selected_mode(self, mode_value: str) -> bool:
        if not self.cond_onoff.isChecked():
            return True

        charge_checked = self.cbox_charge.isChecked()
        discharge_checked = self.cbox_discharge.isChecked()
        all_checked = self.cbox_all.isChecked()

        if all_checked or charge_checked == discharge_checked:
            return True

        normalized = mode_value.strip()
        if charge_checked:
            return "충전" in normalized
        if discharge_checked:
            return "방전" in normalized
        return True

    def add_file_to_list(self, path: str) -> None:
        name = Path(path).name
        if name in self.file_map:
            return

        self.file_map[name] = path

        item = QListWidgetItem(name)
        item.setFlags(item.flags() | Qt.ItemIsUserCheckable)
        item.setCheckState(Qt.Unchecked)
        self.file_list.addItem(item)

    def clear_graph(self) -> None:
        if self.chart is not None:
            self.chart.removeAllSeries()
        if self.temp_chart is not None:
            self.temp_chart.removeAllSeries()
        if self.inter_res_chart is not None:
            self.inter_res_chart.removeAllSeries()

        self.selected_files.clear()
        self.file_map.clear()
        self.filter_meta_map.clear()

        for i in range(self.file_list.count()):
            item = self.file_list.item(i)
            item.setCheckState(Qt.Unchecked)

        self.file_list.clear()

    def load_csv(self, file_path: str) -> ParsedCsvData:
        data = ParsedCsvData()
        lines = self._read_lines(file_path)
        if len(lines) < 3:
            return data

        headers = [self._normalize_header(h) for h in lines[2].split(",")]

        volt_idx = -1
        temp_idx = -1
        inter_res_idx = -1
        mode_idx = -1

        for i, header in enumerate(headers):
            if "total" in header and "volt" in header:
                volt_idx = i
            if "battery" in header and "temp" in header:
                temp_idx = i
            if "inter" in header and "res" in header:
                inter_res_idx = i
            if "mode" in header:
                mode_idx = i

        if volt_idx < 0 and temp_idx < 0 and inter_res_idx < 0:
            return data

        row = 0
        for line in lines[3:]:
            parts = line.split(",")

            if mode_idx >= 0:
                mode_raw = self._cell(parts, mode_idx)
                if not self.matches_selected_mode(mode_raw):
                    row += 1
                    continue

            if volt_idx >= 0:
                volt_raw = self._cell(parts, volt_idx).replace("V", "").replace("v", "")
                try:
                    volt_value = float(volt_raw)
                    data.voltage_x.append(float(row))
                    data.voltage_y.append(volt_value)
                except ValueError:
                    pass

            if temp_idx >= 0:
                temp_raw = self._cell(parts, temp_idx).replace("C", "").replace("c", "").replace("°", "")
                try:
                    temp_value = float(temp_raw)
                    data.temp_x.append(float(row))
                    data.temp_y.append(temp_value)
                except ValueError:
                    pass

            if inter_res_idx >= 0 and not data.inter_res.valid:
                inter_raw = (
                    self._cell(parts, inter_res_idx)
                    .replace("ohm", "")
                    .replace("OHM", "")
                    .replace("Ω", "")
                )
                try:
                    inter_val = float(inter_raw)
                    if inter_val != 0.0:
                        data.inter_res.valid = True
                        data.inter_res.value = inter_val * 1000.0
                except ValueError:
                    pass

            row += 1

        return data

    def update_graph(self) -> None:
        self.init_charts()
        self.reset_charts()
        self.process_selected_files()

    def init_charts(self) -> None:
        if self.chart is None:
            self.chart, self.view = self.create_chart("배터리 전압 추이", self.log_graph_container)

        if self.temp_chart is None:
            self.temp_chart, self.temp_view = self.create_chart("배터리 온도 변화", self.temp_graph_container)

        if self.inter_res_chart is None:
            chart = QChart()
            chart.setTitle("배터리 내부저항 변화")
            title_font = QFont()
            title_font.setPointSize(15)
            title_font.setBold(True)
            chart.setTitleFont(title_font)
            chart.legend().setAlignment(Qt.AlignBottom)

            axis_x = QValueAxis()
            axis_y = QValueAxis()
            axis_x.setRange(0, 1)
            axis_y.setRange(0, 1)
            chart.addAxis(axis_x, Qt.AlignBottom)
            chart.addAxis(axis_y, Qt.AlignLeft)

            view = ChartViewEx(chart)
            view.setRenderHint(QPainter.Antialiasing)
            view.setRubberBand(QChartView.RectangleRubberBand)
            view.setDragMode(QGraphicsView.ScrollHandDrag)

            layout = self.inter_graph_container.layout()
            if layout is None:
                layout = QVBoxLayout(self.inter_graph_container)
                layout.setContentsMargins(0, 0, 0, 0)
            layout.addWidget(view)

            self.inter_res_chart = chart
            self.inter_res_view = view

    def create_chart(self, title: str, parent: QWidget) -> tuple[QChart, QChartView]:
        chart = QChart()
        chart.setTitle(title)

        title_font = QFont()
        title_font.setPointSize(15)
        title_font.setBold(True)
        chart.setTitleFont(title_font)
        chart.legend().setAlignment(Qt.AlignBottom)

        axis_x = QValueAxis()
        axis_y = QValueAxis()
        chart.addAxis(axis_x, Qt.AlignBottom)
        chart.addAxis(axis_y, Qt.AlignLeft)

        view = ChartViewEx(chart)
        view.setRenderHint(QPainter.Antialiasing)
        view.setRubberBand(QChartView.RectangleRubberBand)
        view.setDragMode(QGraphicsView.ScrollHandDrag)

        layout = parent.layout()
        if layout is None:
            layout = QVBoxLayout(parent)
            layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(view)

        return chart, view

    def reset_charts(self) -> None:
        if self.chart is None or self.temp_chart is None or self.inter_res_chart is None:
            return

        self.chart.removeAllSeries()
        self.temp_chart.removeAllSeries()
        self.inter_res_chart.removeAllSeries()

        for axis in list(self.inter_res_chart.axes()):
            self.inter_res_chart.removeAxis(axis)

    def process_selected_files(self) -> None:
        if self.chart is None or self.temp_chart is None or self.inter_res_chart is None:
            return

        bar_series = QStackedBarSeries()
        inter_res_names: list[str] = []
        inter_res_values: list[float] = []

        all_voltage_x: list[float] = []
        all_voltage_y: list[float] = []
        all_temp_x: list[float] = []
        all_temp_y: list[float] = []

        ordered_selected: list[str] = []
        for i in range(self.file_list.count()):
            item = self.file_list.item(i)
            if item.text() in self.selected_files:
                ordered_selected.append(item.text())

        has_inter_res_data = False

        for name in ordered_selected:
            path = self.file_map[name]
            data = self.load_csv(path)

            all_voltage_x.extend(data.voltage_x)
            all_voltage_y.extend(data.voltage_y)
            all_temp_x.extend(data.temp_x)
            all_temp_y.extend(data.temp_y)

            self.add_voltage_series(name, data.voltage_x, data.voltage_y)
            self.add_temp_series(name, data.temp_x, data.temp_y)

            if data.inter_res.valid:
                inter_res_names.append(name)
                inter_res_values.append(data.inter_res.value)
                has_inter_res_data = True

        if has_inter_res_data:
            categories = inter_res_names.copy()
            n = len(inter_res_values)
            for i in range(n):
                bar_set = QBarSet(inter_res_names[i])
                for j in range(n):
                    bar_set.append(inter_res_values[i] if i == j else 0.0)
                bar_series.append(bar_set)
            self.draw_inter_res_bar(bar_series, categories, True)
        else:
            self.draw_inter_res_bar(None, [], False)

        self.update_axes(all_voltage_x, all_voltage_y, all_temp_x, all_temp_y)

    def draw_inter_res_bar(
        self,
        series: QAbstractBarSeries | None,
        categories: list[str],
        has_data: bool,
    ) -> None:
        if self.inter_res_chart is None:
            return

        for axis in list(self.inter_res_chart.axes()):
            self.inter_res_chart.removeAxis(axis)

        if series is not None:
            series.setBarWidth(0.5)
            self.inter_res_chart.addSeries(series)

        axis_x = QBarCategoryAxis()
        if categories:
            axis_x.append(categories)

        axis_y = QValueAxis()
        axis_y.setTitleText("mΩ")
        axis_y.setLabelFormat("%.2f")

        if has_data and series is not None:
            values: list[float] = []
            for bar_set in series.barSets():
                for i in range(bar_set.count()):
                    value = bar_set.at(i)
                    if value > 0.0:
                        values.append(value)

            if not values:
                axis_y.setRange(0.0, 1.0)
            else:
                min_y = min(values)
                max_y = max(values)
                pad = (max_y - min_y) * 0.05
                if pad <= 0.0:
                    pad = max_y * 0.05 if max_y > 0.0 else 0.1
                axis_y.setRange(min_y - pad, max_y + pad)
        else:
            axis_y.setRange(0.0, 1.0)

        self.inter_res_chart.addAxis(axis_x, Qt.AlignBottom)
        self.inter_res_chart.addAxis(axis_y, Qt.AlignLeft)

        if series is not None:
            series.attachAxis(axis_x)
            series.attachAxis(axis_y)

    def add_voltage_series(self, name: str, x: list[float], y: list[float]) -> None:
        if self.chart is None:
            return
        series = QLineSeries()
        series.setName(name)
        for i, xv in enumerate(x):
            if i < len(y):
                series.append(xv, y[i])
        self.attach_series(self.chart, series)

    def add_temp_series(self, name: str, x: list[float], y: list[float]) -> None:
        if self.temp_chart is None:
            return
        series = QLineSeries()
        series.setName(name)
        for i, xv in enumerate(x):
            if i < len(y):
                series.append(xv, y[i])
        self.attach_series(self.temp_chart, series)

    def attach_series(self, chart: QChart, series: QAbstractSeries) -> None:
        chart.addSeries(series)
        for axis in chart.axes(Qt.Horizontal):
            series.attachAxis(axis)
        for axis in chart.axes(Qt.Vertical):
            series.attachAxis(axis)

    def update_axes(
        self,
        all_voltage_x: list[float],
        all_voltage_y: list[float],
        all_temp_x: list[float],
        all_temp_y: list[float],
    ) -> None:
        if self.chart is not None:
            self._set_axis(self.chart, all_voltage_x, all_voltage_y)
        if self.temp_chart is not None:
            self._set_axis(self.temp_chart, all_temp_x, all_temp_y)

    def _set_axis(self, chart: QChart, x: list[float], y: list[float]) -> None:
        axis_x = chart.axes(Qt.Horizontal)
        axis_y = chart.axes(Qt.Vertical)

        x_axis = axis_x[0] if axis_x else None
        y_axis = axis_y[0] if axis_y else None

        if not isinstance(x_axis, QValueAxis) or not isinstance(y_axis, QValueAxis) or not x or not y:
            if isinstance(x_axis, QValueAxis):
                x_axis.setRange(0.0, 1.0)
            if isinstance(y_axis, QValueAxis):
                y_axis.setRange(0.0, 1.0)
            return

        min_x = min(x)
        max_x = max(x)
        min_y = min(y)
        max_y = max(y)

        x_pad = (max_x - min_x) * 0.02
        y_pad = (max_y - min_y) * 0.08

        if math.isclose(x_pad, 0.0):
            x_pad = 1.0
        if math.isclose(y_pad, 0.0):
            y_pad = 0.5

        x_axis.setRange(max(0.0, min_x - x_pad), max_x + x_pad)
        y_axis.setRange(max(0.0, min_y - y_pad), max_y + y_pad)

    def _read_lines(self, file_path: str) -> list[str]:
        enc = locale.getpreferredencoding(False)
        try:
            with open(file_path, "r", encoding=enc, errors="ignore") as f:
                return f.read().splitlines()
        except OSError:
            return []

    @staticmethod
    def _normalize_header(value: str) -> str:
        return value.replace('"', "").strip().lower()

    @staticmethod
    def _cell(parts: list[str], idx: int) -> str:
        if 0 <= idx < len(parts):
            return parts[idx].strip()
        return ""

    def _req(self, name: str, cls: type[Any]) -> Any:
        widget = self.window.findChild(cls, name)
        if widget is None:
            raise RuntimeError(f"Required widget not found: {name}")
        return widget
