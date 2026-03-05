#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

class OdbcDb;

class CleanupJob {
public:
    CleanupJob(OdbcDb& db, int intervalSeconds);
    ~CleanupJob();
    void start();
    void stop();

private:
    void run();
    OdbcDb& m_db;
    int m_interval;
    std::atomic<bool> m_running;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};
