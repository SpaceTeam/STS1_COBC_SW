#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/FramVector.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/char_traits.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/to_string.h>
#include <etl/utility.h>
#include <etl/vector.h>

#include <array>
#include <charconv>
#include <cstdint>
#include <span>
#include <system_error>
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
constexpr auto framTimeout = 1 * ms;
auto uart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


auto PrintUsageInfo() -> void;
auto ReadFromUart() -> Input;
auto HandleInvalidInput() -> void;
auto HandleGetCommand(std::span<Token const> input) -> void;
auto HandleVarGetCommand(std::span<Token const> input) -> void;
auto HandleQueueGetCommand() -> void;
auto HandleHistoryGetCommand() -> void;
auto HandleSetCommand(std::span<Token const> input) -> void;
auto HandleVarSetCommand(std::span<Token const> input) -> void;
auto HandleQueueSetCommand(std::span<Token const> input) -> void;
auto HandleResetCommand(std::span<Token const> input) -> void;
auto PrintAllVariables() -> void;
auto ResetAllVariables() -> void;
auto PrintVariable(etl::string_view variable) -> void;
auto SetVariable(etl::string_view variable, etl::string_view value) -> void;
template<StringLiteral name, typename T, auto parseFunction>  // NOLINTNEXTLINE(*value-param)
auto WriteAndConvertFunction(etl::string_view variable, etl::string_view value) -> void;
auto ParseAsUInt32(etl::string_view string) -> Result<std::uint32_t>;
auto ParseAsBool(etl::string_view string) -> Result<bool>;
auto ParseAsPartitionId(etl::string_view string) -> Result<fw::PartitionId>;
auto ParseAsMessageTypeIdFields(etl::string_view string) -> Result<MessageTypeIdFields>;
auto ToString(fw::PartitionId id, etl::istring * string) -> void;
auto ToCZString(edu::ProgramStatus status) -> char const *;


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
            auto nCommandsParsed = 1U;
            if(commandName == "get")
            {
                HandleGetCommand(std::span(input).subspan(nCommandsParsed));
            }
            else if(commandName == "set")
            {
                HandleSetCommand(std::span(input).subspan(nCommandsParsed));
            }
            else if(commandName == "reset")
            {
                HandleResetCommand(std::span(input).subspan(nCommandsParsed));
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
    PRINTF("  get var --all                         to get all variables\n");
    PRINTF("  get var <variable>                    to get one variable\n");
    PRINTF("  set var <variable> <value>            to set one variable\n");
    PRINTF("  reset var                             to reset all variables to 0\n");
    PRINTF("  get queue                             to get the EDU queue\n");
    PRINTF("  set queue <id> <startTime> <timeout>  to set the EDU queue\n");
    PRINTF("  reset queue                           to clear the EDU queue of all elements\n");
    PRINTF("  get history                           to get the EDU history\n");
    PRINTF("  reset history                         to reset the EDU history\n");
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


auto HandleGetCommand(std::span<Token const> input) -> void
{
    auto const & secondaryCommandName = input.front();
    auto nCommandsParsed = 1U;
    if(secondaryCommandName == "var")
    {
        HandleVarGetCommand(input.subspan(nCommandsParsed));
        return;
    }
    if(secondaryCommandName == "queue")
    {
        HandleQueueGetCommand();
        return;
    }
    if(secondaryCommandName == "history")
    {
        HandleHistoryGetCommand();
        return;
    }
    HandleInvalidInput();
}


auto HandleVarGetCommand(std::span<Token const> input) -> void
{
    if(input.front() == "--all")
    {
        PrintAllVariables();
        return;
    }
    for(auto && variableName : input)
    {
        PrintVariable(variableName);
    }
}


auto HandleQueueGetCommand() -> void
{
    PRINTF("EDU program queue: current index = %i, size = %i\n",
           persistentVariables.Load<"eduProgramQueueIndex">(),
           static_cast<int>(edu::programQueue.Size()));

    for(auto index = 0U; index < edu::programQueue.Size(); ++index)
    {
        auto entry = edu::programQueue.Get(index);
        // Workaround because RODOS::PRINTF does not support formatting with padding > 9
        if(value_of(entry.startTime) < 1'000'000'000)
        {
            PRINTF("  %2i: program ID = %05i, start time =  %9u, timeout = %4i s\n",
                   index,
                   value_of(entry.programId),
                   static_cast<unsigned>(value_of(entry.startTime)),
                   entry.timeout);
        }
        else
        {
            PRINTF("  %2i: program ID = %05i, start time = %u, timeout = %4i s\n",
                   index,
                   value_of(entry.programId),
                   static_cast<unsigned>(value_of(entry.startTime)),
                   entry.timeout);
        }
    }
}


auto HandleHistoryGetCommand() -> void
{
    PRINTF("EDU program history size = %i:\n", static_cast<int>(edu::programStatusHistory.Size()));
    for(auto index = 0U; index < edu::programStatusHistory.Size(); ++index)
    {
        auto entry = edu::programStatusHistory.Get(index);
        // Workaround because RODOS::PRINTF does not support formatting with padding > 9
        if(value_of(entry.startTime) < 1'000'000'000)
        {
            PRINTF("  %2i: program ID = %05i, start time =  %9u, status = %s\n",
                   index,
                   value_of(entry.programId),
                   static_cast<unsigned>(value_of(entry.startTime)),
                   ToCZString(entry.status));
        }
        else
        {
            PRINTF("  %2i: program ID = %05i, start time = %u, status = %s\n",
                   index,
                   value_of(entry.programId),
                   static_cast<unsigned>(value_of(entry.startTime)),
                   ToCZString(entry.status));
        }
    }
}


auto HandleSetCommand(std::span<Token const> input) -> void
{
    auto const & secondaryCommandName = input.front();
    auto nCommandsParsed = 1U;
    if(secondaryCommandName == "var")
    {
        HandleVarSetCommand(input.subspan(nCommandsParsed));
        return;
    }
    if(secondaryCommandName == "queue")
    {
        HandleQueueSetCommand(input.subspan(nCommandsParsed));
        return;
    }
    HandleInvalidInput();
}


auto HandleVarSetCommand(std::span<Token const> input) -> void
{
    // The set command requires variable-value pairs -> even number of arguments
    auto nArguments = input.size();
    if(nArguments % 2 != 0)
    {
        HandleInvalidInput();
        return;
    }
    for(auto i = 0U; i < input.size(); i += 2)
    {
        auto const & variable = input[i];
        auto const & value = input[i + 1];
        SetVariable(variable, value);
    }
}


auto HandleQueueSetCommand(std::span<Token const> input) -> void
{
    // The set command requires programId-startTime-timeout triplets -> dividable by 3
    auto nArguments = input.size();
    if(nArguments % 3 != 0)
    {
        HandleInvalidInput();
        return;
    }
    for(auto i = 0U; i < input.size(); i += 3)
    {
        auto programIdResult = ParseAsUInt32(input[i]);
        auto startTimeResult = ParseAsUInt32(input[i + 1]);
        auto timeoutResult = ParseAsUInt32(input[i + 2]);

        if(programIdResult.has_error() or startTimeResult.has_error() or timeoutResult.has_error())
        {
            PRINTF(
                "Invalid input for EDU program entry: programId = %s, startTime = %s, "
                "timeout = %s\n\n",
                input[i].c_str(),
                input[i + 1].c_str(),
                input[i + 2].c_str());
            return;
        }
        if(edu::programQueue.IsFull())
        {
            PRINTF("EDU ProgramQueue full, can't add more!\n\n");
            return;
        }

        auto entry = edu::ProgramQueueEntry{ProgramId(programIdResult.value()),
                                            RealTime(startTimeResult.value()),
                                            static_cast<std::int16_t>(timeoutResult.value())};
        PRINTF("Added program: program ID = %i, start time = %u, timeout = %i s\n",
               value_of(entry.programId),
               static_cast<unsigned>(value_of(entry.startTime)),
               entry.timeout);
        edu::programQueue.PushBack(entry);
    }
}


auto HandleResetCommand(std::span<Token const> input) -> void
{
    auto const & secondaryCommandName = input.front();
    if(secondaryCommandName == "var")
    {
        ResetAllVariables();
        return;
    }
    if(secondaryCommandName == "queue")
    {
        edu::programQueue.Clear();
        PRINTF("Cleared EDU Queue\n");
        return;
    }
    if(secondaryCommandName == "history")
    {
        auto historySection = framSections.Get<"eduProgramStatusHistory">();
        auto resetData = std::array<Byte const, value_of(historySection.size)>{};
        fram::WriteTo(historySection.begin, Span(resetData), framTimeout);

        PRINTF("Cleared EDU History\n");
        return;
    }
    HandleInvalidInput();
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
    PrintVariable("maxEduIdleDuration");
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
    persistentVariables.Store<"fileTransferWindowEnd">(RodosTime(0));
    persistentVariables.Store<"antennasShouldBeDeployed">(false);
    persistentVariables.Store<"realTime">(RealTime(0));
    persistentVariables.Store<"realTimeOffset">(Duration(0));
    persistentVariables.Store<"realTimeOffsetCorrection">(Duration(0));
    persistentVariables.Store<"nFirmwareChecksumErrors">(0);
    persistentVariables.Store<"epsIsWorking">(false);
    persistentVariables.Store<"flashIsWorking">(false);
    persistentVariables.Store<"rfIsWorking">(false);
    persistentVariables.Store<"nFlashErrors">(0);
    persistentVariables.Store<"nRfErrors">(0);
    persistentVariables.Store<"nFileSystemErrors">(0);
    // EDU
    persistentVariables.Store<"eduShouldBePowered">(false);
    persistentVariables.Store<"maxEduIdleDuration">(Duration(0));
    persistentVariables.Store<"newEduResultIsAvailable">(false);
    persistentVariables.Store<"eduProgramQueueIndex">(0);
    persistentVariables.Store<"nEduCommunicationErrors">(0);
    // Communication
    persistentVariables.Store<"nCorrectableUplinkErrors">(0);
    persistentVariables.Store<"nUncorrectableUplinkErrors">(0);
    persistentVariables.Store<"nGoodTransferFrames">(0);
    persistentVariables.Store<"nBadTransferFrames">(0);
    persistentVariables.Store<"lastFrameSequenceNumber">(0);
    persistentVariables.Store<"lastMessageTypeId">(MessageTypeIdFields{});
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
        etl::to_string(value_of(persistentVariables.Load<"fileTransferWindowEnd">()), value);
    }
    else if(variable == "antennasShouldBeDeployed")
    {
        etl::to_string(persistentVariables.Load<"antennasShouldBeDeployed">(), value);
    }
    else if(variable == "realTime")
    {
        etl::to_string(value_of(persistentVariables.Load<"realTime">()), value);
    }
    else if(variable == "realTimeOffset")
    {
        etl::to_string(value_of(persistentVariables.Load<"realTimeOffset">()), value);
    }
    else if(variable == "realTimeOffsetCorrection")
    {
        etl::to_string(value_of(persistentVariables.Load<"realTimeOffsetCorrection">()), value);
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
    else if(variable == "maxEduIdleDuration")
    {
        etl::to_string(value_of(persistentVariables.Load<"maxEduIdleDuration">()), value);
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
        auto messageTypeId = persistentVariables.Load<"lastMessageTypeId">();
        etl::to_string(messageTypeId.serviceTypeId, value);
        value += ",";
        etl::to_string(messageTypeId.messageSubtypeId, value, /*append=*/true);
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
    else if(variable == "maxEduIdleDuration")
    {
        WriteAndConvertFunction<"maxEduIdleDuration", Duration, ParseAsUInt32>(variable, value);
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
        WriteAndConvertFunction<"lastMessageTypeId",
                                MessageTypeIdFields,
                                ParseAsMessageTypeIdFields>(variable, value);
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
    }
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
        PRINTF("%s <- %s\n", Message(variable).c_str(), Message(value).c_str());
    }
}


auto ParseAsUInt32(etl::string_view string) -> Result<std::uint32_t>  // NOLINT(*value-param)
{
    std::uint32_t value = 0;
    auto result = std::from_chars(string.data(), string.data() + string.size(), value);
    if(result.ec != std::errc{})
    {
        return ErrorCode::invalidParameter;
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


// NOLINTNEXTLINE(*value-param)
auto ParseAsMessageTypeIdFields(etl::string_view string) -> Result<MessageTypeIdFields>
{
    auto messageTypeId = MessageTypeIdFields{};
    auto result =
        std::from_chars(string.data(), string.data() + string.size(), messageTypeId.serviceTypeId);
    if(result.ec != std::errc{} or (*result.ptr != ',' and *result.ptr != '.'))
    {
        return ErrorCode::invalidParameter;
    }
    result = std::from_chars(result.ptr + 1,  // NOLINT(*pointer-arithmetic)
                             string.data() + string.size(),
                             messageTypeId.messageSubtypeId);
    if(result.ec != std::errc{})
    {
        return ErrorCode::invalidParameter;
    }
    return messageTypeId;
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


auto ToCZString(edu::ProgramStatus status) -> char const *
{
    switch(status)
    {
        case edu::ProgramStatus::programRunning:
            return "programRunning";
        case edu::ProgramStatus::programCouldNotBeStarted:
            return "programCouldNotBeStarted";
        case edu::ProgramStatus::programExecutionFailed:
            return "programExecutionFailed";
        case edu::ProgramStatus::programExecutionSucceeded:
            return "programExecutionSucceeded";
        case edu::ProgramStatus::resultStoredInFileSystem:
            return "resultStoredInFileSystem";
        case edu::ProgramStatus::resultRequestedByGround:
            return "resultRequestedByGround";
        case edu::ProgramStatus::resultAcknowledgedByGround:
            return "resultAcknowledgedByGround";
        case edu::ProgramStatus::resultDeleted:
            return "resultDeleted";
    }
    return "unknown state";
}
}
}
