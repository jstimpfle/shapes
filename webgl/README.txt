The code was ported to work with emscripten / WebGL2.

Before building, you'll first need to download and set up the Emscripten SDK,
as described on https://emscripten.org/docs/getting_started/downloads.html

The SDK should include the emscripten C compiler executable, called "emcc".
Adapt the path to the emcc compiler in webgl/Makefile.

Now build the code with emscripten: Run "make -f webgl/Makefile". That should
should build the files shapes.html, shapes.js, and shapes.wasm.

For a simple test, serve the current directory over HTTP. A simple HTTP server
written in Python 3 can be found in webgl/httpserve.py. You can run it with
"python3 webgl/httpserve.py"

View the shapes.html file in a browser.
