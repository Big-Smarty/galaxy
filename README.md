# Galaxy

This is a simple project I made as a demonstration for a talk at UnFUG Furtwangen.

build:
`xmake`

run:
`./build/linux/x86_64/<debug/release>/galaxy`
xmake changes the working directory to the path of the binary, thus making it unable to find and load the compiled shader files. So it has to be run from within the root directory in this way.
