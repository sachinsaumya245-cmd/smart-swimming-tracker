#pragma once
#include "types.h"
#include "filter.h"
#include "config.h"

// ═══════════════════════════════════════════════════════════════
//  SwimTracker – replaces hundreds of globals with one class.
//  Contains the stroke-detection state machine (feature 7),
//  session statistics (8), style detection (13), auto-SPL (14),
//  cadence analytics (19), and speed estimation (20).
// ═══════════════════════════════════════════════════════════════

class SwimTracker {
public:
    // ── Lifecycle ────────────────────────────────────────────
    void reset();
    void update(const SensorData& d, unsigned long nowMs);
    void computeFinals();                       // fill derived stats

    // ── Accessors ────────────────────────────────────────────
    const SessionStats&    stats()    const { return s_; }
    const CadenceStats&    cadence()  const { return cad_; }
    CalibrationData&       cal()            { return cal_; }
    const CalibrationData& cal()      const { return cal_; }
    bool  resting()  const { return resting_; }
    void  startSession(unsigned long now);
    unsigned long restDurationMs(unsigned long now) const;

    // ── Settings (persisted externally by main) ──────────────
    int   spl         = Config::DEFAULT_SPL;
    float poolM       = Config::DEFAULT_POOL_M;

private:
    // ── Peak state machine (feature 7) ───────────────────────
    void onWaiting(float dev, float threshold, unsigned long now);
    void onRising(float smoothA, float smoothG, float dev,
                  float threshold, unsigned long now);

    // ── Stroke sub-systems ───────────────────────────────────
    void registerStroke(unsigned long now);
    void registerTurn(unsigned long now);
    float strokeRate() const;

    // ── Style detection (feature 13) ─────────────────────────
    void accumStyle(const SensorData& d);
    SwimStyle classifyStyle();

    // ── Auto-SPL (feature 14) ────────────────────────────────
    void learnSPL();

    // ── Cadence (feature 19) ─────────────────────────────────
    void updateCadence();

    // ── Rest ─────────────────────────────────────────────────
    void checkRest(unsigned long now);

    // ── Data ─────────────────────────────────────────────────
    SessionStats       s_;
    CalibrationData    cal_;
    CadenceStats       cad_;
    PeakData           pk_;
    StyleAccum         stAcc_;

    MovingAverageFilter fltA_, fltG_;

    // Stroke timing ring buffer
    unsigned long stTimes_[Config::RATE_WINDOW] = {};
    uint8_t       stIdx_   = 0;
    uint8_t       stFill_  = 0;
    unsigned long lastStr_  = 0;
    unsigned long lastAct_  = 0;
    unsigned long lastTurn_ = 0;   // lap cooldown timer

    // Rest
    bool          resting_ = false;
    unsigned long restMs_  = 0;

    // Cadence intervals ring
    unsigned long intervals_[Config::RATE_WINDOW] = {};
    uint8_t       ivIdx_  = 0;
    uint8_t       ivFill_ = 0;

    // Style voting ring
    SwimStyle styleVotes_[Config::STYLE_VOTE_WINDOW] = {};
    uint8_t   svIdx_  = 0;
    uint8_t   svFill_ = 0;

    // Auto-SPL lap history
    int     lapStrokes_[10] = {};
    uint8_t lsIdx_  = 0;
    uint8_t lsFill_ = 0;
};
