#pragma once
#include "types.h"

class Storage {
public:
    void begin();

    int   getSPL();
    void  setSPL(int v);
    float getPool();
    void  setPool(float v);

    CalibrationData loadCal();
    void            saveCal(const CalibrationData& c);
};
