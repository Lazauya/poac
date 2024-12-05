# Installation

[![Packaging status](https://repology.org/badge/vertical-allrepos/poac.svg)](https://repology.org/project/poac/versions)

## Supported Operating Systems

|                                                                      Linux                                                                       |                                                                      macOS                                                                       |
| :----------------------------------------------------------------------------------------------------------------------------------------------: | :----------------------------------------------------------------------------------------------------------------------------------------------: |
| [![Linux](https://github.com/poac-dev/poac/actions/workflows/linux.yml/badge.svg)](https://github.com/poac-dev/poac/actions/workflows/linux.yml) | [![macOS](https://github.com/poac-dev/poac/actions/workflows/macos.yml/badge.svg)](https://github.com/poac-dev/poac/actions/workflows/macos.yml) |

## Homebrew (macOS & Linux)

```sh
brew install poac
```

## AUR (Arch Linux)

Poac is available in the [AUR](https://aur.archlinux.org/packages/poac).

```sh
paru -S poac
```

## Runtime Requirements

* C++ compiler
* GNU Make
* pkg-config
* mkdir
* printf
* cpplint (for `poac lint`)
* clang-format (for `poac fmt`)
* clang-tidy (for `poac tidy`)
