#ifndef STUFF_H
#define STUFF_H

#include "receiver.h"
#include <mutex>
#include <unordered_map>
#include <string>

#define STREAM_NAME_LEN 8

extern std::mutex topbl_mutex;
extern std::unordered_map<std::string, receiver::sptr> receiver_map;

void *ws_loop(void *);

#endif
