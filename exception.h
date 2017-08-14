#pragma once

#include <exception>

class InvalidSizeException : public std::exception {
   public:
    InvalidSizeException() : std::exception() {}
};