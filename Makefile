EVALFILE ?= model.onnx
EXE      ?= bumblebot
BUILD    ?= build

.PHONY: all clean

all:
	cmake -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=Release -DEVALFILE=$(EVALFILE)
	cmake --build $(BUILD) -j
	cp $(BUILD)/bumblebot $(EXE)

clean:
	cmake --build $(BUILD) --target clean || true
	rm -f $(EXE)
