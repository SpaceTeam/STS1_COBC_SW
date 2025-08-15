#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/list.h>
#include <etl/string.h>
#include <etl/to_string.h>

#include <array>
#include <cstdint>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>


namespace sts1cobcsw
{
using RODOS::PRINTF;


namespace
{
constexpr auto stackSize = 60'000U;
constexpr auto maxMessageLength = 100U;
constexpr auto maxWords = 10U;
constexpr auto maxWordLength = 30U;
constexpr auto maxValueLength = 16U;
auto uart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


auto PrintAllVariables() -> void;
auto SetAllVariables() -> void;
auto PrintVariable(etl::istring & variable) -> void;
auto SetVariable(etl::istring & variable, etl::istring & value) -> void;
auto PrintHelpMessage() -> void;
auto InvalidMessage() -> void;
template<StringLiteral name, typename T, auto parseFunction>
auto WriteAndConvertFunction(etl::istring & variable, etl::istring & value) -> void;
auto ParseToUint32(etl::istring & string) -> Result<std::uint32_t>;
auto ParseToBool(etl::istring & string) -> Result<bool>;
auto ParseToPartitionId(etl::istring & string) -> Result<fw::PartitionId>;
auto PartitionIdToString(fw::PartitionId id, etl::istring & string) -> void;


class FramExplorer : public RODOS::StaticThread<stackSize>
{
public:
    FramExplorer() : StaticThread("FramExplorer")
    {}


private:
    void init() override
    {
        auto const baudRate = 115'200;
        hal::Initialize(&uart, baudRate);
    }


    void run() override
    {
        PRINTF("\nFRAM explorer\n");
        PrintHelpMessage();
        fram::Initialize();  // This is required for the persistent variables to work

        auto nextChar = std::array<Byte, 1>{};
        auto inputWords = etl::list<etl::string<maxWordLength>, maxWords>{};

        while(true)
        {
            PRINTF("\n\n");
            // readData
            inputWords.clear();
            inputWords.emplace_back();  // Add first word
            for(auto readCharacters = 0U; readCharacters < maxMessageLength; readCharacters++)
            {
                hal::ReadFrom(&uart, Span(&nextChar));
                auto character = static_cast<char>(nextChar[0]);
                if(character == '\n')
                {
                    break;
                }
                if(character == ' ')
                {
                    if(inputWords.full())
                    {
                        break;
                    }
                    inputWords.emplace_back();  // Add a new word
                }
                else
                {
                    inputWords.back().push_back(character);
                }
            }

            // Validate min size
            if(inputWords.size() < 2)
            {
                InvalidMessage();
                continue;
            }

            // Interpret data
            auto const & command = inputWords.front();
            auto it = ++inputWords.begin();  // Iterator to the second element

            if(command == "get")
            {
                if(*it == "--all")
                {
                    PrintAllVariables();
                }
                else
                {
                    for(; it != inputWords.end(); ++it)
                    {
                        PrintVariable(*it);
                    }
                }
            }
            else if(command == "set")
            {
                if(*it == "--all")
                {
                    SetAllVariables();
                }
                else
                {
                    // there should be pairs of two and the initial set word
                    auto remaining = std::distance(it, inputWords.end());
                    if(remaining % 2 != 0)
                    {
                        InvalidMessage();
                    }
                    else
                    {
                        while(it != inputWords.end())
                        {
                            auto & variable = *it++;
                            auto & value = *it++;
                            SetVariable(variable, value);
                        }
                    }
                }
            }
            else
            {
                InvalidMessage();
            }
        }
    }
} framExplorer;


auto PrintAllVariables() -> void
{
    auto variable = etl::string<maxMessageLength>{};

    // Bootloader
    PrintVariable(variable = "nTotalResets");
    PrintVariable(variable = "nResetsSinceRf");
    PrintVariable(variable = "activeSecondaryFwPartition");
    PrintVariable(variable = "backupSecondaryFwPartition");
    // Housekeeping
    PrintVariable(variable = "txIsOn");
    PrintVariable(variable = "fileTransferWindowEnd");
    PrintVariable(variable = "antennasShouldBeDeployed");
    PrintVariable(variable = "realTime");
    PrintVariable(variable = "realTimeOffset");
    PrintVariable(variable = "realTimeOffsetCorrection");
    PrintVariable(variable = "nFirmwareChecksumErrors");
    PrintVariable(variable = "epsIsWorking");
    PrintVariable(variable = "flashIsWorking");
    PrintVariable(variable = "rfIsWorking");
    PrintVariable(variable = "nFlashErrors");
    PrintVariable(variable = "nRfErrors");
    PrintVariable(variable = "nFileSystemErrors");
    // EDU
    PrintVariable(variable = "eduShouldBePowered");
    PrintVariable(variable = "eduStartDelayLimit");
    PrintVariable(variable = "newEduResultIsAvailable");
    PrintVariable(variable = "eduProgramQueueIndex");
    PrintVariable(variable = "nEduCommunicationErrors");
    // Communication
    PrintVariable(variable = "nCorrectableUplinkErrors");
    PrintVariable(variable = "nUncorrectableUplinkErrors");
    PrintVariable(variable = "nGoodTransferFrames");
    PrintVariable(variable = "nBadTransferFrames");
    PrintVariable(variable = "lastFrameSequenceNumber");
    PrintVariable(variable = "lastMessageTypeId");
    PrintVariable(variable = "lastMessageTypeIdWasInvalid");
    PrintVariable(variable = "lastApplicationDataWasInvalid");
}


auto SetAllVariables() -> void
{
    PRINTF("\nToDo: Set all\n\n");
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
    // ToDo: implement MessageTypeId
    // persistentVariables.Store<"lastMessageTypeId">(0);
    persistentVariables.Store<"lastMessageTypeIdWasInvalid">(false);
    persistentVariables.Store<"lastApplicationDataWasInvalid">(false);

    PrintAllVariables();
}


auto PrintHelpMessage() -> void
{
    PRINTF("Write: get --all                to get all variables\n");
    PRINTF("Write: get <variable>           to get one variable\n");
    PRINTF("Write: set <variable> <value>   to write one variable\n");
    PRINTF("Write: set --all                to write all variables to 0\n");
}


auto PrintVariable(etl::istring & variable) -> void
{
    auto value = etl::string<maxValueLength>{};
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
        PartitionIdToString(persistentVariables.Load<"activeSecondaryFwPartition">(), value);
    }
    else if(variable == "backupSecondaryFwPartition")
    {
        PartitionIdToString(persistentVariables.Load<"backupSecondaryFwPartition">(), value);
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
        PRINTF("Variable %s not found!\n", variable.c_str());
        return;
    }

    PRINTF("%s = %s\n", variable.c_str(), value.c_str());
}


auto SetVariable(etl::istring & variable, etl::istring & value) -> void
{
    // Bootloader
    if(variable == "nTotalResets")
    {
        WriteAndConvertFunction<"nTotalResets", std::uint32_t, ParseToUint32>(variable, value);
    }
    else if(variable == "nResetsSinceRf")
    {
        WriteAndConvertFunction<"nResetsSinceRf", std::uint8_t, ParseToUint32>(variable, value);
    }
    else if(variable == "activeSecondaryFwPartition")
    {
        WriteAndConvertFunction<"activeSecondaryFwPartition", fw::PartitionId, ParseToPartitionId>(
            variable, value);
    }
    else if(variable == "backupSecondaryFwPartition")
    {
        WriteAndConvertFunction<"backupSecondaryFwPartition", fw::PartitionId, ParseToPartitionId>(
            variable, value);
    }
    // Housekeeping
    else if(variable == "txIsOn")
    {
        WriteAndConvertFunction<"txIsOn", bool, ParseToBool>(variable, value);
    }
    else if(variable == "fileTransferWindowEnd")
    {
        WriteAndConvertFunction<"fileTransferWindowEnd", RodosTime, ParseToUint32>(variable, value);
    }
    else if(variable == "antennasShouldBeDeployed")
    {
        WriteAndConvertFunction<"antennasShouldBeDeployed", bool, ParseToBool>(variable, value);
    }
    else if(variable == "realTime")
    {
        WriteAndConvertFunction<"realTime", RealTime, ParseToUint32>(variable, value);
    }
    else if(variable == "realTimeOffset")
    {
        WriteAndConvertFunction<"realTimeOffset", Duration, ParseToUint32>(variable, value);
    }
    else if(variable == "realTimeOffsetCorrection")
    {
        WriteAndConvertFunction<"realTimeOffsetCorrection", Duration, ParseToUint32>(variable,
                                                                                     value);
    }
    else if(variable == "nFirmwareChecksumErrors")
    {
        WriteAndConvertFunction<"nFirmwareChecksumErrors", std::uint8_t, ParseToUint32>(variable,
                                                                                        value);
    }
    else if(variable == "epsIsWorking")
    {
        WriteAndConvertFunction<"epsIsWorking", bool, ParseToBool>(variable, value);
    }
    else if(variable == "flashIsWorking")
    {
        WriteAndConvertFunction<"flashIsWorking", bool, ParseToBool>(variable, value);
    }
    else if(variable == "rfIsWorking")
    {
        WriteAndConvertFunction<"rfIsWorking", bool, ParseToBool>(variable, value);
    }
    else if(variable == "nFlashErrors")
    {
        WriteAndConvertFunction<"nFlashErrors", std::uint8_t, ParseToUint32>(variable, value);
    }
    else if(variable == "nRfErrors")
    {
        WriteAndConvertFunction<"nRfErrors", std::uint8_t, ParseToUint32>(variable, value);
    }
    else if(variable == "nFileSystemErrors")
    {
        WriteAndConvertFunction<"nFileSystemErrors", std::uint8_t, ParseToUint32>(variable, value);
    }
    // EDU
    else if(variable == "eduShouldBePowered")
    {
        WriteAndConvertFunction<"eduShouldBePowered", bool, ParseToBool>(variable, value);
    }
    else if(variable == "eduStartDelayLimit")
    {
        WriteAndConvertFunction<"eduStartDelayLimit", Duration, ParseToUint32>(variable, value);
    }
    else if(variable == "newEduResultIsAvailable")
    {
        WriteAndConvertFunction<"newEduResultIsAvailable", bool, ParseToBool>(variable, value);
    }
    else if(variable == "eduProgramQueueIndex")
    {
        WriteAndConvertFunction<"eduProgramQueueIndex", std::uint8_t, ParseToUint32>(variable,
                                                                                     value);
    }
    else if(variable == "nEduCommunicationErrors")
    {
        WriteAndConvertFunction<"nEduCommunicationErrors", std::uint8_t, ParseToUint32>(variable,
                                                                                        value);
    }
    // Communication
    else if(variable == "nCorrectableUplinkErrors")
    {
        WriteAndConvertFunction<"nCorrectableUplinkErrors", std::uint16_t, ParseToUint32>(variable,
                                                                                          value);
    }
    else if(variable == "nUncorrectableUplinkErrors")
    {
        WriteAndConvertFunction<"nUncorrectableUplinkErrors", std::uint16_t, ParseToUint32>(
            variable, value);
    }
    else if(variable == "nGoodTransferFrames")
    {
        WriteAndConvertFunction<"nGoodTransferFrames", std::uint16_t, ParseToUint32>(variable,
                                                                                     value);
    }
    else if(variable == "nBadTransferFrames")
    {
        WriteAndConvertFunction<"nBadTransferFrames", std::uint16_t, ParseToUint32>(variable,
                                                                                    value);
    }
    else if(variable == "lastFrameSequenceNumber")
    {
        WriteAndConvertFunction<"lastFrameSequenceNumber", std::uint8_t, ParseToUint32>(variable,
                                                                                        value);
    }
    else if(variable == "lastMessageTypeId")
    {
        PRINTF("NOT SUPPORTED!\n");
        // ToDo: implement messageType Field
        // WriteAndConvertFunction<"lastMessageTypeId", MessageTypeIdFields,
        // ParseToUint32>(variable, value);
    }
    else if(variable == "lastMessageTypeIdWasInvalid")
    {
        WriteAndConvertFunction<"lastMessageTypeIdWasInvalid", bool, ParseToUint32>(variable,
                                                                                    value);
    }
    else if(variable == "lastApplicationDataWasInvalid")
    {
        WriteAndConvertFunction<"lastApplicationDataWasInvalid", bool, ParseToUint32>(variable,
                                                                                      value);
    }
    else
    {
        PRINTF("Variable %s not found!\n", variable.c_str());
        return;
    }

    PRINTF("%s <- %s\n", variable.c_str(), value.c_str());
}


auto InvalidMessage() -> void
{
    PRINTF("\nInvalid message received\n\n");
    PrintHelpMessage();
}


template<StringLiteral name, typename T, auto parseFunction>
auto WriteAndConvertFunction(etl::istring & variable, etl::istring & value) -> void
{
    auto convertedValueResult = parseFunction(value);
    if(convertedValueResult.has_error())
    {
        PRINTF(
            "Value %s for variable %s could not be converted!\n", value.c_str(), variable.c_str());

        if constexpr(std::is_same_v<decltype(parseFunction), decltype(&ParseToPartitionId)>)
        {
            PRINTF("Use:\n0 or primary\n1 or secondary1\n2 or secondary2\n");
        }
    }
    else
    {
        persistentVariables.Store<name>(static_cast<T>(convertedValueResult.value()));
    }
}


auto ParseToUint32(etl::istring & string) -> Result<std::uint32_t>
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


auto ParseToBool(etl::istring & string) -> Result<bool>
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


auto ParseToPartitionId(etl::istring & string) -> Result<fw::PartitionId>
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


auto PartitionIdToString(fw::PartitionId id, etl::istring & string) -> void
{
    switch(id)
    {
        case fw::PartitionId::primary:
            string = "primary";
            break;
        case fw::PartitionId::secondary1:
            string = "secondary1";
            break;
        case fw::PartitionId::secondary2:
            string = "secondary2";
            break;
    }
}
}
}
