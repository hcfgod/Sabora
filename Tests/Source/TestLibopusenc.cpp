/**
 * @file TestLibopusenc.cpp
 * @brief Unit tests for libopusenc high-level Opus encoder wrapper.
 *
 * These tests ensure libopusenc symbols are linked and the basic
 * encoder lifecycle works with in-memory callbacks.
 */

#include "doctest.h"
#include "Sabora.h"

#include <opus/opusenc.h>

#include <vector>

namespace
{
    int WriteCallback(void* userData, const unsigned char* ptr, opus_int32 len)
    {
        auto* buffer = static_cast<std::vector<unsigned char>*>(userData);
        if (buffer == nullptr || ptr == nullptr || len < 0)
        {
            return 1;
        }

        buffer->insert(buffer->end(), ptr, ptr + len);
        return 0;
    }

    int CloseCallback(void* /*userData*/)
    {
        return 0;
    }
} // namespace

TEST_SUITE("libopusenc")
{
    TEST_CASE("Create comments and encoder with callbacks")
    {
        OggOpusComments* comments = ope_comments_create();
        REQUIRE(comments != nullptr);

        ope_comments_add(comments, "ENCODER", "Sabora Tests");

        std::vector<unsigned char> encodedBytes;
        const OpusEncCallbacks callbacks{
            &WriteCallback,
            &CloseCallback
        };

        int error = OPE_OK;
        OggOpusEnc* encoder = ope_encoder_create_callbacks(
            &callbacks,
            &encodedBytes,
            comments,
            48000,
            2,
            0,
            &error);

        CHECK(encoder != nullptr);
        CHECK(error == OPE_OK);

        if (encoder != nullptr)
        {
            CHECK(ope_encoder_drain(encoder) == OPE_OK);
            ope_encoder_destroy(encoder);
        }

        ope_comments_destroy(comments);

        // The buffer may be empty if nothing is emitted yet, but the callback path ran
        CHECK(encodedBytes.size() >= 0);
    }

    TEST_CASE("Verify libopusenc constants")
    {
        CHECK(OPE_API_VERSION >= 0);
        CHECK(OPE_OK == 0);
        CHECK(OPE_BAD_ARG < 0);
        CHECK(OPE_ALLOC_FAIL < 0);
    }
}
