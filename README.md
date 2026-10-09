# AirXonix r367 — H700 / Android

Это отдельная ветка проекта AirXonix, не меняющая основную ветку OpenXRay.

**Исходники:** [AirXonix-H700-Android-r367-public-source.zip](AirXonix-H700-Android-r367-public-source.zip) — содержит полный C++ порт H700/PortMaster, исходники Android ARM64 (SDL2/OpenGL ES2), Gradle/CMake, настройки и тесты.

**Ресурсы:** оригинальные коммерческие EXE и MUSIC не опубликованы. Workflow `Build AirXonix Android` собирает APK с синтезированными clean-room ресурсами. Для сборки со своими оригинальными данными распакуйте архив, запустите `python3 scripts/bundle_personal_assets.py /path/to/game` и соберите локально через Android SDK.

**Статус:** Android APK и запуск на устройстве требуют проверки результатов CI и аппаратного тестирования. Нельзя считать сборку успешной до завершения GitHub Actions.

Оригинальный H700 лаунчер: `native/portmaster/AirXonix.sh` внутри архива.
