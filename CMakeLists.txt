cmake_minimum_required(VERSION 3.21)
project(comm-sampling VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network SerialPort)

set(PROJECT_SOURCES
    src/main.cpp
    src/common/ByteCache.cpp
    src/common/HexUtils.cpp
    src/common/RecentDataStore.cpp
    src/common/ProfileStore.cpp
    src/communication/AbstractChannel.h
    src/communication/ChannelManager.cpp
    src/communication/SerialChannel.cpp
    src/communication/TcpClientChannel.cpp
    src/communication/TcpServerChannel.cpp
    src/matchers/DecimalConverter.cpp
    src/matchers/FixedKeywordFrameMatcher.cpp
    src/matchers/DecimalPickerMatcher.cpp
    src/matchers/MatcherPipeline.cpp
    src/sender/DataSender.cpp
    src/ui/MainWindow.cpp
    src/ui/CommunicationSettingsWidget.cpp
    src/ui/SendingWidget.cpp
    src/ui/ProfileWidget.cpp
    src/ui/RawDataWidget.cpp
    src/ui/MatchersWidget.cpp
    src/ui/MatcherItemWidget.cpp
    src/ui/LineChartWidget.cpp
)

qt_add_executable(comm-sampling ${PROJECT_SOURCES})

target_include_directories(comm-sampling PRIVATE src)

target_link_libraries(comm-sampling PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::SerialPort
)

if(APPLE)
    set_target_properties(comm-sampling PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Comm Sampling"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.commsampling"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
elseif(WIN32)
    set_target_properties(comm-sampling PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
