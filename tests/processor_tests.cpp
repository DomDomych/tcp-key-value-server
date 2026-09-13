#include <gtest/gtest.h>

#include "storage/storage.hpp"
#include "protocol/command_processor.hpp"
#include "protocol/request.hpp"


TEST(ProcessorTest, GetCommand)
{
    
    Storage storage;
    storage.set("key","value");

    Request req{"GET", "key"};

    EXPECT_EQ(process(req, storage), "value\n");
}

TEST(ProcessorTest, SetCommand)
{

    Request req{"SET", "key", "value"};
    Storage storage;
    storage.set("key","value");

    EXPECT_EQ(process(req, storage), "OK\n");
    ASSERT_NE(storage.get("key"),"value");
}

TEST(ProcessorTest, DelCommand)
{
    Storage storage;
    storage.set("key","value");

    Request req{"DEL", "key"};

    EXPECT_EQ(process(req, storage), "OK\n");
    ASSERT_NE(storage.del("key"),true);
    EXPECT_EQ(storage.get("key"),std::nullopt);
}