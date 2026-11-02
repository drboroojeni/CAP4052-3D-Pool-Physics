#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

// Physics uses meters, seconds, and kilograms. Positions lie on the XZ bed;
// the renderer supplies height Y. This is a teaching model without spin,
// airborne motion, curved cushions, or regulation pool rules.
namespace pool {

inline constexpr double kHalfLength = 1.27;
inline constexpr double kHalfWidth = 0.635;
inline constexpr double kBallRadius = 0.028575;
inline constexpr double kBallMass = 0.17;
inline constexpr double kPocketRadius = 0.070;
inline constexpr double kFixedDt = 1.0 / 240.0;
inline constexpr double kStopSpeed = 0.012;

struct Vec2 {
    double x = 0.0, z = 0.0;
    Vec2 operator+(Vec2 b) const { return {x+b.x, z+b.z}; }
    Vec2 operator-(Vec2 b) const { return {x-b.x, z-b.z}; }
    Vec2 operator-() const { return {-x,-z}; }
    Vec2 operator*(double s) const { return {x*s, z*s}; }
    Vec2 operator/(double s) const { return {x/s, z/s}; }
    Vec2& operator+=(Vec2 b) { x+=b.x; z+=b.z; return *this; }
    Vec2& operator-=(Vec2 b) { x-=b.x; z-=b.z; return *this; }
    Vec2& operator*=(double s) { x*=s; z*=s; return *this; }
};
inline Vec2 operator*(double s, Vec2 v) { return v*s; }
inline double dot(Vec2 a, Vec2 b) { return a.x*b.x+a.z*b.z; }
inline double length(Vec2 a) { return std::sqrt(dot(a,a)); }
inline Vec2 normalized(Vec2 a) {
    const double n=length(a);
    return n>1e-12 ? a/n : Vec2{};
}

struct Ball {
    int id = 0;                    // Cue ball is 0, object balls are 1..15.
    Vec2 position{}, velocity{};
    double radius = kBallRadius;
    double mass = kBallMass;
    bool pocketed = false;
    double rollAngle = 0.0;         // Visual rolling distance/radius, in radians.
};

struct Params {
    double rollingDeceleration = 0.22; // m/s^2, constant slowdown on the bed.
    double ballRestitution = 0.96;
    double cushionRestitution = 0.80;
    double maxSpeed = 3.0;            // m/s, also caps cue-shot power.
};

enum class EventType { BallContact, CushionContact, Pocket, Scratch, Strike, Respot };
struct Event {
    EventType type = EventType::BallContact;
    int ballA = -1, ballB = -1;
    Vec2 position{};
    double speed = 0.0;
};

class PhysicsWorld {
public:
    Params params{};
    std::vector<Ball> balls;
    std::uint64_t tick = 0;
    double simulationTime = 0.0;
    std::uint64_t collisions = 0, pockets = 0, scratches = 0;
    std::vector<Event> lastEvents;  // Replaced by events from the latest step.

    PhysicsWorld();
    void resetRack();
    // 0: rack, 1: head-on contact, 2: cushion, 3: pocket, 4: cue scratch.
    // Scenarios start at rest so the student aims and strikes every shot.
    void setupScenario(int index);
    void step(double h);
    bool allStopped() const;
    bool strike(Vec2 direction, double speed);
    bool respotCueBall();
    int objectBallsRemaining() const;
    double kineticEnergy() const;
    Ball* cueBall();
    const Ball* cueBall() const;
    static const std::vector<Vec2>& pocketCenters();

private:
    void clearCounters();
    bool capturePocket(Ball& ball, Vec2 previous);
    void resolveCushions(Ball& ball);
    void resolvePair(Ball& a, Ball& b);
    bool spotIsFree(Vec2 position, double radius) const;
};

} // namespace pool
