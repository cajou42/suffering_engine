add_library(sufferin_engine_core)

target_sources(sufferin_engine_core
  PRIVATE
    ${src_core_path}Application.cpp

  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS
      ${include_core_path}
    FILES
      ${include_core_path}Application.h
)