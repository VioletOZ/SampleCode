#pragma once
#include "stdafx.h"
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>

static uint32_t GetRandomNumber(uint32_t min, uint32_t max) {
    // 범위가 유효한지 확인
    if (min >= max) {
        return min; 
    }

    random::random_device seed;
    random::mt19937 gen(seed());
    random::uniform_int_distribution<uint32_t> distr(min, max - 1);
    variate_generator<random::mt19937, random::uniform_int_distribution<uint32_t>> rand(gen, distr);

    return rand(); 
}