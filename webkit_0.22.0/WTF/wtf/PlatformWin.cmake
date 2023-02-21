list(APPEND WTF_SOURCES
    text/cf/AtomicStringImplCF.cpp
    text/cf/StringCF.cpp
    text/cf/StringImplCF.cpp
    text/cf/StringViewCF.cpp

    threads/win/BinarySemaphoreWin.cpp

    win/MainThreadWin.cpp
    win/RunLoopWin.cpp
    win/WTFDLL.cpp
    win/WorkItemWin.cpp
    win/WorkQueueWin.cpp
)

list(APPEND WTF_LIBRARIES
    winmm
)

if (${WTF_PLATFORM_WIN_CAIRO})
    list(APPEND WTF_LIBRARIES
        cflite
    )
endif ()

list(APPEND WTF_HEADERS
    "${DERIVED_SOURCES_WTF_DIR}/WTFHeaderDetection.h"
)

# FIXME: This should run testOSXLevel.cmd if it is available.
# https://bugs.webkit.org/show_bug.cgi?id=135861
add_custom_command(
    OUTPUT "${DERIVED_SOURCES_WTF_DIR}/WTFHeaderDetection.h"
    WORKING_DIRECTORY "${DERIVED_SOURCES_WTF_DIR}"
    COMMAND echo /* No Legible Output Support Found */ > WTFHeaderDetection.h
    VERBATIM)
