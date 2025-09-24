CC = emcc
CFLAGS = -Os --closure 1
SRC = dlx_em.cpp
OUT_JS = dlx.js
OUT_WASM = dlx.wasm

all: $(OUT_JS)

standalone: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT_WASM)

$(OUT_JS): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT_JS) -s NO_EXIT_RUNTIME=1 -s "EXPORTED_RUNTIME_METHODS=['ccall','cwrap']" -s "EXPORTED_FUNCTIONS=['_free']"

#html: $(SRC)
#	$(CC) $(CFLAGS) $(SRC_STANDALONE) --shell-file shell.html -o dlx.html -s NO_EXIT_RUNTIME=1 -s "EXPORTED_RUNTIME_METHODS=['ccall','cwrap']" -s "EXPORTED_FUNCTIONS=['_free']"

clean:
	rm -f $(OUT_JS) $(OUT_WASM)
