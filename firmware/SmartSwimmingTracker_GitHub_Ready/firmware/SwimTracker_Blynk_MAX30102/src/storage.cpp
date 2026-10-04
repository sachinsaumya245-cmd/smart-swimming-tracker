#include "storage.h"
#include "config.h"
#include <Preferences.h>

static Preferences p;

void Storage::begin() { p.begin("swim", true); p.end(); }

int   Storage::getSPL()  { p.begin("swim",true);  int   v=p.getInt("spl", Config::DEFAULT_SPL);  p.end(); return v; }
void  Storage::setSPL(int v)  { p.begin("swim",false); p.putInt("spl",v);  p.end(); }
float Storage::getPool() { p.begin("swim",true);  float v=p.getFloat("pool",Config::DEFAULT_POOL_M); p.end(); return v; }
void  Storage::setPool(float v) { p.begin("swim",false); p.putFloat("pool",v); p.end(); }

CalibrationData Storage::loadCal() {
    CalibrationData c;
    p.begin("swim", true);
    c.strokeThreshold = p.getFloat("accth", 1.5f);
    c.gyroThreshold   = p.getFloat("gyroth", 60.0f);
    c.minDurMs        = p.getULong("mindur", 150);
    c.maxDurMs        = p.getULong("maxdur", 1200);
    p.end();
    return c;
}

void Storage::saveCal(const CalibrationData& c) {
    p.begin("swim", false);
    p.putFloat("accth",  c.strokeThreshold);
    p.putFloat("gyroth", c.gyroThreshold);
    p.putULong("mindur", c.minDurMs);
    p.putULong("maxdur", c.maxDurMs);
    p.end();
}
