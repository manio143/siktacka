#pragma once

#include <exception>

class InvalidSizeException : public std::exception {
   public:
    InvalidSizeException() : std::exception() {}
};

class InvalidValueException : public std::exception {
   private:
    const char* msg;

   public:
    InvalidValueException(const char* msg) : std::exception(), msg(msg) {}

    const char* what() const noexcept override { return msg; }
};
