cmake_minimum_required(VERSION 3.10)
project(simoscript)

# C++の標準設定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ソースファイルを指定
set(SOURCES
    src/simoscript.cpp
    src/operators.cpp
    src/token.cpp
)

# 実行ファイルを生成
add_executable(simoscript ${SOURCES})