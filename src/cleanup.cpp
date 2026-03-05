#include "../headers/cleanup.hpp"
#include "../headers/odbc_db.hpp"
#include "../headers/logger.hpp"
#include <chrono>
#include <iostream>

CleanupJob::CleanupJob(OdbcDb& db, int intervalSeconds)
    : m_db(db), m_interval(intervalSeconds), m_running(false) {
}

CleanupJob::~CleanupJob() { stop(); }

void CleanupJob::start() {
    m_running = true;
    m_thread = std::thread(&CleanupJob::run, this);
}

void CleanupJob::stop() {
    m_running = false;
    m_cv.notify_one();
    if (m_thread.joinable()) m_thread.join();
}

void CleanupJob::run() {
    while (m_running) {
        try {
            Logger::log(Logger::Level::L_DEBUG, "Releasing");
            int n = m_db.release_expired_reservations();
			if (n > 0) Logger::log(Logger::Level::L_INFO, "Released " + std::to_string(n) + " expired reservations");
        }
        catch (const std::exception& e) {
			Logger::log(Logger::Level::L_ERROR, std::string("Cleanup error: ") + e.what());
        }

        std::unique_lock<std::mutex> lk(m_mutex);
        m_cv.wait_for(lk, std::chrono::seconds(m_interval),
            [this] { return !m_running.load(); });
    }
}
