#include "delay_step.h"

#include <algorithm>
#include <cmath>

double StepDelay(double currentSeconds, int direction)
{
    constexpr double minimum = 0.1;
    constexpr double firstRegular = 0.5;
    constexpr double step = 0.5;
    constexpr double maximum = 3600.0;
    constexpr double epsilon = 1e-9;

    const double current = std::clamp(currentSeconds, minimum, maximum);
    if (direction > 0)
    {
        if (current < firstRegular - epsilon)
        {
            return firstRegular;
        }

        const double next = (std::floor((current + epsilon) / step) + 1.0) * step;
        return std::min(next, maximum);
    }

    if (direction < 0)
    {
        if (current <= firstRegular + epsilon)
        {
            return minimum;
        }

        const double previous = (std::ceil((current - epsilon) / step) - 1.0) * step;
        return std::max(previous, firstRegular);
    }

    return current;
}

