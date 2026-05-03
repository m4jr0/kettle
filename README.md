# Kettle

> Brewing ideas. Literally.

*(I just thought the idea was funny)*

Kettle is a small experimental node-based scripting system built quickly in a gamejam-like fashion, so some features are still incomplete.
The approach is similar to Unreal Engine 5's Blueprints: you create logic graphs, compile them into a compact binary format, and execute them in a lightweight VM.

<p align="center">  
  <img src="docs/images/graph.json.png" alt="Example Graph">
</p>  

## Architecture

Kettle is composed of three parts:

### Kettle Forge

* Built with [litegraph.js](https://tamats.com/projects/litegraph/).
* Used to create node graphs visually.
* Exports graphs as JSON.

**Location:** [`tools/forge/index.html`](tools/forge/index.html)

For now, graphs must be exported manually by copying the JSON output at the bottom of the page into a file after clicking on the **Export JSON** button.

### Kettle Packer

* Converts JSON to KTL binary format.
* Also generates a debug string mapping file.

**Location:** [`tools/packer/pack_graph.py`](tools/packer/pack_graph.py)

Kettle Packer was implemented in Python because of time constraints.

Example:

```bash
python tools/packer/pack_graph.py examples/graph.json tmp/graph.ktl
```

Output:

```text
tmp/
  graph.ktl
  graph.ktl.strings.json
```

### Kettle (Runtime / VM)

* Executes KTL bytecode.
* Contains a compiler, a virtual machine, native bindings, and a minimal CLI tool.

**Location:** [`src/`](src/)

## Pipeline

So the full pipeline is:
**Forge** → JSON → **Packer** → KTL → **Kettle**

The Kettle CLI compiles the KTL and executes it directly.

## Build

Tested on Windows/MSVC only (for now).

### Using CMake

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Run

```bash
kettle_cli.exe tmp/graph.ktl
```

## Examples

A [minimal graph example](examples/) is available in both formats (JSON + KTL):

```text
examples/
  graph.json
  graph.ktl
  graph.ktl.strings.json
```

The `graph.ktl.strings.json` file contains a mapping of internalized strings for debugging purposes.

## Quick Test

If you want to try Kettle quickly without building:

1. Download the [latest release](https://github.com/m4jr0/kettle/releases/latest)
2. Extract it somewhere
3. Run:

```bash
kettle_cli.exe examples/graph.ktl
```

This matches the graph displayed at the beginning of this README.

## License

This project is under the [MIT license](LICENSE).