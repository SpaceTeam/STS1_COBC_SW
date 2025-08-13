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

#include <rodos_no_using_namespace.h>

#include <etl/list.h>
#include <etl/string.h>
#include <etl/to_string.h>

#include <array>
#include <cstdint>
#include <iterator>
#include <new>
#include <type_traits>


namespace sts1cobcsw
{
using RODOS::PRINTF;


namespace
{
constexpr auto maxMessageLength = 50U;
constexpr auto maxWords = 10U;
constexpr auto maxWordLength = 15U;
constexpr auto maxValueLength = 11U;
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


class FramTool : public RODOS::StaticThread<>
{
public:
    FramTool() : StaticThread("FramTool")
    {}


private:
    void init() override
    {
        auto const baudRate = 115'200;
        hal::Initialize(&uart, baudRate);
    }


    void run() override
    {
        PRINTF("\nFram Tool\n\n");
        PrintHelpMessage();
        fram::Initialize();  // This is required for the persistent variables to work

        auto nextChar = std::array<Byte, 1>{};
        auto inputWords = etl::list<etl::string<maxWordLength>, maxWords>{};

        while(true)
        {
            // readData
            inputWords.clear();
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
} framTool;


auto PrintAllVariables() -> void
{
    PRINTF("\nToDo: Print all\n\n");
    auto variable = etl::string<maxMessageLength>{};

    PrintVariable(variable = "nTotalResets");
    PrintVariable(variable = "nResetsSinceRf");
    PrintVariable(variable = "activeSecondaryFwPartition");
}


auto SetAllVariables() -> void
{
    PRINTF("\nToDo: Set all\n\n");
    persistentVariables.Store<"nTotalResets">(0);
    persistentVariables.Store<"nResetsSinceRf">(0);
    persistentVariables.Store<"activeSecondaryFwPartition">(sts1cobcsw::fw::PartitionId::primary);

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
    else if(variable == "txIsOn")
    {
        etl::to_string(persistentVariables.Load<"txIsOn">(), value);
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
    else if(variable == "txIsOn")
    {
        WriteAndConvertFunction<"txIsOn", bool, ParseToBool>(variable, value);
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
    PRINTF("\nInvalid message received\n");
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
    if(string == "secondary2" || string == "1")
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
