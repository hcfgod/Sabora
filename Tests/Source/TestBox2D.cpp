/**
 * @file TestBox2D.cpp
 * @brief Smoke test to validate Box2D (static) integration.
 */

#include "doctest.h"
#include <box2d/box2d.h>

TEST_SUITE("Box2D")
{
    TEST_CASE("Create world and step")
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = b2Vec2{ 0.0f, -9.8f };

        const b2WorldId world = b2CreateWorld(&worldDef);
        REQUIRE(b2World_IsValid(world));

        b2World_Step(world, 1.0f / 60.0f, 4);

        b2DestroyWorld(world);
    }
}