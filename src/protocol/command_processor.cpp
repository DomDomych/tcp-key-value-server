#include "protocol/command_processor.hpp"
#include <string>
#include "storage/storage.hpp"

std::string process(const Request &req, Storage& storage)
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
        storage.set(req.key,req.value);
        return "OK\n";
    }

    else if (req.command == "GET")
    {
        if(req.key.empty())
        {
            return "NO KEY\n";
        }

        auto value = storage.get(req.key);
        if(!value)
        {
            return "NO SUCH KEY\n";
        }
        else return *value;
    }

    else if (req.command == "DEL")
    {
        if(req.key.empty())
        {
            return "NO KEY\n";
        }

        if(storage.del(req.key)==false)
        {
            return "NO SUCH KEY\n";
        }
        else return "OK\n";
    }

    else
    {
        return "ERROR\n";
    }
}