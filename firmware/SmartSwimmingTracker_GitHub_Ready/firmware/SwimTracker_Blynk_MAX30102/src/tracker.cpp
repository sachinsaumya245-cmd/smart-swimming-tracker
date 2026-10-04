#include "tracker.h"
#include "led.h"
#include <cmath>
#include <cstring>

// ═══════════════════════════════════════════════════════════════
//  Lifecycle
// ═══════════════════════════════════════════════════════════════

void SwimTracker::reset() {
    s_   = {};  pk_  = {};  cad_ = {};  stAcc_.reset();
    fltA_.reset();  fltG_.reset();
    stIdx_ = stFill_ = 0;  lastStr_ = lastAct_ = lastTurn_ = 0;
    resting_ = false; restMs_ = 0;
    ivIdx_ = ivFill_ = 0;
    svIdx_ = svFill_ = 0;
    lsIdx_ = lsFill_ = 0;
    memset(stTimes_, 0, sizeof(stTimes_));
    memset(intervals_, 0, sizeof(intervals_));
    memset(lapStrokes_, 0, sizeof(lapStrokes_));
}

void SwimTracker::startSession(unsigned long now) {
    s_.startMs = now;
    lastAct_   = now;
    lastStr_   = now;
}

unsigned long SwimTracker::restDurationMs(unsigned long now) const {
    return resting_ ? (now - restMs_) : 0;
}

// Called at 50 Hz from the IMU task
void SwimTracker::update(const SensorData& d, unsigned long now) {
    float sA = fltA_.update(d.accelMag);
    float sG = fltG_.update(d.gyroMag);
    float dev = sA - cal_.gravityBaseline;
    float thr = cal_.noiseGate - cal_.gravityBaseline;

    switch (pk_.state) {
        case PeakState::WAITING:
            onWaiting(dev, thr, now);
            break;
        case PeakState::PEAK_RISING:
            accumStyle(d);                      // feature 13
            onRising(sA, sG, dev, thr, now);
            break;
        default: break;
    }
    checkRest(now);
}

// ═══════════════════════════════════════════════════════════════
//  Peak State Machine (feature 7)
// ═══════════════════════════════════════════════════════════════

void SwimTracker::onWaiting(float dev, float thr, unsigned long now) {
    if ((now - pk_.endMs) <= Config::PEAK_REFRACTORY_MS) return;
    if (dev > thr) {
        pk_.state    = PeakState::PEAK_RISING;
        pk_.startMs  = now;
        pk_.accelMax = cal_.gravityBaseline + dev;
        pk_.gyroMax  = 0;
        stAcc_.reset();
        LED::set(true);
    }
}

void SwimTracker::onRising(float sA, float sG, float dev,
                           float thr, unsigned long now) {
    if (sA > pk_.accelMax) pk_.accelMax = sA;
    if (sG > pk_.gyroMax)  pk_.gyroMax  = sG;

    bool settled  = dev < thr * Config::PEAK_EXIT_RATIO;
    bool tooLong  = (now - pk_.startMs) > cal_.maxDurMs;

    if (!settled && !tooLong) return;

    unsigned long dur = now - pk_.startMs;
    float peakDev     = pk_.accelMax - cal_.gravityBaseline;

    // ── Turn? ────────────────────────────────────────────────
    // ALL conditions must be true (AND logic).  The old code
    // used OR (accel OR gyro) which let a single strong stroke
    // trigger a false lap.
    bool turnAccel   = peakDev > Config::TURN_ACCEL_G;
    bool turnGyro    = pk_.gyroMax > Config::TURN_GYRO_DPS;
    bool enoughStr   = s_.strokesThisLap >= Config::MIN_STROKES_FOR_TURN;
    bool lapCooldown = (lastTurn_ == 0) ||
                       (now - lastTurn_) > Config::MIN_LAP_TIME_MS;
    bool isTurn = turnAccel && turnGyro && enoughStr && lapCooldown;

    if (isTurn) {
        registerTurn(now);
    }
    // ── Normal stroke? ───────────────────────────────────────
    else {
        bool aOk = peakDev >= (cal_.strokeThreshold - cal_.gravityBaseline);
        bool gOk = pk_.gyroMax >= cal_.gyroThreshold;
        bool dOk = dur >= cal_.minDurMs && dur <= cal_.maxDurMs;
        bool gap = (now - lastStr_) > Config::MIN_STROKE_GAP_MS;
        if (aOk && gOk && dOk && gap) registerStroke(now);
    }

    pk_.state = PeakState::WAITING;
    pk_.endMs = now;
    LED::set(false);
}

// ═══════════════════════════════════════════════════════════════
//  Stroke / Turn registration
// ═══════════════════════════════════════════════════════════════

void SwimTracker::registerStroke(unsigned long now) {
    s_.strokes++;
    s_.strokesThisLap++;
    lastStr_ = now;

    // Interval for cadence (feature 19)
    if (stFill_ > 0) {
        unsigned long prev = stTimes_[(stIdx_ - 1 + Config::RATE_WINDOW)
                                      % Config::RATE_WINDOW];
        intervals_[ivIdx_] = now - prev;
        ivIdx_ = (ivIdx_ + 1) % Config::RATE_WINDOW;
        if (ivFill_ < Config::RATE_WINDOW) ++ivFill_;
    }

    stTimes_[stIdx_] = now;
    stIdx_ = (stIdx_ + 1) % Config::RATE_WINDOW;
    if (stFill_ < Config::RATE_WINDOW) ++stFill_;

    // Style vote (feature 13)
    SwimStyle vote = classifyStyle();
    styleVotes_[svIdx_] = vote;
    svIdx_ = (svIdx_ + 1) % Config::STYLE_VOTE_WINDOW;
    if (svFill_ < Config::STYLE_VOTE_WINDOW) ++svFill_;

    // Resume from rest
    if (resting_) {
        resting_ = false;
        lastAct_ = now;
    }
    s_.activeMs += (now - lastAct_);
    lastAct_ = now;

    // Lap by stroke count (also respects cooldown)
    bool splCooldown = (lastTurn_ == 0) ||
                       (now - lastTurn_) > Config::MIN_LAP_TIME_MS;
    if (s_.strokesThisLap >= spl && splCooldown) {
        s_.laps++;
        lapStrokes_[lsIdx_] = s_.strokesThisLap;
        lsIdx_ = (lsIdx_ + 1) % 10;
        if (lsFill_ < 10) ++lsFill_;
        s_.strokesThisLap = 0;
        lastTurn_ = now;
        learnSPL();
        LED::blink(2);
    }

    updateCadence();
}

void SwimTracker::registerTurn(unsigned long now) {
    s_.laps++;
    lapStrokes_[lsIdx_] = s_.strokesThisLap;
    lsIdx_ = (lsIdx_ + 1) % 10;
    if (lsFill_ < 10) ++lsFill_;

    int strokesInLap = s_.strokesThisLap;   // capture before reset
    s_.strokesThisLap = 0;
    lastStr_  = now;
    lastTurn_ = now;                        // start lap cooldown
    learnSPL();

    if (resting_) { resting_ = false; lastAct_ = now; }
    s_.activeMs += (now - lastAct_);
    lastAct_ = now;
    LED::blink(4);
}

// ═══════════════════════════════════════════════════════════════
//  Stroke Rate
// ═══════════════════════════════════════════════════════════════

float SwimTracker::strokeRate() const {
    if (stFill_ < 2) return 0;
    int n = (stFill_ < Config::RATE_WINDOW) ? stFill_ : Config::RATE_WINDOW;
    int oldest = (stIdx_ - n + Config::RATE_WINDOW) % Config::RATE_WINDOW;
    unsigned long span =
        stTimes_[(stIdx_ - 1 + Config::RATE_WINDOW) % Config::RATE_WINDOW]
        - stTimes_[oldest];
    return (span == 0) ? 0 : (n - 1) * 60000.0f / span;
}

// ═══════════════════════════════════════════════════════════════
//  Style Detection (feature 13)
// ═══════════════════════════════════════════════════════════════

void SwimTracker::accumStyle(const SensorData& d) {
    stAcc_.sumAbsGx += fabsf(d.gx);
    stAcc_.sumAbsGy += fabsf(d.gy);
    stAcc_.sumAbsGz += fabsf(d.gz);
    stAcc_.sumAz    += d.az;
    if (d.accelMag > stAcc_.peakAccel) stAcc_.peakAccel = d.accelMag;
    stAcc_.samples++;
}

SwimStyle SwimTracker::classifyStyle() {
    if (stAcc_.samples < 3) return SwimStyle::UNKNOWN;
    float total = stAcc_.sumAbsGx + stAcc_.sumAbsGy + stAcc_.sumAbsGz;
    if (total < 1.0f) return SwimStyle::UNKNOWN;

    float rX = stAcc_.sumAbsGx / total;   // roll  (freestyle/backstroke)
    float rY = stAcc_.sumAbsGy / total;   // pitch (breaststroke)
    float avgAz = stAcc_.sumAz / stAcc_.samples;
    float peakRatio = stAcc_.peakAccel / cal_.strokeThreshold;

    // Butterfly: very high accel peak, moderate gyro
    if (peakRatio > Config::STYLE_FLY_ACCEL_FLOOR && rX < 0.55f)
        return SwimStyle::BUTTERFLY;

    // Breaststroke: low total rotation, pitch-dominant
    if (rY > 0.45f || total / stAcc_.samples < Config::STYLE_BREAST_GYRO_CEIL)
        return SwimStyle::BREASTSTROKE;

    // Backstroke vs freestyle: check gravity axis orientation
    if (avgAz < Config::STYLE_BACK_AZ_SIGN)
        return SwimStyle::BACKSTROKE;

    return SwimStyle::FREESTYLE;
}

// ═══════════════════════════════════════════════════════════════
//  Auto-SPL Learning (feature 14)
// ═══════════════════════════════════════════════════════════════

void SwimTracker::learnSPL() {
    if (lsFill_ < Config::SPL_MIN_LAPS) return;
    int sum = 0, n = (lsFill_ < 10) ? lsFill_ : 10;
    for (int i = 0; i < n; ++i) sum += lapStrokes_[i];
    float avg = (float)sum / n;
    spl = (int)(Config::SPL_LEARN_RATE * avg +
                (1.0f - Config::SPL_LEARN_RATE) * spl + 0.5f);
    if (spl < 5) spl = 5;
}

// ═══════════════════════════════════════════════════════════════
//  Cadence Consistency (feature 19)
// ═══════════════════════════════════════════════════════════════

void SwimTracker::updateCadence() {
    if (ivFill_ < 2) return;
    int n = ivFill_;
    float sum = 0;
    for (int i = 0; i < n; ++i) sum += intervals_[i];
    float mean = sum / n;
    float varSum = 0;
    for (int i = 0; i < n; ++i) {
        float d = intervals_[i] - mean;
        varSum += d * d;
    }
    cad_.meanMs      = mean;
    cad_.stdDevMs     = sqrtf(varSum / n);
    cad_.rate         = (mean > 0) ? 60000.0f / mean : 0;
    cad_.consistency  = (mean > 0)
                        ? fmaxf(0, 100.0f * (1.0f - cad_.stdDevMs / mean))
                        : 0;
}

// ═══════════════════════════════════════════════════════════════
//  Rest Detection
// ═══════════════════════════════════════════════════════════════

void SwimTracker::checkRest(unsigned long now) {
    if (resting_) return;
    if (s_.strokes == 0) return;
    if ((now - lastStr_) <= Config::REST_THRESHOLD_MS) return;

    resting_ = true;
    s_.rests++;
    restMs_ = now;
    s_.activeMs += (now - lastAct_);
    lastAct_ = now;
}

// ═══════════════════════════════════════════════════════════════
//  Derived Stats & Export (features 15, 20)
// ═══════════════════════════════════════════════════════════════

void SwimTracker::computeFinals() {
    s_.distance  = s_.laps * poolM +
                   (float(s_.strokesThisLap) / spl) * poolM;
    s_.calories  = s_.strokes * Config::CAL_PER_STROKE;
    s_.avgRate   = (s_.activeMs > 0)
                   ? s_.strokes * 60000.0f / s_.activeMs : 0;
    s_.speedMs   = (s_.activeMs > 0)
                   ? s_.distance / (s_.activeMs / 1000.0f) : 0;
    s_.pace100m  = (s_.distance > 0)
                   ? (s_.activeMs / 1000.0f) / (s_.distance / 100.0f) : 0;

    // Majority-vote style
    int votes[5] = {};
    for (int i = 0; i < svFill_; ++i) votes[(uint8_t)styleVotes_[i]]++;
    int best = 0;
    for (int i = 1; i < 5; ++i) if (votes[i] > votes[best]) best = i;
    s_.style = (SwimStyle)best;
}

