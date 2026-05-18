# Battery Aging Plan

배터리 에이징 실험 일정을 관리하고, CSV 로그 데이터를 불러와 전압, 온도, 내부저항 변화를 확인하는 Qt 기반 데스크톱 프로그램입니다.

## Current Version

- Version: `0.1`
- 기준: `Battery_Aging_Plan/CMakeLists.txt`
- 주요 기술: C++17, Qt Widgets, Qt Charts, CMake
- 현재 확인된 개발 빌드 환경: Qt 6.11.0, MinGW 64-bit

## 주요 기능

### Schedule / Plan

- 날짜별 메모, 사이클 수, 에이징 시간을 기록합니다.
- `schedule.json`에 일정을 저장하고 다시 불러옵니다.
- `config.json`의 목표 에이징 시간(`target_hours`)을 기준으로 누적 진행률을 표시합니다.
- 에이징 시간이 입력된 날짜는 달력에 색상으로 표시됩니다.

### Log Analyze

- CSV 로그 파일을 단일 파일 또는 폴더 단위로 불러옵니다.
- 선택한 CSV 파일의 데이터를 그래프로 표시합니다.
  - 배터리 전압 추이
  - 배터리 온도 변화
  - 배터리 내부저항 변화
- 온도, 사이클 수, 충전/방전 모드 조건으로 로그 데이터를 필터링할 수 있습니다.
- 그래프는 마우스 휠 확대/축소와 더블클릭 초기화를 지원합니다.

### Live

- 실시간 데이터 표시를 위한 페이지 구조가 준비되어 있습니다.
- 현재 버전에서는 실제 실시간 측정/통신 기능은 아직 구현 전입니다.

## 프로젝트 구조

```text
.
├── README.md
├── CODE_STRUCTURE.md
├── config.json
├── schedule.json
├── EXP_YYYY-MM-DD_HH-MM-SS.csv
├── logs/
└── Battery_Aging_Plan/
    ├── CMakeLists.txt
    ├── battery_aging.ui
    ├── include/
    │   ├── main_window.h
    │   ├── plan_page.h
    │   ├── log_analyze.h
    │   ├── live_page.h
    │   └── UI_Styling.h
    └── src/
        ├── main.cpp
        ├── main_window.cpp
        ├── plan_page.cpp
        ├── log_analyze.cpp
        ├── live_page.cpp
        └── UI_Styling.cpp
```

## 주요 파일 설명

- `Battery_Aging_Plan/src/main.cpp`
  - 프로그램 진입점입니다.
  - QApplication을 만들고 전역 UI 스타일을 적용한 뒤 메인 윈도우를 실행합니다.

- `Battery_Aging_Plan/src/main_window.cpp`
  - 메인 윈도우와 페이지 전환을 담당합니다.
  - `Plan_Page`, `Log_Analyze`, `Live_Page`를 초기화합니다.

- `Battery_Aging_Plan/src/plan_page.cpp`
  - 일정, 메모, 목표 시간, 누적 에이징 시간을 관리합니다.

- `Battery_Aging_Plan/src/log_analyze.cpp`
  - CSV 파일 로딩, 필터링, 그래프 표시를 담당합니다.
  - CSV 파싱 결과는 `ParsedCsvData` 구조체로 정리되어 그래프 갱신에 사용됩니다.

- `Battery_Aging_Plan/src/live_page.cpp`
  - 실시간 페이지의 기본 구조가 있습니다.

- `Battery_Aging_Plan/src/UI_Styling.cpp`
  - Qt 스타일시트와 그림자 효과를 적용합니다.

## 데이터 파일

### config.json

프로그램 설정값을 저장합니다.

```json
{
  "goal": {
    "target_hours": 1500
  },
  "path": {
    "log_dir": "./logs"
  }
}
```

### schedule.json

날짜별 일정 정보를 저장합니다.

```json
{
  "2026-04-16": {
    "aging_hours": 672,
    "cycle": 4,
    "note": "memo"
  }
}
```

### EXP_YYYY-MM-DD_HH-MM-SS.csv

실험 장비에서 생성되는 로그 파일입니다. Log Analyze 페이지에서 CSV를 불러와 전압, 온도, 내부저항 데이터를 그래프로 확인합니다.

## 빌드 방법

Qt와 CMake가 설치되어 있어야 합니다.

```powershell
cd Battery_Aging_Plan
cmake -S . -B build
cmake --build build
```

Qt Creator를 사용하는 경우 `Battery_Aging_Plan/CMakeLists.txt`를 열어 빌드할 수 있습니다.

현재 작업 폴더에는 Qt Creator에서 생성한 Debug/Release 빌드 폴더가 포함되어 있습니다.

```text
Battery_Aging_Plan/build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug/
Battery_Aging_Plan/build/Desktop_Qt_6_11_0_MinGW_64_bit-Release/
```

## 실행 방법

빌드 후 생성된 실행 파일을 실행합니다.

```powershell
Battery_Aging_Plan\build\Desktop_Qt_6_11_0_MinGW_64_bit-Debug\Battery_Aging_Plan.exe
```

또는 Release 빌드를 사용할 수 있습니다.

```powershell
Battery_Aging_Plan\build\Desktop_Qt_6_11_0_MinGW_64_bit-Release\Battery_Aging_Plan.exe
```

## 개발 메모

- 현재 프로젝트 버전은 `0.1`입니다.
- `Log_Analyze` 클래스명은 기존 오타였던 `Log_Analyize`에서 정리되었습니다.
- `Live_Page`는 구조만 준비된 상태라 추후 실시간 장비 통신 기능을 추가할 수 있습니다.
- CSV 파싱은 현재 쉼표 기준의 단순 파싱을 사용합니다. CSV 값 안에 쉼표가 포함되는 형식이 필요해지면 전용 CSV 파서로 교체하는 것이 좋습니다.
- `CODE_STRUCTURE.md`에는 파일별 역할 설명이 정리되어 있습니다.

## 최근 확인

- Debug 빌드 성공 확인
- 확인일: 2026-05-18
