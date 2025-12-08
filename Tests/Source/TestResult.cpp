/**
 * @file TestResult.cpp
 * @brief Unit tests for Result type functionality.
 */

#include "doctest.h"
#include "Core/Result.h"

using namespace Sabora;

TEST_SUITE("Result")
{
    TEST_CASE("Success - Creates successful result")
    {
        auto result = Result<int>::Success(42);
        CHECK(result.IsSuccess());
        CHECK_FALSE(result.IsFailure());
        CHECK(result.Value() == 42);
    }

    TEST_CASE("Failure - Creates failed result")
    {
        auto result = Result<int>::Failure(ErrorCode::FileNotFound, "File not found");
        CHECK_FALSE(result.IsSuccess());
        CHECK(result.IsFailure());
        CHECK(result.GetError().Code() == ErrorCode::FileNotFound);
        CHECK(result.GetError().Message() == "File not found");
    }

    TEST_CASE("ValueOr - Returns value on success")
    {
        auto result = Result<int>::Success(42);
        CHECK(result.ValueOr(0) == 42);
    }

    TEST_CASE("ValueOr - Returns default on failure")
    {
        auto result = Result<int>::Failure(ErrorCode::FileNotFound);
        CHECK(result.ValueOr(0) == 0);
    }

    TEST_CASE("Map - Transforms success value")
    {
        auto result = Result<int>::Success(21);
        auto mapped = result.Map([](const int& val) { return val * 2; });
        
        REQUIRE(mapped.IsSuccess());
        CHECK(mapped.Value() == 42);
    }

    TEST_CASE("Map - Propagates error")
    {
        auto result = Result<int>::Failure(ErrorCode::FileNotFound);
        auto mapped = result.Map([](const int& val) { return val * 2; });
        
        CHECK(mapped.IsFailure());
        CHECK(mapped.GetError().Code() == ErrorCode::FileNotFound);
    }

    TEST_CASE("AndThen - Chains operations")
    {
        auto result = Result<int>::Success(21);
        auto chained = result.AndThen([](const int& val) {
            if (val > 0) {
                return Result<int>::Success(val * 2);
            }
            return Result<int>::Failure(ErrorCode::CoreInvalidArgument);
        });
        
        REQUIRE(chained.IsSuccess());
        CHECK(chained.Value() == 42);
    }

    TEST_CASE("AndThen - Propagates error in chain")
    {
        auto result = Result<int>::Failure(ErrorCode::FileNotFound);
        auto chained = result.AndThen([](const int& val) {
            return Result<int>::Success(val * 2);
        });
        
        CHECK(chained.IsFailure());
        CHECK(chained.GetError().Code() == ErrorCode::FileNotFound);
    }

    TEST_CASE("OrElse - Provides recovery on failure")
    {
        auto result = Result<int>::Failure(ErrorCode::FileNotFound);
        auto recovered = result.OrElse([](const Error& err) {
            return Result<int>::Success(0); // Default value
        });
        
        REQUIRE(recovered.IsSuccess());
        CHECK(recovered.Value() == 0);
    }

    TEST_CASE("OrElse - Keeps success value")
    {
        auto result = Result<int>::Success(42);
        auto recovered = result.OrElse([](const Error& err) {
            return Result<int>::Success(0);
        });
        
        REQUIRE(recovered.IsSuccess());
        CHECK(recovered.Value() == 42);
    }

    TEST_CASE("Result<void> - Success case")
    {
        auto result = Result<void>::Success();
        CHECK(result.IsSuccess());
        CHECK_FALSE(result.IsFailure());
    }

    TEST_CASE("Result<void> - Failure case")
    {
        auto result = Result<void>::Failure(ErrorCode::CoreInitializationFailed);
        CHECK_FALSE(result.IsSuccess());
        CHECK(result.IsFailure());
        CHECK(result.GetError().Code() == ErrorCode::CoreInitializationFailed);
    }

    TEST_CASE("Error - Category and code name")
    {
        Error error(ErrorCode::FileNotFound, "Test error");
        CHECK(error.Category() == ErrorCategory::FileSystem);
        CHECK(error.CodeName() != nullptr);
        CHECK(error.CategoryName() != nullptr);
    }

    TEST_CASE("Error - Inner error wrapping")
    {
        Error innerError(ErrorCode::FileNotFound, "Inner error");
        Error outerError(ErrorCode::CoreInitializationFailed, "Outer error", 
                        std::make_shared<Error>(innerError));
        
        CHECK(outerError.HasInnerError());
        CHECK(outerError.InnerError()->Code() == ErrorCode::FileNotFound);
    }
}

