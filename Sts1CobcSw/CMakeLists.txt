if(BUILD_BOOTLOADER)
    add_subdirectory(Bootloader)
else()
    add_subdirectory(Blake2s)
    add_subdirectory(ChannelCoding)
    add_subdirectory(Edu)
    add_subdirectory(FileSystem)
    add_subdirectory(Firmware)
    add_subdirectory(FirmwareManagement)
    add_subdirectory(FramSections)
    add_subdirectory(Hal)
    add_subdirectory(Outcome)
    add_subdirectory(RealTime)
    add_subdirectory(RodosTime)
    add_subdirectory(Serial)
    add_subdirectory(Mailbox)
    add_subdirectory(Telemetry)
    add_subdirectory(Utility)
    add_subdirectory(Vocabulary)
    add_subdirectory(WatchdogTimers)
    add_subdirectory(Rf)
    add_subdirectory(RfProtocols)
    add_subdirectory(Flash)
    add_subdirectory(Fram)
    add_subdirectory(Sensors)
endif()
