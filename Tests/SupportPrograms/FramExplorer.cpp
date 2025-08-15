#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/char_traits.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/to_string.h>
#include <etl/utility.h>
#include <etl/vector.h>

#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>


namespace sts1cobcsw
{
using RODOS::PRINTF;


namespace
{
using Message = etl::string<100>;
using Token = etl::string<30>;
using Input = etl::vector<Token, 10>;
using ValueString = etl::string<16>;


constexpr auto stackSize = 60'000U;
auto uart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


auto PrintUsageInfo() -> void;
auto ReadFromUart() -> Input;
auto HandleInvalidInput() -> void;
auto HandleGetCommand(Input const & input) -> void;
auto HandleSetCommand(Input const & input) -> void;
auto PrintAllVariables() -> void;
auto ResetAllVariables() -> void;
auto PrintVariable(etl::string_view variable) -> void;
auto SetVariable(etl::string_view variable, etl::string_view value) -> void;
template<StringLiteral name, typename T, auto parseFunction>  // NOLINTNEXTLINE(*value-param)
auto WriteAndConvertFunction(etl::string_view variable, etl::string_view value) -> void;
auto ParseAsUInt32(etl::string_view string) -> Result<std::uint32_t>;
auto ParseAsBool(etl::string_view string) -> Result<bool>;
auto ParseAsPartitionId(etl::string_view string) -> Result<fw::PartitionId>;
auto ToString(fw::PartitionId id, etl::istring * string) -> void;


class FramExplorer : public RODOS::StaticThread<stackSize>
{
public:
    FramExplorer() : StaticThread("FramExplorer")
    {}


private:
    auto init() -> void override
    {
        auto const baudRate = 115'200;
        hal::Initialize(&uart, baudRate);
    }


    auto run() -> void override
    {
        PRINTF("\nFRAM explorer\n\n");
        PrintUsageInfo();
        fram::Initialize();
        while(true)
        {
            PRINTF("\n");
            auto input = ReadFromUart();
            if(input.size() < 2)
            {
                HandleInvalidInput();
                continue;
            }
            auto const & commandName = input.front();
            if(commandName == "get")
            {
                HandleGetCommand(input);
            }
            else if(commandName == "set")
            {
                HandleSetCommand(input);
            }
            else
            {
                HandleInvalidInput();
            }
        }
    }
} framExplorer;


auto PrintUsageInfo() -> void
{
    PRINTF("Usage:\n");
    PRINTF("  get --all                to get all variables\n");
    PRINTF("  get <variable>           to get one variable\n");
    PRINTF("  set <variable> <value>   to set one variable\n");
    PRINTF("  set --all                to reset all variables to 0\n");
}


auto ReadFromUart() -> Input
{
    auto input = Input{};
    auto token = Token{};
    for(auto i = 0U; i < Message::MAX_SIZE; ++i)
    {
        char character;  // NOLINT(*init-variables)
        hal::ReadFrom(&uart, Span(&character));
        if(character == ' ' or character == '\n')
        {
            if(not token.empty())
            {
                input.push_back(token);
                token.clear();
            }
            if(character == '\n' or input.full())
            {
                break;
            }
        }
        else
        {
            token.push_back(character);
        }
    }
    return input;
}


auto HandleInvalidInput() -> void
{
    PRINTF("Invalid input received\n\n");
    PrintUsageInfo();
}


auto HandleGetCommand(Input const & input) -> void
{
    if(input[1] == "--all")
    {
        PrintAllVariables();
        return;
    }
    for(auto && variableName : std::span(input).subspan<1>())
    {
        PrintVariable(variableName);
    }
}


auto HandleSetCommand(Input const & input) -> void
{
    if(input[1] == "--all")
    {
        ResetAllVariables();
        return;
    }
    // The set command requires variable-value pairs -> even number of arguments
    auto nArguments = input.size() - 1;
    if(nArguments % 2 != 0)
    {
        HandleInvalidInput();
        return;
    }
    for(auto i = 1U; i < input.size(); i += 2)
    {
        auto const & variable = input[i];
        auto const & value = input[i + 1];
        SetVariable(variable, value);
    }
}


auto PrintAllVariables() -> void
{
    // Bootloader
    PrintVariable("nTotalResets");
    PrintVariable("nResetsSinceRf");
    PrintVariable("activeSecondaryFwPartition");
    PrintVariable("backupSecondaryFwPartition");
    // Housekeeping
    PrintVariable("txIsOn");
    PrintVariable("fileTransferWindowEnd");
    PrintVariable("antennasShouldBeDeployed");
    PrintVariable("realTime");
    PrintVariable("realTimeOffset");
    PrintVariable("realTimeOffsetCorrection");
    PrintVariable("nFirmwareChecksumErrors");
    PrintVariable("epsIsWorking");
    PrintVariable("flashIsWorking");
    PrintVariable("rfIsWorking");
    PrintVariable("nFlashErrors");
    PrintVariable("nRfErrors");
    PrintVariable("nFileSystemErrors");
    // EDU
    PrintVariable("eduShouldBePowered");
    PrintVariable("eduStartDelayLimit");
    PrintVariable("newEduResultIsAvailable");
    PrintVariable("eduProgramQueueIndex");
    PrintVariable("nEduCommunicationErrors");
    // Communication
    PrintVariable("nCorrectableUplinkErrors");
    PrintVariable("nUncorrectableUplinkErrors");
    PrintVariable("nGoodTransferFrames");
    PrintVariable("nBadTransferFrames");
    PrintVariable("lastFrameSequenceNumber");
    PrintVariable("lastMessageTypeId");
    PrintVariable("lastMessageTypeIdWasInvalid");
    PrintVariable("lastApplicationDataWasInvalid");
}


auto ResetAllVariables() -> void
{
    // Bootloader
    persistentVariables.Store<"nTotalResets">(0);
    persistentVariables.Store<"nResetsSinceRf">(0);
    persistentVariables.Store<"activeSecondaryFwPartition">(sts1cobcsw::fw::PartitionId::primary);
    persistentVariables.Store<"backupSecondaryFwPartition">(sts1cobcsw::fw::PartitionId::primary);
    // Housekeeping
    persistentVariables.Store<"txIsOn">(false);
    persistentVariables.Store<"fileTransferWindowEnd">(static_cast<RodosTime>(0));
    persistentVariables.Store<"antennasShouldBeDeployed">(false);
    persistentVariables.Store<"realTime">(static_cast<RealTime>(0));
    persistentVariables.Store<"realTimeOffset">(static_cast<Duration>(0));
    persistentVariables.Store<"realTimeOffsetCorrection">(static_cast<Duration>(0));
    persistentVariables.Store<"nFirmwareChecksumErrors">(0);
    persistentVariables.Store<"epsIsWorking">(false);
    persistentVariables.Store<"flashIsWorking">(false);
    persistentVariables.Store<"rfIsWorking">(false);
    persistentVariables.Store<"nFlashErrors">(0);
    persistentVariables.Store<"nRfErrors">(0);
    persistentVariables.Store<"nFileSystemErrors">(0);
    // EDU
    persistentVariables.Store<"eduShouldBePowered">(false);
    persistentVariables.Store<"eduStartDelayLimit">(static_cast<Duration>(0));
    persistentVariables.Store<"newEduResultIsAvailable">(false);
    persistentVariables.Store<"eduProgramQueueIndex">(0);
    persistentVariables.Store<"nEduCommunicationErrors">(0);
    // Communication
    persistentVariables.Store<"nCorrectableUplinkErrors">(0);
    persistentVariables.Store<"nUncorrectableUplinkErrors">(0);
    persistentVariables.Store<"nGoodTransferFrames">(0);
    persistentVariables.Store<"nBadTransferFrames">(0);
    persistentVariables.Store<"lastFrameSequenceNumber">(0);
    // TODO: Implement MessageTypeId
    // persistentVariables.Store<"lastMessageTypeId">(0);
    persistentVariables.Store<"lastMessageTypeIdWasInvalid">(false);
    persistentVariables.Store<"lastApplicationDataWasInvalid">(false);

    PrintAllVariables();
}


auto PrintVariable(etl::string_view variable) -> void  // NOLINT(*value-param)
{
    auto value = Message{};
    // Bootloader
    if(variable == "nTotalResets")
    {
        etl::to_string(persistentVariables.Load<"nTotalResets">(), value);
    }
    else if(variable == "nResetsSinceRf")
    {
        etl::to_string(persistentVariables.Load<"nResetsSinceRf">(), value);
    }
    else if(variable == "activeSecondaryFwPartition")
    {
        ToString(persistentVariables.Load<"activeSecondaryFwPartition">(), &value);
    }
    else if(variable == "backupSecondaryFwPartition")
    {
        ToString(persistentVariables.Load<"backupSecondaryFwPartition">(), &value);
    }
    // Housekeeping
    else if(variable == "txIsOn")
    {
        etl::to_string(persistentVariables.Load<"txIsOn">(), value);
    }
    else if(variable == "fileTransferWindowEnd")
    {
        etl::to_string(persistentVariables.Load<"fileTransferWindowEnd">().value_of(), value);
    }
    else if(variable == "antennasShouldBeDeployed")
    {
        etl::to_string(persistentVariables.Load<"antennasShouldBeDeployed">(), value);
    }
    else if(variable == "realTime")
    {
        etl::to_string(persistentVariables.Load<"realTime">().value_of(), value);
    }
    else if(variable == "realTimeOffset")
    {
        etl::to_string(persistentVariables.Load<"realTimeOffset">().value_of(), value);
    }
    else if(variable == "realTimeOffsetCorrection")
    {
        etl::to_string(persistentVariables.Load<"realTimeOffsetCorrection">().value_of(), value);
    }
    else if(variable == "nFirmwareChecksumErrors")
    {
        etl::to_string(persistentVariables.Load<"nFirmwareChecksumErrors">(), value);
    }
    else if(variable == "epsIsWorking")
    {
        etl::to_string(persistentVariables.Load<"epsIsWorking">(), value);
    }
    else if(variable == "flashIsWorking")
    {
        etl::to_string(persistentVariables.Load<"flashIsWorking">(), value);
    }
    else if(variable == "rfIsWorking")
    {
        etl::to_string(persistentVariables.Load<"rfIsWorking">(), value);
    }
    else if(variable == "nFlashErrors")
    {
        etl::to_string(persistentVariables.Load<"nFlashErrors">(), value);
    }
    else if(variable == "nRfErrors")
    {
        etl::to_string(persistentVariables.Load<"nRfErrors">(), value);
    }
    else if(variable == "nFileSystemErrors")
    {
        etl::to_string(persistentVariables.Load<"nFileSystemErrors">(), value);
    }
    // EDU
    else if(variable == "eduShouldBePowered")
    {
        etl::to_string(persistentVariables.Load<"eduShouldBePowered">(), value);
    }
    else if(variable == "eduStartDelayLimit")
    {
        etl::to_string(persistentVariables.Load<"eduStartDelayLimit">().value_of(), value);
    }
    else if(variable == "newEduResultIsAvailable")
    {
        etl::to_string(persistentVariables.Load<"newEduResultIsAvailable">(), value);
    }
    else if(variable == "eduProgramQueueIndex")
    {
        etl::to_string(persistentVariables.Load<"eduProgramQueueIndex">(), value);
    }
    else if(variable == "nEduCommunicationErrors")
    {
        etl::to_string(persistentVariables.Load<"nEduCommunicationErrors">(), value);
    }
    // Communication
    else if(variable == "nCorrectableUplinkErrors")
    {
        etl::to_string(persistentVariables.Load<"nCorrectableUplinkErrors">(), value);
    }
    else if(variable == "nUncorrectableUplinkErrors")
    {
        etl::to_string(persistentVariables.Load<"nUncorrectableUplinkErrors">(), value);
    }
    else if(variable == "nGoodTransferFrames")
    {
        etl::to_string(persistentVariables.Load<"nGoodTransferFrames">(), value);
    }
    else if(variable == "nBadTransferFrames")
    {
        etl::to_string(persistentVariables.Load<"nBadTransferFrames">(), value);
    }
    else if(variable == "lastFrameSequenceNumber")
    {
        etl::to_string(persistentVariables.Load<"lastFrameSequenceNumber">(), value);
    }
    else if(variable == "lastMessageTypeId")
    {
        value = "NOT SUPPORTED!";
        // ToDo: implement MessageTypeId
        // etl::to_string(persistentVariables.Load<"lastMessageTypeId">(), value);
    }
    else if(variable == "lastMessageTypeIdWasInvalid")
    {
        etl::to_string(persistentVariables.Load<"lastMessageTypeIdWasInvalid">(), value);
    }
    else if(variable == "lastApplicationDataWasInvalid")
    {
        etl::to_string(persistentVariables.Load<"lastApplicationDataWasInvalid">(), value);
    }
    else
    {
        PRINTF("Variable %s not found!\n", Message(variable).c_str());
        return;
    }
    PRINTF("%s = %s\n", Message(variable).c_str(), value.c_str());
}


auto SetVariable(etl::string_view variable, etl::string_view value) -> void  // NOLINT(*value-param)
{
    // Bootloader
    if(variable == "nTotalResets")
    {
        WriteAndConvertFunction<"nTotalResets", std::uint32_t, ParseAsUInt32>(variable, value);
    }
    else if(variable == "nResetsSinceRf")
    {
        WriteAndConvertFunction<"nResetsSinceRf", std::uint8_t, ParseAsUInt32>(variable, value);
    }
    else if(variable == "activeSecondaryFwPartition")
    {
        WriteAndConvertFunction<"activeSecondaryFwPartition", fw::PartitionId, ParseAsPartitionId>(
            variable, value);
    }
    else if(variable == "backupSecondaryFwPartition")
    {
        WriteAndConvertFunction<"backupSecondaryFwPartition", fw::PartitionId, ParseAsPartitionId>(
            variable, value);
    }
    // Housekeeping
    else if(variable == "txIsOn")
    {
        WriteAndConvertFunction<"txIsOn", bool, ParseAsBool>(variable, value);
    }
    else if(variable == "fileTransferWindowEnd")
    {
        WriteAndConvertFunction<"fileTransferWindowEnd", RodosTime, ParseAsUInt32>(variable, value);
    }
    else if(variable == "antennasShouldBeDeployed")
    {
        WriteAndConvertFunction<"antennasShouldBeDeployed", bool, ParseAsBool>(variable, value);
    }
    else if(variable == "realTime")
    {
        WriteAndConvertFunction<"realTime", RealTime, ParseAsUInt32>(variable, value);
    }
    else if(variable == "realTimeOffset")
    {
        WriteAndConvertFunction<"realTimeOffset", Duration, ParseAsUInt32>(variable, value);
    }
    else if(variable == "realTimeOffsetCorrection")
    {
        WriteAndConvertFunction<"realTimeOffsetCorrection", Duration, ParseAsUInt32>(variable,
                                                                                     value);
    }
    else if(variable == "nFirmwareChecksumErrors")
    {
        WriteAndConvertFunction<"nFirmwareChecksumErrors", std::uint8_t, ParseAsUInt32>(variable,
                                                                                        value);
    }
    else if(variable == "epsIsWorking")
    {
        WriteAndConvertFunction<"epsIsWorking", bool, ParseAsBool>(variable, value);
    }
    else if(variable == "flashIsWorking")
    {
        WriteAndConvertFunction<"flashIsWorking", bool, ParseAsBool>(variable, value);
    }
    else if(variable == "rfIsWorking")
    {
        WriteAndConvertFunction<"rfIsWorking", bool, ParseAsBool>(variable, value);
    }
    else if(variable == "nFlashErrors")
    {
        WriteAndConvertFunction<"nFlashErrors", std::uint8_t, ParseAsUInt32>(variable, value);
    }
    else if(variable == "nRfErrors")
    {
        WriteAndConvertFunction<"nRfErrors", std::uint8_t, ParseAsUInt32>(variable, value);
    }
    else if(variable == "nFileSystemErrors")
    {
        WriteAndConvertFunction<"nFileSystemErrors", std::uint8_t, ParseAsUInt32>(variable, value);
    }
    // EDU
    else if(variable == "eduShouldBePowered")
    {
        WriteAndConvertFunction<"eduShouldBePowered", bool, ParseAsBool>(variable, value);
    }
    else if(variable == "eduStartDelayLimit")
    {
        WriteAndConvertFunction<"eduStartDelayLimit", Duration, ParseAsUInt32>(variable, value);
    }
    else if(variable == "newEduResultIsAvailable")
    {
        WriteAndConvertFunction<"newEduResultIsAvailable", bool, ParseAsBool>(variable, value);
    }
    else if(variable == "eduProgramQueueIndex")
    {
        WriteAndConvertFunction<"eduProgramQueueIndex", std::uint8_t, ParseAsUInt32>(variable,
                                                                                     value);
    }
    else if(variable == "nEduCommunicationErrors")
    {
        WriteAndConvertFunction<"nEduCommunicationErrors", std::uint8_t, ParseAsUInt32>(variable,
                                                                                        value);
    }
    // Communication
    else if(variable == "nCorrectableUplinkErrors")
    {
        WriteAndConvertFunction<"nCorrectableUplinkErrors", std::uint16_t, ParseAsUInt32>(variable,
                                                                                          value);
    }
    else if(variable == "nUncorrectableUplinkErrors")
    {
        WriteAndConvertFunction<"nUncorrectableUplinkErrors", std::uint16_t, ParseAsUInt32>(
            variable, value);
    }
    else if(variable == "nGoodTransferFrames")
    {
        WriteAndConvertFunction<"nGoodTransferFrames", std::uint16_t, ParseAsUInt32>(variable,
                                                                                     value);
    }
    else if(variable == "nBadTransferFrames")
    {
        WriteAndConvertFunction<"nBadTransferFrames", std::uint16_t, ParseAsUInt32>(variable,
                                                                                    value);
    }
    else if(variable == "lastFrameSequenceNumber")
    {
        WriteAndConvertFunction<"lastFrameSequenceNumber", std::uint8_t, ParseAsUInt32>(variable,
                                                                                        value);
    }
    else if(variable == "lastMessageTypeId")
    {
        PRINTF("NOT SUPPORTED!\n");
        // TODO: Implement messageType Field
        // WriteAndConvertFunction<"lastMessageTypeId", MessageTypeIdFields,
        // ParseToUint32>(variable, value);
    }
    else if(variable == "lastMessageTypeIdWasInvalid")
    {
        WriteAndConvertFunction<"lastMessageTypeIdWasInvalid", bool, ParseAsBool>(variable, value);
    }
    else if(variable == "lastApplicationDataWasInvalid")
    {
        WriteAndConvertFunction<"lastApplicationDataWasInvalid", bool, ParseAsBool>(variable,
                                                                                    value);
    }
    else
    {
        PRINTF("Variable %s not found!\n", Message(variable).c_str());
        return;
    }

    PRINTF("%s <- %s\n", Message(variable).c_str(), Message(value).c_str());
}


template<StringLiteral name, typename T, auto parseFunction>  // NOLINTNEXTLINE(*value-param)
auto WriteAndConvertFunction(etl::string_view variable, etl::string_view value) -> void
{
    auto convertedValueResult = parseFunction(value);
    if(convertedValueResult.has_error())
    {
        PRINTF("Value %s for variable %s could not be converted!\n",
               Message(value).c_str(),
               Message(variable).c_str());

        if constexpr(std::is_same_v<decltype(parseFunction), decltype(&ParseAsPartitionId)>)
        {
            PRINTF("Use:\n0 or primary\n1 or secondary1\n2 or secondary2\n");
        }
    }
    else
    {
        persistentVariables.Store<name>(static_cast<T>(convertedValueResult.value()));
    }
}


auto ParseAsUInt32(etl::string_view string) -> Result<std::uint32_t>  // NOLINT(*value-param)
{
    auto value = 0U;
    for(auto ch : string)
    {
        if(ch < '0' || ch > '9')
        {
            return ErrorCode::invalidParameter;
        }

        auto digit = static_cast<std::uint32_t>(ch - '0');
        if(value > (UINT32_MAX - digit) / 10)
        {
            // Overflow
            return ErrorCode::invalidParameter;
        }

        value = value * 10 + digit;
    }

    return value;
}


auto ParseAsBool(etl::string_view string) -> Result<bool>  // NOLINT(*value-param)
{
    if(string == "true" || string == "1")
    {
        return true;
    }
    if(string == "false" || string == "0")
    {
        return false;
    }

    return ErrorCode::invalidParameter;
}


auto ParseAsPartitionId(etl::string_view string) -> Result<fw::PartitionId>  // NOLINT(*value-param)
{
    if(string == "primary" || string == "0")
    {
        return fw::PartitionId::primary;
    }
    if(string == "secondary1" || string == "1")
    {
        return fw::PartitionId::secondary1;
    }
    if(string == "secondary2" || string == "2")
    {
        return fw::PartitionId::secondary2;
    }

    return ErrorCode::invalidParameter;
}


auto ToString(fw::PartitionId id, etl::istring * string) -> void
{
    switch(id)
    {
        case fw::PartitionId::primary:
            *string = "primary";
            break;
        case fw::PartitionId::secondary1:
            *string = "secondary1";
            break;
        case fw::PartitionId::secondary2:
            *string = "secondary2";
            break;
    }
}
}
}
