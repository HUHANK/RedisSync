#include <iostream>
#include <signal.h>
#include <cstring>
#include "master_redis_handler.h"
#include "slave_redis_handler.h"
using namespace std;

Reactor gReactor;

void signal_handler(int sig)
{
    gReactor.stop();
}

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGILL, signal_handler);
    
    // init log
    MFShare::log::logger().setDir("./log");
    MFShare::log::logger().setFileName("redis_sync");
    MFShare::log::logger().setMaxFileSize(1024 * 1024 * 100);
    MFShare::log::logger().setAppName("RedisSync");

    MasterRedisHandler masterHandler("192.168.28.233", 6379);
    SlaveRedisHandler slaveHandler("192.168.28.235", 6579);
    masterHandler.setSlaveHandler(&slaveHandler);

    if (!masterHandler.connect() || !slaveHandler.connect())
    {
        LOG_ERROR("Failed to connect redis server");
        return -1;
    }
    
    gReactor.register_handler(&masterHandler);
    gReactor.register_handler(&slaveHandler);
    masterHandler.sendPing();
    
    gReactor.run_forever();
    cout << "Program overed..." << endl;
    return 0;
}