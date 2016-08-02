add_executable(mabo_main mabo.cpp)
set_property(TARGET mabo_main PROPERTY OUTPUT_NAME mabo)
target_link_libraries(mabo_main mabo)
