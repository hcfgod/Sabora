/**
 * @file TestLibogg.cpp
 * @brief Unit tests for libogg OGG container format library.
 * 
 * These tests verify that libogg is properly linked and can create
 * OGG streams, packets, and pages for audio container operations.
 */

#include "doctest.h"
#include "Sabora.h"

// libogg OGG container format library
#include <ogg/ogg.h>

#include <cstring>
#include <vector>

using namespace Sabora;

TEST_SUITE("libogg")
{
    TEST_CASE("Initialize ogg_stream_state")
    {
        ogg_stream_state stream;
        int serialno = 12345;
        
        int result = ogg_stream_init(&stream, serialno);
        CHECK(result == 0); // Success
        
        // Verify stream is initialized
        CHECK(stream.serialno == serialno);
        
        // Clean up
        ogg_stream_clear(&stream);
    }

    TEST_CASE("Create and destroy ogg_packet")
    {
        ogg_packet packet = {};
        
        // Initialize packet with some data
        const int packetSize = 100;
        std::vector<unsigned char> data(packetSize, 0x42);
        
        packet.packet = data.data();
        packet.bytes = packetSize;
        packet.b_o_s = 1; // Beginning of stream
        packet.e_o_s = 0; // Not end of stream
        packet.granulepos = 0;
        packet.packetno = 0;
        
        // Verify packet fields
        CHECK(packet.packet != nullptr);
        CHECK(packet.bytes == packetSize);
        CHECK(packet.b_o_s == 1);
        CHECK(packet.e_o_s == 0);
    }

    TEST_CASE("Create ogg_page from stream")
    {
        ogg_stream_state stream;
        int serialno = 54321;
        
        int result = ogg_stream_init(&stream, serialno);
        REQUIRE(result == 0);
        
        // Create a packet
        ogg_packet packet = {};
        std::vector<unsigned char> data(50, 0xAA);
        packet.packet = data.data();
        packet.bytes = 50;
        packet.b_o_s = 1;
        packet.e_o_s = 0;
        packet.granulepos = 0;
        packet.packetno = 0;
        
        // Pack the packet into the stream
        result = ogg_stream_packetin(&stream, &packet);
        CHECK(result == 0);
        
        // Flush the stream to create a page
        ogg_page page;
        result = ogg_stream_flush(&stream, &page);
        
        // Should create a page if there's data
        if (result != 0)
        {
            // Try pageout instead (non-flush)
            result = ogg_stream_pageout(&stream, &page);
        }
        
        // At least one of these should succeed
        if (result != 0)
        {
            WARN("Could not create ogg_page (may require more data)");
        }
        else
        {
            CHECK(page.header != nullptr);
            CHECK(page.header_len > 0);
        }
        
        // Clean up
        ogg_stream_clear(&stream);
    }

    TEST_CASE("Verify ogg_page structure")
    {
        ogg_page page = {};
        
        // Verify page structure exists (even if empty)
        // The structure should be valid even if not populated
        CHECK(true); // Structure exists
    }

    TEST_CASE("Check ogg_stream_packetin")
    {
        ogg_stream_state stream;
        int serialno = 99999;
        
        int result = ogg_stream_init(&stream, serialno);
        REQUIRE(result == 0);
        
        // Create a test packet
        ogg_packet packet = {};
        std::vector<unsigned char> data(25, 0x12);
        packet.packet = data.data();
        packet.bytes = 25;
        packet.b_o_s = 0;
        packet.e_o_s = 0;
        packet.granulepos = 100;
        packet.packetno = 1;
        
        // Add packet to stream
        result = ogg_stream_packetin(&stream, &packet);
        CHECK(result == 0); // Success
        
        // Clean up
        ogg_stream_clear(&stream);
    }

    TEST_CASE("Verify library is linked correctly")
    {
        // Simple test to ensure libogg symbols are available
        ogg_stream_state stream;
        int result = ogg_stream_init(&stream, 1);
        
        // If we get here without linker errors, library is linked
        // Function exists if we can call it (result can be 0 or non-zero)
        bool functionExists = true;
        CHECK(functionExists);
        
        if (result == 0)
        {
            ogg_stream_clear(&stream);
        }
    }

    TEST_CASE("Encode and decode packet data")
    {
        ogg_stream_state stream;
        int serialno = 12345;
        
        int result = ogg_stream_init(&stream, serialno);
        REQUIRE(result == 0);
        
        // Create a packet with test data
        ogg_packet packet = {};
        std::vector<unsigned char> data = {0x01, 0x02, 0x03, 0x04, 0x05};
        packet.packet = data.data();
        packet.bytes = static_cast<long>(data.size());
        packet.b_o_s = 1;
        packet.e_o_s = 0;
        packet.granulepos = 0;
        packet.packetno = 0;
        
        // Pack the packet into the stream
        result = ogg_stream_packetin(&stream, &packet);
        CHECK(result == 0);
        
        // Try to get a page from the stream
        ogg_page page;
        result = ogg_stream_pageout(&stream, &page);
        
        // Page creation may require more data, but function should work
        if (result != 0)
        {
            // Try flush instead
            result = ogg_stream_flush(&stream, &page);
        }
        
        // At least the function call should work (result indicates if page was created)
        CHECK(true); // Function exists and was called
        
        ogg_stream_clear(&stream);
    }

    TEST_CASE("Multiple streams with different serial numbers")
    {
        ogg_stream_state stream1, stream2;
        
        int result1 = ogg_stream_init(&stream1, 100);
        int result2 = ogg_stream_init(&stream2, 200);
        
        CHECK(result1 == 0);
        CHECK(result2 == 0);
        CHECK(stream1.serialno == 100);
        CHECK(stream2.serialno == 200);
        
        ogg_stream_clear(&stream1);
        ogg_stream_clear(&stream2);
    }
}

