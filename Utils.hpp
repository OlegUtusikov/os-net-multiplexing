#pragma once

#include <vector>
#include <string_view>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sstream>
#include <chrono>

#define LOGI(format, ...) Utils::print_log("INFO", __FILE__, __FUNCTION__, __LINE__, std::cerr, format, ##__VA_ARGS__)
#define LOGE(format, ...) Utils::print_log("ERROR", __FILE__, __FUNCTION__, __LINE__, std::cerr, format, ##__VA_ARGS__)

namespace Utils {

__attribute__((format (printf, 6, 7)))
void print_log(const char *type, const char *file, const char *func, int line, std::ostream &stream,
               const char *format, ...);

std::vector<std::pair<std::string, int>> lookup_host(std::string_view const &host);
std::string vector_to_str(std::vector<std::pair<std::string, int>> const &v);
void prepare(std::string &request);
std::size_t get_current_time();

}   // namespace Utils