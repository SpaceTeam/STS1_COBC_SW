#include <Sts1CobcSw/RfProtocols/FileTransferTimer.hpp>

#include <Sts1CobcSw/RodosTime/RodosTime.hpp>


namespace sts1cobcsw
{
FileTransferTimer::FileTransferTimer(Duration duration, RodosTime fileTransferWindowEnd)
    : expirationTime_(CurrentRodosTime() + duration), fileTransferWindowEnd_(fileTransferWindowEnd)
{}


auto FileTransferTimer::Set(Duration duration) -> void
{
    expirationTime_ = CurrentRodosTime() + duration;
}


auto FileTransferTimer::UpdateFileTransferWindowEnd(RodosTime fileTransferWindowEnd) -> void
{
    if(fileTransferWindowEnd_ == fileTransferWindowEnd)
    {
        return;
    }
    auto now = CurrentRodosTime();
    if(fileTransferWindowEnd_ > now)
    {
        if(fileTransferWindowEnd > now)
        {
            fileTransferWindowEnd_ = fileTransferWindowEnd;
        }
        else
        {
            fileTransferWindowEnd_ = now;
        }
    }
    else
    {
        if(fileTransferWindowEnd > now)
        {
            auto remainingDuration = expirationTime_ - fileTransferWindowEnd_;
            expirationTime_ = now + remainingDuration;
            fileTransferWindowEnd_ = fileTransferWindowEnd;
        }
    }
}


auto FileTransferTimer::ExpirationTime() const -> RodosTime
{
    return expirationTime_ < fileTransferWindowEnd_ ? expirationTime_ : endOfTime;
}


auto FileTransferTimer::HasExpired() const -> bool
{
    return CurrentRodosTime() >= ExpirationTime();
}
}
