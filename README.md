# cute-slicer

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Build status](https://github.com/bullno1/cute-slicer/actions/workflows/build.yml/badge.svg)](https://github.com/bullno1/cute-slicer/actions/workflows/build.yml)

Individual tileset extractor for Cute Framework.

# Motivation

Because Cute Framework has automatic atlasing, loading an entire tileset just to use a few tiles procedurally is annoying.
Moreover, the tiles have to be addressed by coordinates.
Instead of that, it would be better to just save each tile as an individual file with descriptive names (e.g: "lava.png", "cracked_floor.png"...).
This is a tool to quickly achieve that.

This project also serves as a way to test [bgame](https://github.com/bullno1/bgame) multiplatform build pipeline.

# Building
## Linux

```
./boostrap
cmd/linux/build
```

The files should be in `bin/linux/RelWithDebInfo-reloadable`.

## Web

```
./boostrap
cmd/web/build
```

The files should be in `bin/web/RelWithDebInfo-static`.
