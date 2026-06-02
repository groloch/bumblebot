# bumblebot

Bumblebot is a MCTS-based chess engine, currently served through ONNX Runtime with the MIGraphX execution provider (AMD ROCm, FP16). Supports main UCI commands and is usable in GUI clients like EnCroissant and Arena.

The engine is in early development and not yet competitive.

## Build & run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cd build && ./bumblebot
```

This currently requires a compatible AMD GPU and a onnx model file to be placed in the same directory as the executable. The model file is not included in this repository, but can be found on huggingface:
https://huggingface.co/groloch/bumblebot_v0_100m.
