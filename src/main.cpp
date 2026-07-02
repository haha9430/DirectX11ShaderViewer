#include "app/D3DApp.h"

#include <Windows.h>

int wmain()
{
    dxsv::D3DApp app;
    if (!app.initialize(GetModuleHandleW(nullptr), SW_SHOW))
    {
        return -1;
    }

    return app.run();
}
