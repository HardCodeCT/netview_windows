cmake_minimum_required(VERSION 3.20)
project(NetView VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows-specific settings
if(WIN32)
    # Set Windows version (Windows 7+)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DUNICODE -D_UNICODE)
    
    # Enable resource compilation
    enable_language(RC)
    
    # Set subsystem to Windows (GUI app)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    
    # Optimize for release
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /Ot /GL")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    endif()
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/MonitorWidget.cpp
    src/DataManager.cpp
    src/HistoryManager.cpp
    src/HistoryWindow.cpp
    src/ContactWindow.cpp
    src/AutoStartManager.cpp
    src/UIHelper.cpp
    src/FontManager.cpp
    src/LicenseManager.cpp
    src/FirebaseManager.cpp
    src/PaymentWindow.cpp
)

# Header files
set(HEADERS
    src/MonitorWidget.h
    src/DataManager.h
    src/HistoryManager.h
    src/HistoryWindow.h
    src/ContactWindow.h
    src/AutoStartManager.h
    src/UIHelper.h
    src/FontManager.h
    src/LicenseManager.h
    src/FirebaseManager.h
    src/PaymentWindow.h
    src/resource.h
)

# Resource file (includes icon, embedded PNGs, and embedded font)
set(RESOURCES
    src/resource.rc
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    iphlpapi
    ws2_32
    dwmapi
    gdi32
    gdiplus
    user32
    kernel32
    shell32
    ole32
    oleaut32
    advapi32
    taskschd
    comsupp
    winhttp
    comctl32
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "NetView"
)

# Copy documentation
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/docs
        ${CMAKE_BINARY_DIR}/bin/docs
    COMMENT "Copying documentation..."
)

# Build success message
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "═══════════════════════════════════════════"
    COMMAND ${CMAKE_COMMAND} -E echo "  NetView Build Complete!"
    COMMAND ${CMAKE_COMMAND} -E echo "═══════════════════════════════════════════"
    COMMAND ${CMAKE_COMMAND} -E echo "Executable: $<TARGET_FILE:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Features:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ BLACK theme (Monitor Widget)"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ Network monitoring with usage history"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ Daily/Monthly/Yearly statistics"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ Persistent history tracking"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ Glass-style Contact button (top center)"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ Professional contact window with PNG icons"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ 7-day trial period"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ $4.99 lifetime license"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ Crypto payment integration"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ Firebase backend support"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ PNG icons embedded in executable"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ Custom font embedded in executable"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ No external files required"
    COMMAND ${CMAKE_COMMAND} -E echo "  ✓ Professional installer ready"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Payment System:"
    COMMAND ${CMAKE_COMMAND} -E echo "  • 6 crypto wallets (BTC, ETH, USDT, BNB, SOL, LTC)"
    COMMAND ${CMAKE_COMMAND} -E echo "  • Firebase payment verification"
    COMMAND ${CMAKE_COMMAND} -E echo "  • Trial tracking system"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Contact Options:"
    COMMAND ${CMAKE_COMMAND} -E echo "  • X (Twitter): @Hard_Code_T"
    COMMAND ${CMAKE_COMMAND} -E echo "  • WhatsApp: +234 816 571 3623"
    COMMAND ${CMAKE_COMMAND} -E echo "  • Email: firmino3535@gmail.com"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Next: Configure Firebase project URL"
    COMMAND ${CMAKE_COMMAND} -E echo "      in FirebaseManager.h and build installer"
    COMMAND ${CMAKE_COMMAND} -E echo "═══════════════════════════════════════════"
)

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/docs/
    DESTINATION docs
)

# Print configuration
message(STATUS "═══════════════════════════════════════════")
message(STATUS "  NetView Configuration")
message(STATUS "═══════════════════════════════════════════")
message(STATUS "CMake: ${CMAKE_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "")
message(STATUS "UI Theme: BLACK backgrounds with glass effects")
message(STATUS "Icon Support: PNG (Embedded Resources)")
message(STATUS "Font: Custom TTF (Embedded)")
message(STATUS "History: Persistent (Registry)")
message(STATUS "Contact: Glass button (top center)")
message(STATUS "History: Button (bottom right)")
message(STATUS "License: 7-day trial + $4.99 lifetime")
message(STATUS "Payment: 6 crypto currencies")
message(STATUS "Backend: Firebase REST API")
message(STATUS "═══════════════════════════════════════════")
