CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

PROJECT(hydrogen)

SET(CMAKE_CXX_STANDARD 20)

INCLUDE_DIRECTORIES(include)

ADD_EXECUTABLE(hydro
                "src/main.cpp"
                "include/tokenizer.hpp"
                "include/parser.hpp"
                "include/generator.hpp"
)
