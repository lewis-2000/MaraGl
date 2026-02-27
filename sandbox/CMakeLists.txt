project(Sandbox)

add_executable(sandbox
    src/main.cpp
)

target_link_libraries(sandbox PRIVATE core)