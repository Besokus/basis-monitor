cmake_minimum_required(VERSION 3.20)

project(basis_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BASIS_MONITOR_FILESYSTEM_LIB "")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    set(BASIS_MONITOR_FILESYSTEM_LIB stdc++fs)
endif()

enable_testing()
find_package(Threads REQUIRED)

set(XTP_QUOTE_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../XTPXQuoteAPI_1.0.15_20260113")
set(XTP_QUOTE_INCLUDE_DIR "${XTP_QUOTE_SDK_ROOT}/header")
set(XTP_QUOTE_LIB "${XTP_QUOTE_SDK_ROOT}/lib/centos/onload-8.1.2.26/libxtpxquoteapi.so")
set(CTP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ctp/live/include")
set(CTP_MD_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ctp/live/lib/linux")
set(CTP_MD_LIB "${CTP_MD_LIB_DIR}/thostmduserapi_se.so")
set(CTP_DATA_COLLECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ctp/data_collect")
set(CTP_DATA_COLLECT_INCLUDE_DIR "${CTP_DATA_COLLECT_ROOT}")
set(CTP_DATA_COLLECT_LIB "${CTP_DATA_COLLECT_ROOT}/LinuxDataCollect.so")

if(NOT EXISTS "${CTP_INCLUDE_DIR}/ThostFtdcMdApi.h")
    message(FATAL_ERROR "Missing formal live CTP headers under vendor/ctp/live/include")
endif()

if(NOT EXISTS "${CTP_MD_LIB}")
    message(FATAL_ERROR "Missing formal live CTP MdApi library under vendor/ctp/live/lib/linux")
endif()

if(NOT EXISTS "${CTP_DATA_COLLECT_INCLUDE_DIR}/DataCollect.h" OR NOT EXISTS "${CTP_DATA_COLLECT_LIB}")
    message(FATAL_ERROR "Missing CTP data collect files under vendor/ctp/data_collect")
endif()

if(UNIX AND EXISTS "${XTP_QUOTE_LIB}")
    add_library(xtp_quote SHARED IMPORTED GLOBAL)
    set_target_properties(xtp_quote PROPERTIES
        IMPORTED_LOCATION "${XTP_QUOTE_LIB}"
        IMPORTED_NO_SONAME TRUE
    )
endif()

if(UNIX)
    set(BASIS_MONITOR_RUNTIME_RPATH
        "${CTP_MD_LIB_DIR};${CTP_DATA_COLLECT_ROOT};\$ORIGIN/../../XTPXQuoteAPI_1.0.15_20260113/lib/centos/onload-8.1.2.26")
    set(CMAKE_BUILD_RPATH "${BASIS_MONITOR_RUNTIME_RPATH}")
    set(CMAKE_INSTALL_RPATH "${BASIS_MONITOR_RUNTIME_RPATH}")
    link_directories(
        ${XTP_QUOTE_SDK_ROOT}/lib/centos/onload-8.1.2.26
    )
endif()

add_library(basis_monitor_core
    src/monitor/alert_engine.cpp
    src/monitor/basis_calculator.cpp
    src/monitor/basis_monitor_service.cpp
    src/monitor/market_data_health_registry.cpp
    src/monitor/market_data_health_tracker.cpp
)

target_include_directories(basis_monitor_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CTP_INCLUDE_DIR}
)

if(UNIX)
    target_compile_options(basis_monitor_core PRIVATE -Wall -Wextra -pedantic)
endif()

add_library(basis_monitor_market_data
    src/config/config_loader.cpp
    src/logging/logger.cpp
    src/ctp/ctp_environment.cpp
    src/ctp/md_api_session.cpp
    src/ctp/md_spi_bridge.cpp
    src/market_data/market_data_session_factory.cpp
    src/market_data/xtp_quote_api.cpp
    src/market_data/xtp_market_data_session.cpp
)

target_include_directories(basis_monitor_market_data
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CTP_INCLUDE_DIR}
        ${XTP_QUOTE_INCLUDE_DIR}
)

target_include_directories(basis_monitor_market_data
    PUBLIC
        ${CTP_DATA_COLLECT_INCLUDE_DIR}
)

if(UNIX)
    target_link_directories(basis_monitor_market_data
        PUBLIC
            ${CTP_MD_LIB_DIR}
            ${CTP_DATA_COLLECT_ROOT}
            ${XTP_QUOTE_SDK_ROOT}/lib/centos/onload-8.1.2.26
    )
endif()

target_link_libraries(basis_monitor_market_data
    PUBLIC
        Threads::Threads
)

if(UNIX)
    target_link_libraries(basis_monitor_market_data
        PUBLIC
            "-l:thostmduserapi_se.so"
    )
endif()

if(UNIX)
    target_link_libraries(basis_monitor_market_data
        PUBLIC
            "-l:LinuxDataCollect.so"
    )
    target_compile_definitions(basis_monitor_market_data PUBLIC BASIS_MONITOR_HAS_CTP_DATA_COLLECT=1)
endif()

if(TARGET xtp_quote)
    target_link_libraries(basis_monitor_market_data PUBLIC xtp_quote)
endif()

if(UNIX)
    target_compile_options(basis_monitor_market_data PRIVATE -Wall -Wextra -pedantic)
endif()

add_library(basis_monitor_storage
    src/storage/storage_common.cpp
    src/storage/tick_store.cpp
    src/storage/basis_result_store.cpp
    src/storage/alert_store.cpp
    src/storage/report_store.cpp
)

target_include_directories(basis_monitor_storage
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(UNIX)
    target_compile_options(basis_monitor_storage PRIVATE -Wall -Wextra -pedantic)
endif()

add_library(basis_monitor_data
    src/data/contract_selector.cpp
    src/data/reference_data_loader.cpp
    src/data/subscription_instrument_builder.cpp
    src/data/reference_subset_builder.cpp
)

target_include_directories(basis_monitor_data
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(UNIX)
    target_compile_options(basis_monitor_data PRIVATE -Wall -Wextra -pedantic)
endif()

if(BASIS_MONITOR_FILESYSTEM_LIB)
    target_link_libraries(basis_monitor_data PUBLIC ${BASIS_MONITOR_FILESYSTEM_LIB})
endif()

add_library(basis_monitor_report
    src/report/latest_basis_snapshot_store.cpp
    src/report/basis_report_formatter.cpp
    src/report/report_image_formatter.cpp
    src/report/report_image_renderer.cpp
    src/report/scheduled_report_service.cpp
)

target_include_directories(basis_monitor_report
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(UNIX)
    target_compile_options(basis_monitor_report PRIVATE -Wall -Wextra -pedantic)
endif()

add_library(basis_monitor_notify
    src/notify/wecom_robot_notifier.cpp
    src/notify/wecom_message_formatter.cpp
)

target_include_directories(basis_monitor_notify
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(UNIX)
    target_compile_options(basis_monitor_notify PRIVATE -Wall -Wextra -pedantic)
endif()

add_executable(basis_monitor
    app/main.cpp
)

target_link_libraries(basis_monitor
    PRIVATE
        basis_monitor_core
        basis_monitor_data
        basis_monitor_market_data
        basis_monitor_storage
        basis_monitor_report
        basis_monitor_notify
)

add_executable(prepare_reference_subset
    app/prepare_reference_subset.cpp
)

target_link_libraries(prepare_reference_subset
    PRIVATE
        basis_monitor_data
)

add_executable(test_basis_calculator
    tests/test_basis_calculator.cpp
)

target_link_libraries(test_basis_calculator PRIVATE basis_monitor_core)

add_test(NAME test_basis_calculator COMMAND test_basis_calculator)

add_executable(test_alert_engine
    tests/test_alert_engine.cpp
)

target_link_libraries(test_alert_engine PRIVATE basis_monitor_core)

add_test(NAME test_alert_engine COMMAND test_alert_engine)

add_executable(test_config_loader
    tests/test_config_loader.cpp
)

target_link_libraries(test_config_loader
    PRIVATE
        basis_monitor_market_data
        basis_monitor_data
)

add_test(NAME test_config_loader COMMAND test_config_loader)

add_executable(test_basis_monitor_service
    tests/test_basis_monitor_service.cpp
)

target_link_libraries(test_basis_monitor_service PRIVATE basis_monitor_core)

add_test(NAME test_basis_monitor_service COMMAND test_basis_monitor_service)

add_executable(test_market_data_health_tracker
    tests/test_market_data_health_tracker.cpp
)

target_link_libraries(test_market_data_health_tracker PRIVATE basis_monitor_core)

add_test(NAME test_market_data_health_tracker COMMAND test_market_data_health_tracker)

add_executable(test_market_data_health_registry
    tests/test_market_data_health_registry.cpp
)

target_link_libraries(test_market_data_health_registry PRIVATE basis_monitor_core)

add_test(NAME test_market_data_health_registry COMMAND test_market_data_health_registry)

add_executable(test_tick_store
    tests/test_tick_store.cpp
)

target_link_libraries(test_tick_store PRIVATE basis_monitor_core)
target_link_libraries(test_tick_store PRIVATE basis_monitor_storage)

add_test(NAME test_tick_store COMMAND test_tick_store)

add_executable(test_basis_result_store
    tests/test_basis_result_store.cpp
)

target_link_libraries(test_basis_result_store PRIVATE basis_monitor_core)
target_link_libraries(test_basis_result_store PRIVATE basis_monitor_storage)

add_test(NAME test_basis_result_store COMMAND test_basis_result_store)

add_executable(test_alert_store
    tests/test_alert_store.cpp
)

target_link_libraries(test_alert_store PRIVATE basis_monitor_core)
target_link_libraries(test_alert_store PRIVATE basis_monitor_storage)

add_test(NAME test_alert_store COMMAND test_alert_store)

add_executable(test_logger
    tests/test_logger.cpp
)

target_link_libraries(test_logger PRIVATE basis_monitor_market_data)

add_test(NAME test_logger COMMAND test_logger)

add_executable(test_reference_data_loader
    tests/test_reference_data_loader.cpp
)

target_link_libraries(test_reference_data_loader PRIVATE basis_monitor_data)

add_test(NAME test_reference_data_loader COMMAND test_reference_data_loader)

add_executable(test_contract_selector
    tests/test_contract_selector.cpp
)

target_link_libraries(test_contract_selector PRIVATE basis_monitor_data)

add_test(NAME test_contract_selector COMMAND test_contract_selector)

add_executable(test_reference_subset_builder
    tests/test_reference_subset_builder.cpp
)

target_link_libraries(test_reference_subset_builder PRIVATE basis_monitor_data)

add_test(NAME test_reference_subset_builder COMMAND test_reference_subset_builder)

add_executable(test_subscription_instrument_builder
    tests/test_subscription_instrument_builder.cpp
)

target_link_libraries(test_subscription_instrument_builder PRIVATE basis_monitor_data)

add_test(NAME test_subscription_instrument_builder COMMAND test_subscription_instrument_builder)

add_executable(test_md_spi_bridge
    tests/test_md_spi_bridge.cpp
)

target_link_libraries(test_md_spi_bridge
    PRIVATE
        basis_monitor_market_data
)

add_test(NAME test_md_spi_bridge COMMAND test_md_spi_bridge)

add_executable(test_md_api_session
    tests/test_md_api_session.cpp
)

target_link_libraries(test_md_api_session
    PRIVATE
        basis_monitor_market_data
)

add_test(NAME test_md_api_session COMMAND test_md_api_session)

add_executable(test_market_data_session_factory
    tests/test_market_data_session_factory.cpp
)

target_link_libraries(test_market_data_session_factory
    PRIVATE
        basis_monitor_market_data
)

add_test(NAME test_market_data_session_factory COMMAND test_market_data_session_factory)

add_executable(test_xtp_market_data_session
    tests/test_xtp_market_data_session.cpp
)

target_link_libraries(test_xtp_market_data_session
    PRIVATE
        basis_monitor_market_data
)

add_test(NAME test_xtp_market_data_session COMMAND test_xtp_market_data_session)

add_executable(test_latest_basis_snapshot_store
    tests/test_latest_basis_snapshot_store.cpp
)

target_link_libraries(test_latest_basis_snapshot_store
    PRIVATE
        basis_monitor_report
)

add_test(NAME test_latest_basis_snapshot_store COMMAND test_latest_basis_snapshot_store)

add_executable(test_basis_report_formatter
    tests/test_basis_report_formatter.cpp
)

target_link_libraries(test_basis_report_formatter
    PRIVATE
        basis_monitor_report
)

add_test(NAME test_basis_report_formatter COMMAND test_basis_report_formatter)

add_executable(test_report_image_formatter
    tests/test_report_image_formatter.cpp
)

target_link_libraries(test_report_image_formatter
    PRIVATE
        basis_monitor_report
)

add_test(NAME test_report_image_formatter COMMAND test_report_image_formatter)

add_executable(test_report_image_renderer
    tests/test_report_image_renderer.cpp
)

target_link_libraries(test_report_image_renderer
    PRIVATE
        basis_monitor_report
)

add_test(NAME test_report_image_renderer COMMAND test_report_image_renderer)

add_executable(test_report_store
    tests/test_report_store.cpp
)

target_link_libraries(test_report_store
    PRIVATE
        basis_monitor_storage
)

add_test(NAME test_report_store COMMAND test_report_store)

add_executable(test_scheduled_report_service
    tests/test_scheduled_report_service.cpp
)

target_link_libraries(test_scheduled_report_service
    PRIVATE
        basis_monitor_report
)

add_test(NAME test_scheduled_report_service COMMAND test_scheduled_report_service)

add_executable(test_wecom_robot_notifier
    tests/test_wecom_robot_notifier.cpp
)

target_link_libraries(test_wecom_robot_notifier
    PRIVATE
        basis_monitor_notify
)

add_test(NAME test_wecom_robot_notifier COMMAND test_wecom_robot_notifier)

add_executable(test_wecom_message_formatter
    tests/test_wecom_message_formatter.cpp
)

target_link_libraries(test_wecom_message_formatter
    PRIVATE
        basis_monitor_core
        basis_monitor_report
        basis_monitor_notify
)

add_test(NAME test_wecom_message_formatter COMMAND test_wecom_message_formatter)
