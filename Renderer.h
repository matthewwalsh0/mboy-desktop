#ifndef RENDERER_H
#define RENDERER_H

#include <sys/types.h>
#include "DesktopGUI.h"

class Renderer {
public:
    virtual bool draw(DesktopGUI* gui) { return true; }
};

#endif