#pragma once
#include "stdafx.h"
#include <random>

static uint32_t GetRandomNumber(uint32_t min, uint32_t max) {
    if (min > max) {
        return min;
    }
    if (min == max) {
        return min;
    }

    // thread_local: 스레드마다 하나씩만 생성, 이후 재사용
    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<uint32_t> distr(min, max);

    return distr(gen);
}

// 첫 자리 1~9, 나머지 0~9인 length자리 숫자 문자열(자동 생성 닉네임 등). 스레드마다 독립된 난수 엔진 사용.
static std::string GenerateNumericString(size_t length) {
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(static_cast<char>('0' + GetRandomNumber(i == 0 ? 1 : 0, 9)));
    }
    return result;
}
