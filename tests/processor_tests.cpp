#include <gtest/gtest.h>

#include "protocol/command_processor.hpp"
#include "protocol/request.hpp"
#include "storage/storage.hpp"

TEST(ProcessorTest, GetCommand)
{

    Storage storage("host=localhost "
                    "port=5432 "
                    "dbname=kv_server "
                    "user=kv_user "
                    "password=1234 ",
                    100);
    storage.set("key", "value");

    Request req{"GET", "key"};

    EXPECT_EQ(process(req, storage), "value\n");
}

TEST(ProcessorTest, SetCommand)
{
    Storage storage("host=localhost "
                    "port=5432 "
                    "dbname=kv_server "
                    "user=kv_user "
                    "password=1234 ",
                    100);
    Request req{"SET", "key", "value"};

    EXPECT_EQ(process(req, storage), "OK\n");

    auto value = storage.get("key");

    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "value");
}

TEST(ProcessorTest, DelCommand)
{
    Storage storage("host=localhost "
                    "port=5432 "
                    "dbname=kv_server "
                    "user=kv_user "
                    "password=1234 ",
                    100);
    storage.set("key", "value");

    Request req{"DEL", "key"};

    EXPECT_EQ(process(req, storage), "OK\n");
    EXPECT_FALSE(storage.get("key").has_value());
}