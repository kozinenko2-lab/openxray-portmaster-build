r367 ARM64 build used PortMaster-H700-SYSROOT, Clang 17 with `--target=aarch64-linux-gnu`, `--sysroot`, `-mcpu=cortex-a53`, `-fuse-ld=lld`, and AIRXONIX_RESOURCE_FREE=ON.

The sysroot contains only a text `libGLESv2.so` placeholder (`libmali.so`), not an ELF implementation. A temporary AArch64 stub with SONAME `libGLESv2.so.2` was used for **linking only** and was **not** distributed; runtime resolves real GL functions from the firmware's libGLESv2.so.2.

C++ startup used crt1.o/crti.o/crtn.o and a csu shim that calls `.init_array`, because the GCC host tools / crtbegin objects were absent from the provided sysroot. The produced binary is AArch64 ET_EXEC with no absolute RUNPATH. Check requirements with `readelf -d airxonix`. To reproduce this toolchain process, supply an AArch64 compiler, matching sysroot and real firmware GL ABI. On-device test still required.
