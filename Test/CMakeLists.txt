#
cmake_minimum_required(VERSION 3.5)
project("RecluseRenderApiTest")

set (RECLUSE_TEST_NAME "RecluseRenderApiTest")

set(RECLUSE_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/VulkanTest.cpp)

add_executable(${RECLUSE_TEST_NAME} ${RECLUSE_TEST_FILES})

target_include_directories(${RECLUSE_TEST_NAME} PUBLIC ${RECLUSE_RENDER_API_INCLUDES}
	${RECLUSE_FRAMEWORK_PATH}/Include
	${RECLUSE_FRAMEWORK_BUILD_PATH}/Include/RecluseFramework
	${CMAKE_SOURCE_DIR}/Recluse/Build/Include/RecluseRenderApi)