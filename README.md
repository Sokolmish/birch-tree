# Birch-tree

This is a replacement for standard linux utility tree.
Also, it should work on Windows.

There are some differences from original tree:

- Collapses chain of directories with 1 files into single line
- Allows writing argument value directly after it's name
without space: `birch-tree -L1`

### Future plans

- Parse colors from environment variable, like `LS_COLORS`
- Support more flags from original tree
- Consider using horizontal tree branches (like in pstree output)
- Do better formatting using `COLUMNS` environment variable

## Building

There are following requirements:

- [fmtlib] (v8, but also should work with early versions)
- [CLI11] (tested with version v2.2)
  
[fmtlib]: https://fmt.dev/latest/index.html
[CLI11]: https://github.com/CLIUtils/CLI11

### Building with conan

Dependencies can be istalled using [conan] package manager.
To do this, one should have conan installed and execute following commands.

[conan]: https://conan.io

```shell
mkdir build && cd build
conan install --build=missing ..
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
cmake --build .
```

### Manual building

If all dependencies were installed manually, program can be built
via following commands.

```shell
mkdir build && cd build
cmake -DNOT_USE_CONAN ..
cmake --build .
```

## Examples

```
      birch-tree      |      original tree
                      |
 ./dir/               | ./dir/
 └─ src/main/java/    | └── src
    ├─ file.java      |     └── main
    └─ main.java      |         └── java
                      |             ├── file.java
                      |             └── main.java
```
