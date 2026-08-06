#
cmake_minimum_required(VERISON 3.5)
project("RecluseRenderApiTest")

set (RECLUSE_TEST_NAME "RecluseRenderApiTest")

set(RECLUSE_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/VulkanTest.cpp)

add_executable(${RECLUSE_TEST_NAME} ${RECLUSE_TEST_FILES})
