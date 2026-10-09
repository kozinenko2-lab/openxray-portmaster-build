# AirXonix native port — рабочий source snapshot r366

Это очищенное дерево исходного кода нативного порта AirXonix для Linux ARM64 / PortMaster на SDL2 и OpenGL ES 2.0.

## Изменения r348–r349

- Восстановлена структура отрисовки игровых кроуллеров: розовое 3D-тело теперь рисуется отдельным body-pass через texture slot 3, а два прохода `0x418840/0x418A90` снова используются как плоские проекции/тени на поле, как в оригинале.
- Для большого 3D-логотипа AirXonix подтверждены оригинальные формулы трансляции, масштаба и X/Y-вращения. Ошибка оказалась в GLES render state: alpha-test ошибочно вырезал чёрные texel RGB565-текстуры LOGO. Для LOGO/LAXY alpha-test теперь отключён, как в D3D7 оригинала.
- Добавлены/обновлены регрессионные контракты r348/r349; полный автономный прогон: **241/241 PASS**.

## Состояние проекта

Порт зарегистрированной/full-game ветки уже играбелен, но полный реверс-инжиниринг оригинального EXE ещё продолжается. В r349 автономный набор, не требующий внешних generated assets, проходит **241/241 PASS** с активными `assert()`; ещё 4 resource/clean-room теста регистрируются только при наличии соответствующих сгенерированных артефактов.

Строгий аудит прямых внутренних вызовов `AirXonix.wrp.exe` на этом этапе находит **692** уникальные внутренние CALL-цели; **292** из них явно упомянуты в текущем корпусе исходников/документации, оставшиеся цели ещё классифицируются (часть относится к CRT/служебному коду). `REVERSE_CLOSURE_AUDIT PASS` означает прохождение текущих обязательных reverse-контрактов, а не заявление о 100% декомпиляции EXE.

Clean-room GLES2-порт намеренно не воспроизводит устаревшие экраны регистрации/DRM, ограниченный selector незарегистрированной версии и старые Direct3D fallback-ветки, которые не нужны для GLES2. Они классифицированы в `AIRXONIX_REVERSE_ENGINEERING_GUIDE.txt`, чтобы при повторном анализе EXE их не принять за потерянную игровую механику.

## Что оставлено в финальной папке

- `src/` — исходный код игры, renderer, audio, platform и resource layer.
- `tests/` — регрессионные тесты, фиксирующие результаты реверс-инжиниринга.
- `tools/` — генератор clean-room ресурсов, извлечение ресурсов и аудит.
- `scripts/` — ARM64 PortMaster build helper и финальный reverse-closure audit.
- `cmake/` — вспомогательные CMake-файлы.
- `portmaster/AirXonix.sh` — launcher для PortMaster.
- `CMakeLists.txt` — конфигурация сборки.
- `README_RU.md`, `README_EN.md` — финальные инструкции.
- `AIRXONIX_REVERSE_ENGINEERING_GUIDE.txt` — единый документ с адресами, назначением функций/глобалов и методикой повторного разбора EXE.

Исторические Rxxx snapshot-файлы, старые отчёты сборки, asm-dump, build-кэши, сгенерированные assets и оригинальные коммерческие бинарники из финального source-архива удалены.

## Генерация clean-room ресурсов для тестов

Финальный архив содержит только исходный код, поэтому сгенерированные музыка, текстуры и resource blobs в него не включены. Чтобы повторить resource-зависимые тесты, сначала создайте их заново (нужны Python, NumPy и Pillow):

```sh
python3 tools/generate_cleanroom_resources.py assets
```

Скрипт также создаёт `AirXonix-cleanroom.zip`. Эти файлы специально не хранятся в source-архиве и полностью воспроизводимы из исходного кода.

## Финальная проверка без SDL/GLES

```sh
cmake -S . -B build-headless \
  -DAIRXONIX_BUILD_RUNTIME=OFF \
  -DAIRXONIX_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless -j2
ctest --test-dir build-headless --output-on-failure
```

Ожидаемый результат без внешних generated assets: **241/241 tests passed**.

Финальный аудит живого кода:

```sh
python3 scripts/audit_reverse_closure.py
```

## Обычная Linux-сборка

Нужны CMake 3.16+, C++17, SDL2, OpenGL ES 2.0, pkg-config и, при необходимости PNG override, libpng.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
```

Resource-free вариант с встроенными clean-room ресурсами:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DAIRXONIX_RESOURCE_FREE=ON
cmake --build build -j2
```

## PortMaster ARM64

Укажите ARM64 sysroot PortMaster и cross compiler:

```sh
export AIRXONIX_SYSROOT=/path/to/sysroot
export CC=aarch64-linux-gnu-gcc
export CXX=aarch64-linux-gnu-g++
./scripts/build_portmaster_arm64.sh
```

Скрипт принудительно использует базовую архитектуру ARMv8-A / Cortex-A53 для H700/RK3326-класса устройств.

Launcher `portmaster/AirXonix.sh` ищет игру в типичных путях muOS, ArkOS, ROCKNIX и KNULLI; логи и сохранения создаются внутри папки `AirXonix`.

## Оригинальные ресурсы

Порт способен работать без оригинальных коммерческих файлов. Для проверки максимальной fidelity resource layer также умеет читать поддерживаемые данные прямо из неизменённого `AirXonix.wrp.exe`. Сам EXE и оригинальные assets в этот архив не включены.

Если нужно повторить реверс-инжиниринг с нуля, начинайте с `AIRXONIX_REVERSE_ENGINEERING_GUIDE.txt`: там описаны формат PE, image base, основные функции, глобалы, resource offsets, renderer/audio/gameplay pipeline, особые x87/RNG детали и полный cross-reference найденных адресов.


### Итерация r350 — визуальная точность Settings/M1
- Settings: восстановлено плавное смещение выбранной строки `-0.0028 * row` со скоростью `0.00005/ms` и связанная с ним камера `Z=offset*0.22-0.003` по `0x413B67..0x413CD4`.
- Ручки трёх ползунков теперь получают исходный фиксированный поворот Z=`0x200` перед вращением по X.
- Яркость строк/ползунков/переключателя речи снова умножается на общий fade Settings (`counter>>3`).
- Фоновая сцена M1/M2 использует единый игровой `dt` кадра вместо второго измерения `SDL_GetTicks()`, устраняя рассинхронизацию анимации большого XONIX при просадках FPS.
- Регрессионный результат этого snapshot: **241/241 PASS**; `REVERSE_CLOSURE_AUDIT PASS`; `AirXonix.sh` проходит `bash -n`.

### Итерации r351–r366 — literal fidelity и состояние объектов
- Убрана старая компенсационная матрица TIME/SLOW: QUESTION/TIME/SLOW используют общую исходную ориентацию, отдельная Y-матрица остаётся только у LIFE.
- Для процедурных экструзий восстановлены исходные cap-нормали: радиальный XY-компонент `0.15`, Z=`-0.8/+1.0`, что возвращает D3D7-подобное освещение сердца/букв.
- Information page 3 переведён на точный fountain-contract: общий MSVC RNG, 128-записный пул, 16-частичный ring-buffer, исходные скорости/гравитация и отдельная половинная яркость первого крупного объекта.
- Исправлена reset/lifetime-семантика Homing/Eraser и последовательная инициализация pickup: сохранены нативные поля между level reset и история координат прошлых pickup, устранены sentinel-координаты.
- Дополнительно закреплены контракты cached world/heading летающих врагов, lifetime полевого debris и slot-логика pickup smash.
- Текущий автономный прогон: **241/241 PASS**; `REVERSE_CLOSURE_AUDIT PASS`; `portmaster/AirXonix.sh` проходит `bash -n`.
