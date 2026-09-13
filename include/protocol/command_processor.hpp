#pragma once

#include "request.hpp"
#include <storage/storage.hpp>
#include <string>
#include <unordered_map>

std::string process(const Request &req, Storage &storage);