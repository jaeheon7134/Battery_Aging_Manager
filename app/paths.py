from __future__ import annotations

import os
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class DataPaths:
    base_dir: Path
    ui_path: Path
    config_path: Path
    schedule_path: Path

    @classmethod
    def from_runtime(cls) -> "DataPaths":
        env_base = os.getenv("BATTERY_AGING_DATA_DIR", "").strip()
        if env_base:
            base_dir = Path(env_base).expanduser().resolve()
        elif getattr(sys, "frozen", False):
            base_dir = Path(sys.executable).resolve().parent
        else:
            base_dir = Path(__file__).resolve().parents[1]

        nested_base = base_dir / "battery_plan_qt" / "Battery_Aging_Plan"

        config_path = base_dir / "config.json"
        if not config_path.exists():
            fallback_config = nested_base / "config.json"
            if fallback_config.exists():
                config_path = fallback_config

        schedule_path = base_dir / "schedule.json"
        if not schedule_path.exists():
            fallback_schedule = nested_base / "schedule.json"
            if fallback_schedule.exists():
                schedule_path = fallback_schedule

        return cls(
            base_dir=base_dir,
            ui_path=base_dir / "battery_aging.ui",
            config_path=config_path,
            schedule_path=schedule_path,
        )
