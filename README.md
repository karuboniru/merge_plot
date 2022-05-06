# merge plots together
## Building:
```
mkdir build
cmake -S . -B build
cmake --build build
cd build
```
check if executable merge_plot is in build directory

## Running:
 - modify input.json.example, put file paths in
 - run:
```
/path/to/build/merge_plots input.json
```
 - check outputs