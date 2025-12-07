//
// Created by mateu on 07.12.2025.
//

#ifndef GASM_UTILS_H
#define GASM_UTILS_H

#include <iostream>
#include <iomanip>

inline void printTime(double elapsedSeconds) { // dirty fix of multiple definitions
    int minutes = (int)elapsedSeconds / 60;
    int hours = minutes / 60;
    elapsedSeconds -= minutes * 60;
    minutes -= hours * 60;
    if (hours != 0) std::cout << hours << "h ";
    if (minutes != 0) {
        std::cout << minutes << "m ";
        std::cout << (int)elapsedSeconds << "s";
    } else {
        std::cout << std::fixed
                  << std::setprecision(2)
                  << elapsedSeconds << "s";
    }
}

inline void printProgressBar(int progressPercent, double elapsedSeconds) {
    const int barWidth = 100;
    int filled = progressPercent;

    std::cout << "\r[";
    for (int i = 0; i < filled; ++i) std::cout << "■";
    for (int i = filled; i < barWidth; ++i) std::cout << " ";
    std::cout << "] ";
    printTime(elapsedSeconds);
    std::cout << std::flush;
}

#endif //GASM_UTILS_H
