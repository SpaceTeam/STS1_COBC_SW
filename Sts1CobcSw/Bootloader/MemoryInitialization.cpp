// Disable C++ name mangling because we call these functions in Startup.s with exactly these names.
extern "C"
{
auto InitializeDataSegment() -> void;
auto InitializeBssSegment() -> void;
}


auto InitializeDataSegment() -> void
{
    // Linker symbols to data segments
    extern char * _sidata;          // NOLINT
    extern char volatile * _sdata;  // NOLINT
    extern char volatile * _edata;  // NOLINT
    auto size = _edata - _sdata;
    for(auto i = 0; i < size; i++)
    {
        _sdata[i] = _sidata[i];  // NOLINT(*pointer-arithmetic)
    }
}


auto InitializeBssSegment() -> void
{
    // Linker symbols to bss segment
    extern char volatile * _sbss;  // NOLINT
    extern char volatile * _ebss;  // NOLINT
    auto size = _ebss - _sbss;
    for(auto i = 0; i < size; i++)
    {
        _sbss[i] = 0;  // NOLINT(*pointer-arithmetic)
    }
}
