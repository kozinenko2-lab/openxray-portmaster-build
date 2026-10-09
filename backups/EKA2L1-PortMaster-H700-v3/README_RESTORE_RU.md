# EKA2L1 PortMaster H700 v3 — резервная копия

Это постоянная резервная копия исходников порта EKA2L1 для ARM64/H700.

Основа upstream:
- repository: EKA2L1/EKA2L1
- commit: 11f5b6cd141c0884a130ee45743f1f5e17bb1783

Проверенная H700 сборка:
- source/CI branch: eka2l1-h700-v3
- successful build run: 37865300058
- successful package run: 37867681647
- target: AArch64 / H700 / SDL2 Mali fbdev / native OpenGL ES 3
- maximum required glibc: GLIBC_2.35

## Восстановление

Вариант 1 — готовый снимок:
1. Распаковать EKA2L1-PortMaster-H700-v3-source.tar.gz.
2. Перейти в source-snapshot.
3. Запустить scripts/fetch_portmaster_submodules.sh для восстановления точных third-party submodules.
4. Использовать build-h700.sh/CI как ориентир для сборки.

Вариант 2 — восстановление с upstream:
1. checkout upstream commit 11f5b6cd141c0884a130ee45743f1f5e17bb1783.
2. Применить EKA2L1-PortMaster-H700-v3.patch через patch -p1.
3. Восстановить submodules по PORTMASTER_SUBMODULES.lock.

Архив специально не содержит содержимое third-party git submodules, чтобы
резерв оставался компактным. Их точные gitlink SHA зафиксированы в lock-файле.
