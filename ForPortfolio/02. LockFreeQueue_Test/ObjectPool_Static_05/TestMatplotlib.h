#pragma once
#include <cmath>
#include "matplotlibcpp.h"

#define PI 3.14
namespace plt = matplotlibcpp;
void TestMatplotlib()
{
    int n = 5000;
    std::vector<double> x(n), y(n), z(n), w(n, 2);
    for (int i = 0; i < n; ++i) {
        x.at(i) = i * i;
        y.at(i) = sin(2 * PI * i / 360.0);
        z.at(i) = log(i);
    }

    plt::figure_size(1200, 780);
    plt::plot(x, y);
    plt::plot(x, w, "r--");
    plt::named_plot("log(x)", x, z);
    plt::xlim(0, 1000 * 1000);
    plt::title("Sample figure");
    plt::legend();
    plt::show("./basic.png");
}