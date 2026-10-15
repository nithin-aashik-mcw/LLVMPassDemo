add_executable(mat-mul mat-mul.cpp)

target_compile_options(mat-mul PRIVATE -O3 -march=native -fopenmp)
target_link_options(mat-mul PRIVATE -fopenmp)
