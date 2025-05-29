// Linker symbols to memory segments
extern char * _sidata;
extern char volatile * _sdata;
extern char volatile * _edata;
extern char volatile * _sbss;
extern char volatile * _ebss;


extern "C"
{
auto InitializeDataSegment() -> void;
auto InitializeBssSegment() -> void;
}


auto InitializeDataSegment() -> void
{
    auto size = _edata - _sdata;
    for(auto i = 0; i < size; i++)
    {
        _sdata[i] = _sidata[i];
    }
}


auto InitializeBssSegment() -> void
{
    auto size = _ebss - _sbss;
    for(auto i = 0; i < size; i++)
    {
        _sbss[i] = 0;
    }
}
