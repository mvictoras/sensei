set(sources conduitTest.cpp bridge.cpp)
set(libs opts thread timer util sensei sConduit sDIY)

add_executable(conduitTest ${sources})
target_link_libraries(conduitTest ${libs})
