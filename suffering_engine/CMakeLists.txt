add_executable(sufferin_engine_app)

target_sources(sufferin_engine_app
  PRIVATE
    Launcher.cpp
)

target_link_libraries(sufferin_engine_app
  PRIVATE
    sufferin_engine_core
)

