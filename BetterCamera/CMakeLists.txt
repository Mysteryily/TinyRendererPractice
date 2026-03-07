cmake_minimum_required(VERSION 3.10)
project(TinyRenderer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main
    main.cpp
    tgaimage.cpp
        geometry.h
        model.cpp
        model.h
)

