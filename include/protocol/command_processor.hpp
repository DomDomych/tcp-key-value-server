#pragma once

#include "request.hpp"
#include <storage/storage.hpp>
#include <string>

std::string process(const Request &req, Storage &storage);