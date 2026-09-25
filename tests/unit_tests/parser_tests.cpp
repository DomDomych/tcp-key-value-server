#include <gtest/gtest.h>

#include "protocol/parser.hpp"
#include "protocol/request.hpp"

TEST(ParserTest, ParserGet)
{
    Request req{};

    parse(req, "GET name");

    EXPECT_EQ(req.command, "GET");
    EXPECT_EQ(req.key, "name");
    EXPECT_TRUE(req.value.empty());
}

TEST(ParserTest, ParserSet)
{
    Request req{};

    parse(req, "SET key value");

    EXPECT_EQ(req.command, "SET");
    EXPECT_EQ(req.key, "key");
    EXPECT_EQ(req.value, "value");
}

TEST(ParserTest, ParserDel)
{
    Request req{};

    parse(req, "DEL key");

    EXPECT_EQ(req.command, "DEL");
    EXPECT_EQ(req.key, "key");
    EXPECT_TRUE(req.value.empty());
}

TEST(ParserTest, WrongCommand)
{
    Request req{};

    parse(req, "set key value");
    EXPECT_EQ(req.command, "set");
    EXPECT_EQ(req.key, "key");
    EXPECT_EQ(req.value, "value");
}

TEST(ParserTest, ParseValueWithSpaces)
{
    Request req{};

    parse(req, "SET key value new");
    EXPECT_EQ(req.command, "SET");
    EXPECT_EQ(req.key, "key");
    EXPECT_EQ(req.value, "value new");
}
