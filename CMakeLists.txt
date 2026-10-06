cmake_minimum_required(VERSION 3.23)

project(suffering_engine)

set(core_base_path "suffering_engine_core\\")
set(include_core_path "include\\")
set(src_core_path "src\\")
set(src_app_path "suffering_engine\\")

add_subdirectory(${core_base_path})

add_subdirectory(${src_app_path})