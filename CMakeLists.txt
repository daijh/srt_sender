set(target_name srt_sender)

ADD_DEFINITIONS("-g -c -fPIC -std=c++11 -Wno-attributes")

INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/deps/release/include")
LINK_DIRECTORIES("${PROJECT_SOURCE_DIR}/deps/release/lib")

SET(LINK_LIB ${LINK_LIB} -Wl,--start-group)
SET(LINK_LIB ${LINK_LIB} pthread)
SET(LINK_LIB ${LINK_LIB} avfilter avformat avcodec avdevice avutil swscale swresample)
SET(LINK_LIB ${LINK_LIB} srt)
SET(LINK_LIB ${LINK_LIB} -Wl,--end-group)

AUX_SOURCE_DIRECTORY(. SRCS)
ADD_EXECUTABLE(${target_name} ${SRCS})
TARGET_LINK_LIBRARIES(${target_name} ${LINK_LIB})
