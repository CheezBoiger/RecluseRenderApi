#
cmake_minimum_required(VERSION 3.5)
project("RecluseRenderApiTest")

set (RECLUSE_TEST_NAME "RecluseRenderApiTest")

set(RECLUSE_TEST_FILES 
	${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/VulkanTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/CommandListTest.cpp)

# Grab the gtest system.
include(FetchContent)

# Disable GoogleTest installation rules
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.0.zip
)
# For Windows MSVC runtime compatibility
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# For Windows MSVC runtime compatibility
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(${RECLUSE_TEST_NAME} ${RECLUSE_TEST_FILES})

target_include_directories(${RECLUSE_TEST_NAME} PUBLIC ${RECLUSE_RENDER_API_INCLUDES}
	${RECLUSE_FRAMEWORK_PATH}/Include
	${CMAKE_SOURCE_DIR}/Source
	${RECLUSE_FRAMEWORK_BUILD_PATH}/Include/RecluseFramework
	${CMAKE_SOURCE_DIR}/Recluse/Build/Include/RecluseRenderApi)
	
target_link_libraries(${RECLUSE_TEST_NAME} PUBLIC 
	${RECLUSE_RENDER_API_NAME}
	${RECLUSE_VULKAN_NAME}
	${RECLUSE_FRAMEWORK_BUILD_PATH}/Lib/RecluseFramework.lib
	${RECLUSE_RENDER_API_LIBS}
	PRIVATE gmock_main)
	
# Copy the shared library after the target builds
add_custom_command(TARGET ${RECLUSE_TEST_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${RECLUSE_FRAMEWORK_BUILD_PATH}/Bin/RecluseFramework.dll"
        "$<TARGET_FILE_DIR:${RECLUSE_TEST_NAME}>"
    COMMENT "Copying shared library to executable directory..."
)

# Copy all runtime DLLs required by MyApp to its output directory after building
add_custom_command(TARGET ${RECLUSE_TEST_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${RECLUSE_TEST_NAME}>
        $<TARGET_FILE_DIR:${RECLUSE_TEST_NAME}>
    COMMAND_EXPAND_LISTS
)