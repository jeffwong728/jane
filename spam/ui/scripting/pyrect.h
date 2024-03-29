#ifndef SPAM_UI_SCRIPTING_PYRECT_H
#define SPAM_UI_SCRIPTING_PYRECT_H

#include "pydrawable.h"
#include <ui/projs/modelfwd.h>

struct PyRect : public PyDrawable
{
    PyRect();
    void SetX(const double x);
    void SetY(const double y);
    std::string toString() const;
    double GetX() const;
    double GetY() const;
    double GetWidth() const;
    double GetHeight() const;
    void Translate(const double delta_x, const double delta_y);
    void Rotate(const double angle, const bool angle_as_degree=true);
};

#endif //SPAM_UI_SCRIPTING_PYRECT_H
