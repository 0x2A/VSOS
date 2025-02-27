#pragma once

#include <stdint.h>

/**
* @struct Time
* @brief Stores the year, month, day, hour, minute and second of a time.
*/
struct Time {

    uint16_t year;
    uint8_t month;
    uint8_t day;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    bool is_leap_year() const {
        return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    }
};