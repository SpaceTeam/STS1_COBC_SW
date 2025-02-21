#pragma once


#include <Tests/CatchRodos/TestReporter.hpp>
#include <Tests/CatchRodos/Vocabulary.hpp>

#include <etl/vector.h>


namespace sts1cobcsw
{
class TestRegistry
{
public:
    static auto GetInstance() -> TestRegistry &
    {
        static TestRegistry instance;
        return instance;
    }


    auto RegisterTestInit(void (*function)(), gsl::czstring name) -> void
    {
        if(not testInits_.full())
        {
            testInits_.push_back({function, name});
        }
    }


    auto RegisterTestCase(void (*function)(), gsl::czstring name) -> void
    {
        if(not testCases_.full())
        {
            testCases_.push_back({function, name});
        }
    }


    auto RunTestInits() -> void
    {
        for(auto const & testInit : testInits_)
        {
            TestReporter::GetInstance().AddTestInit(testInit.name);
            testInit.function();
        }
    }


    auto RunTestCases() -> void
    {
        for(auto const & testCase : testCases_)
        {
            TestReporter::GetInstance().AddTestCase(testCase.name);
            testCase.function();
        }
    }


private:
    struct NamedFunction
    {
        void (*function)();
        gsl::czstring name;
    };

    etl::vector<NamedFunction, maxNTestCases> testInits_;
    etl::vector<NamedFunction, maxNTestCases> testCases_;
};


struct TestInitRegistrar
{
    TestInitRegistrar(void (*testInitFunction)(), gsl::czstring testInitName)
    {
        TestRegistry::GetInstance().RegisterTestInit(testInitFunction, testInitName);
    }
};


struct TestCaseRegistrar
{
    TestCaseRegistrar(void (*testCaseFunction)(), gsl::czstring testCaseName)
    {
        TestRegistry::GetInstance().RegisterTestCase(testCaseFunction, testCaseName);
    }
};
}
