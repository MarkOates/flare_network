#include <gtest/gtest.h>

#include <flare_network/NetworkMessage.hpp>

TEST(NetworkMessageTest, can_be_created_without_blowing_up)
{
   NetworkMessage network_message;
}

