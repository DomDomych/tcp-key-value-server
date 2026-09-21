#include "protocol/command_processor.hpp"
#include "storage/storage.hpp"
#include <string>

std::string process(const Request &req, Storage &storage)
{

    if (req.command.empty())
        return "ERROR!\n";

    if (req.command == "SET")
    {

        if (req.key.empty())
        {
            return "NO KEY\n";
        }
        if (req.value.empty())
        {
            return "NO VALUE\n";
        }
        if(storage.set(req.key, req.value)==false)
        {
            return "ERROR\n";
        }
        return "OK\n";
    }

    else if (req.command == "GET")
    {
        if (req.key.empty())
        {
            return "NO KEY\n";
        }

        auto value = storage.get(req.key);
        if (!value)
        {
            return "NO SUCH KEY\n";
        }
        else
            return *value + '\n';
    }

    else if (req.command == "DEL")
    {
        if (req.key.empty())
        {
            return "NO KEY\n";
        }

        if (storage.del(req.key) == false)
        {
            return "NO SUCH KEY\n";
        }
        else
            return "OK\n";
    }

    else
    {
        return "ERROR\n";
    }
}