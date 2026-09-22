#include <vector>
#include <pqxx/pqxx>
#include <memory>
#include <mutex>
#include <condition_variable>

class ConnectionPool;

class ConnectionGuard
{
    public:
        ConnectionGuard(
            ConnectionPool& pool,
            std::unique_ptr<pqxx::connection> connection);

        ~ConnectionGuard();

        ConnectionGuard(const ConnectionGuard&)=delete;
        ConnectionGuard& operator=(const ConnectionGuard&)=delete;

        ConnectionGuard(ConnectionGuard&& other) noexcept;

        pqxx::connection& get();

    private:
        ConnectionPool& pool_;
        std::unique_ptr<pqxx::connection> connection_;
};

class ConnectionPool
{
    public:
        ConnectionPool(
            const std::string& connection_string,
            std::size_t size);
        
        ConnectionGuard acquire();

    private:
        friend class ConnectionGuard;

        void release(std::unique_ptr<pqxx::connection> connection);

        std::vector<std::unique_ptr<pqxx::connection>> available_;

        std::mutex mutex_;
        std::condition_variable cv_;
};