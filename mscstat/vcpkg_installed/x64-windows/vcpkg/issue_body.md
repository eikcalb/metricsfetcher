Package: tensorflow-cc:x64-windows@2.10.0

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.36.32537.0
-    vcpkg-tool version: 2023-12-12-1c9ec1978a6b0c2b39c9e9554a96e3e275f7556e
    vcpkg-scripts version: c9919121d 2024-01-09 (3 days ago)

**To Reproduce**

`vcpkg install `
**Failure logs**

```
-- Note: tensorflow-cc only supports static library linkage. Building static library.
CMake Warning at C:/Users/Onome Israel Agwa/source/repos/mscstat/mscstat/vcpkg_installed/x64-windows/x64-windows/share/tensorflow-common/tensorflow-common.cmake:29 (message):
  Your Windows username 'Onome Israel Agwa' contains spaces.  Applying
  work-around to bazel.  Be warned of possible further issues.
Call Stack (most recent call first):
  ports/tensorflow-cc/portfile.cmake:9 (include)
  scripts/ports.cmake:170 (include)


-- Using cached msys2-bash-5.2.021-1-x86_64.pkg.tar.zst.
-- Using cached msys2-unzip-6.0-2-x86_64.pkg.tar.xz.
-- Using cached msys2-patch-2.7.6-2-x86_64.pkg.tar.zst.
-- Using cached msys2-diffutils-3.10-1-x86_64.pkg.tar.zst.
-- Using cached msys2-libintl-0.22.4-1-x86_64.pkg.tar.zst.
-- Using cached msys2-gzip-1.13-1-x86_64.pkg.tar.zst.
-- Using cached msys2-coreutils-8.32-5-x86_64.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-python-numpy-1.26.2-1-any.pkg.tar.zst.
-- Using cached msys2-file-5.45-1-x86_64.pkg.tar.zst.
-- Using cached msys2-gawk-5.3.0-1-x86_64.pkg.tar.zst.
-- Using cached msys2-grep-1~3.0-6-x86_64.pkg.tar.zst.
-- Using cached msys2-make-4.4.1-1-x86_64.pkg.tar.zst.
-- Using cached msys2-pkgconf-2.1.0-1-x86_64.pkg.tar.zst.
-- Using cached msys2-sed-4.9-1-x86_64.pkg.tar.zst.
-- Using cached msys2-msys2-runtime-3.4.9-3-x86_64.pkg.tar.zst.
-- Using cached msys2-libbz2-1.0.8-4-x86_64.pkg.tar.zst.
-- Using cached msys2-libiconv-1.17-1-x86_64.pkg.tar.zst.
-- Using cached msys2-gcc-libs-13.2.0-2-x86_64.pkg.tar.zst.
-- Using cached msys2-gmp-6.3.0-1-x86_64.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-openblas-0.3.25-1-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-python-3.11.6-2-any.pkg.tar.zst.
-- Using cached msys2-liblzma-5.4.5-1-x86_64.pkg.tar.zst.
-- Using cached msys2-libzstd-1.5.5-1-x86_64.pkg.tar.zst.
-- Using cached msys2-zlib-1.3-1-x86_64.pkg.tar.zst.
-- Using cached msys2-libreadline-8.2.007-1-x86_64.pkg.tar.zst.
-- Using cached msys2-mpfr-4.2.1-1-x86_64.pkg.tar.zst.
-- Using cached msys2-libpcre-8.45-4-x86_64.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-gcc-libgfortran-13.2.0-2-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-gcc-libs-13.2.0-2-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-libwinpthread-git-11.0.0.r404.g3a137bd87-1-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-bzip2-1.0.8-3-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-expat-2.5.0-1-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-libffi-3.4.4-1-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-mpdecimal-2.5.1-1-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-ncurses-6.4.20230708-1-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-openssl-3.2.0-1-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-sqlite3-3.44.0-1-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-tcl-8.6.12-2-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-tk-8.6.12-2-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-xz-5.4.5-1-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-zlib-1.3-1-any.pkg.tar.zst.
-- Using cached msys2-ncurses-6.4-2-x86_64.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-libsystre-1.0.1-4-any.pkg.tar.xz.
-- Using cached msys2-mingw-w64-x86_64-readline-8.2.001-6-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-gettext-0.22.4-3-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-libtre-git-r128.6fb7206-2-any.pkg.tar.xz.
-- Using cached msys2-mingw-w64-x86_64-termcap-1.3.1-7-any.pkg.tar.zst.
-- Using cached msys2-mingw-w64-x86_64-libiconv-1.17-3-any.pkg.tar.zst.
-- Using msys root at C:/code/vcpkg/downloads/tools/msys2/c19a26c7173f8e13
-- Using cached tensorflow-tensorflow-v2.10.0.tar.gz.
-- Extracting source C:/code/vcpkg/downloads/tensorflow-tensorflow-v2.10.0.tar.gz
-- Applying patch C:/Users/Onome Israel Agwa/source/repos/mscstat/mscstat/vcpkg_installed/x64-windows/x64-windows/share/tensorflow-common/fix-build-error.patch
-- Applying patch C:/Users/Onome Israel Agwa/source/repos/mscstat/mscstat/vcpkg_installed/x64-windows/x64-windows/share/tensorflow-common/change-macros-for-static-lib.patch
-- Applying patch C:/Users/Onome Israel Agwa/source/repos/mscstat/mscstat/vcpkg_installed/x64-windows/x64-windows/share/tensorflow-common/fix-windows-build.patch
-- Using source at C:/code/vcpkg/buildtrees/tensorflow-cc/src/v2.10.0-4baaf97d01.clean
-- Configuring TensorFlow (dbg)
CMake Error at scripts/cmake/vcpkg_execute_required_process.cmake:112 (message):
    Command failed: C:/code/vcpkg/downloads/tools/msys2/c19a26c7173f8e13/mingw64/bin/python3.exe C:/code/vcpkg/buildtrees/tensorflow-cc/x64-windows-dbg/configure.py --workspace C:/code/vcpkg/buildtrees/tensorflow-cc/x64-windows-dbg
    Working Directory: C:/code/vcpkg/buildtrees/tensorflow-cc/x64-windows-dbg
    Error code: 1
    See logs for more information:
      C:\code\vcpkg\buildtrees\tensorflow-cc\config-x64-windows-dbg-out.log

Call Stack (most recent call first):
  C:/Users/Onome Israel Agwa/source/repos/mscstat/mscstat/vcpkg_installed/x64-windows/x64-windows/share/tensorflow-common/tensorflow-common.cmake:187 (vcpkg_execute_required_process)
  ports/tensorflow-cc/portfile.cmake:9 (include)
  scripts/ports.cmake:170 (include)



```
<details><summary>C:\code\vcpkg\buildtrees\tensorflow-cc\config-x64-windows-dbg-out.log</summary>

```
Cannot find bazel. Please install bazel/bazelisk.
```
</details>

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    "duktape",
    "libzip",
    "openssl",
    "restbed",
    "spdlog",
    "sqlite3",
    "tensorflow-cc"
  ]
}

```
</details>
