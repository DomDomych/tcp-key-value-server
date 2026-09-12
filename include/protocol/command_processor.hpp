#pragma once

#include "request.hpp"
#include <string>
#include <unordered_map>
#include <storage/storage.hpp>

std::string process(const Request &req, Storage &storage);