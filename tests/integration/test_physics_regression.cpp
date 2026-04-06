#include "../catch_amalgamated.hpp"

#include <vector>
#include <cmath>
#include <array>

TEST_CASE("Physics Fixed Timestep - Accumulator", "[physics][regression]")
{
    float accumulator = 0.0f;
    float fixedDt = 1.0f / 60.0f;
    float frameTime = 1.0f / 120.0f;
    
    accumulator += frameTime;
    
    bool shouldStep = false;
    while (accumulator >= fixedDt) {
        accumulator -= fixedDt;
        shouldStep = true;
    }
    
    REQUIRE(shouldStep == true);
    REQUIRE(accumulator >= 0.0f);
    REQUIRE(accumulator < fixedDt);
}

TEST_CASE("Physics Fixed Timestep - Multiple Frames", "[physics][regression]")
{
    float accumulator = 0.0f;
    float fixedDt = 1.0f / 60.0f;
    int steps = 0;
    
    for (int i = 0; i < 180; ++i) {
        accumulator += fixedDt;
        while (accumulator >= fixedDt) {
            accumulator -= fixedDt;
            steps++;
        }
    }
    
    REQUIRE(steps == 180);
}

TEST_CASE("Physics Determinism - Gravity Calculation", "[physics][regression]")
{
    float gravity = -9.81f;
    float dt = 1.0f / 60.0f;
    
    float position = 10.0f;
    float velocity = 0.0f;
    
    for (int i = 0; i < 60; ++i) {
        velocity += gravity * dt;
        position += velocity * dt;
    }
    
    float expectedPosition = 10.0f + (0.5f * gravity * 60.0f * 60.0f * dt * dt);
    
    REQUIRE(std::abs(position - expectedPosition) < 0.01f);
}

TEST_CASE("Physics Determinism - Verlet Integration", "[physics][regression]")
{
    struct Particle {
        float x, y;
        float oldX, oldY;
    };
    
    std::vector<Particle> particles(3);
    float dt = 1.0f / 60.0f;
    float gravity = -9.81f;
    float damping = 0.99f;
    
    particles[0] = {0.0f, 5.0f, 0.0f, 5.0f};
    particles[1] = {1.0f, 4.0f, 1.0f, 4.0f};
    particles[2] = {-1.0f, 3.0f, -1.0f, 3.0f};
    
    for (int step = 0; step < 30; ++step) {
        for (auto& p : particles) {
            float vx = (p.x - p.oldX) * damping;
            float vy = (p.y - p.oldY) * damping;
            
            p.oldX = p.x;
            p.oldY = p.y;
            
            p.x += vx;
            p.y += vy + gravity * dt * dt;
        }
    }
    
    REQUIRE(particles[0].y < 5.0f);
    REQUIRE(particles[1].y < 4.0f);
    REQUIRE(particles[2].y < 3.0f);
    
    float y0 = particles[0].y;
    particles[0] = {0.0f, 5.0f, 0.0f, 5.0f};
    
    for (int step = 0; step < 30; ++step) {
        for (auto& p : particles) {
            float vx = (p.x - p.oldX) * damping;
            float vy = (p.y - p.oldY) * damping;
            p.oldX = p.x;
            p.oldY = p.y;
            p.x += vx;
            p.y += vy + gravity * dt * dt;
        }
    }
    
    REQUIRE(std::abs(particles[0].y - y0) < 0.001f);
}

TEST_CASE("Physics Collision Response - Elastic", "[physics][regression]")
{
    float m1 = 1.0f, m2 = 1.0f;
    float v1 = 5.0f, v2 = -5.0f;
    
    float restitution = 1.0f;
    float v1Final = ((m1 - m2) * v1 + 2.0f * m2 * v2) / (m1 + m2);
    float v2Final = ((m2 - m1) * v2 + 2.0f * m1 * v1) / (m1 + m2);
    
    REQUIRE(v1Final == 5.0f);
    REQUIRE(v2Final == -5.0f);
}

TEST_CASE("Physics Collision Response - Inelastic", "[physics][regression]")
{
    float m1 = 1.0f, m2 = 1.0f;
    float v1 = 10.0f, v2 = 0.0f;
    
    float restitution = 0.0f;
    float vFinal = (m1 * v1 + m2 * v2) / (m1 + m2);
    
    REQUIRE(vFinal == 5.0f);
}

TEST_CASE("Physics Sphere Collision - Overlap Resolution", "[physics][regression]")
{
    struct Sphere {
        float x, y, z;
        float radius;
    };
    
    Sphere s1 = {0.0f, 0.0f, 0.0f, 1.0f};
    Sphere s2 = {1.5f, 0.0f, 0.0f, 1.0f};
    
    float dx = s2.x - s1.x;
    float dy = s2.y - s1.y;
    float dz = s2.z - s1.z;
    float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    float minDist = s1.radius + s2.radius;
    
    REQUIRE(dist < minDist);
    
    float overlap = minDist - dist;
    float nx = dx / dist;
    float ny = dy / dist;
    float nz = dz / dist;
    
    s1.x -= nx * overlap * 0.5f;
    s2.x += nx * overlap * 0.5f;
    
    float newDx = s2.x - s1.x;
    float newDy = s2.y - s1.y;
    float newDz = s2.z - s1.z;
    float newDist = std::sqrt(newDx*newDx + newDy*newDy + newDz*newDz);
    
    REQUIRE(std::abs(newDist - minDist) < 0.001f);
}

TEST_CASE("Physics State Snapshot - Structure", "[physics][regression]")
{
    struct TransformSnapshot {
        float posX, posY, posZ;
        float rotX, rotY, rotZ, rotW;
        float scaleX, scaleY, scaleZ;
    };
    
    struct PhysicsState {
        std::vector<TransformSnapshot> transforms;
        std::vector<float> velocities;
        uint32_t stepCount;
        float time;
    };
    
    PhysicsState state;
    state.stepCount = 100;
    state.time = 1.6667f;
    state.transforms.resize(3);
    state.velocities.resize(3);
    
    state.transforms[0] = {1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    state.transforms[1] = {4.0f, 5.0f, 6.0f, 0.0f, 0.0f, 0.707f, 0.707f, 1.0f, 1.0f, 1.0f};
    state.transforms[2] = {7.0f, 8.0f, 9.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    
    state.velocities[0] = 1.0f;
    state.velocities[1] = 2.0f;
    state.velocities[2] = 3.0f;
    
    REQUIRE(state.transforms.size() == 3);
    REQUIRE(state.velocities.size() == 3);
    REQUIRE(state.stepCount == 100);
}

TEST_CASE("Physics State Comparison - Tolerance", "[physics][regression]")
{
    struct Snapshot {
        std::array<float, 3> position;
    };
    
    Snapshot golden = {1.0f, 2.0f, 3.0f};
    Snapshot actual1 = {1.001f, 2.001f, 3.001f};
    Snapshot actual2 = {1.1f, 2.1f, 3.1f};
    
    float tolerance = 0.01f;
    
    auto withinTolerance = [&](const Snapshot& a, const Snapshot& b) {
        return std::abs(a.position[0] - b.position[0]) < tolerance &&
               std::abs(a.position[1] - b.position[1]) < tolerance &&
               std::abs(a.position[2] - b.position[2]) < tolerance;
    };
    
    REQUIRE(withinTolerance(golden, actual1) == true);
    REQUIRE(withinTolerance(golden, actual2) == false);
}

TEST_CASE("Physics Box2D Style - AABB", "[physics][regression]")
{
    struct AABB {
        float minX, minY, maxX, maxY;
    };
    
    AABB box1 = {0.0f, 0.0f, 1.0f, 1.0f};
    AABB box2 = {0.5f, 0.5f, 1.5f, 1.5f};
    AABB box3 = {2.0f, 2.0f, 3.0f, 3.0f};
    
    auto intersects = [](const AABB& a, const AABB& b) {
        return !(a.maxX < b.minX || a.minX > b.maxX ||
                 a.maxY < b.minY || a.minY > b.maxY);
    };
    
    REQUIRE(intersects(box1, box2) == true);
    REQUIRE(intersects(box1, box3) == false);
    REQUIRE(intersects(box2, box3) == false);
}

TEST_CASE("Physics Integration - Position Update", "[physics][regression]")
{
    struct Body {
        float x, y;
        float vx, vy;
        float ax, ay;
        float mass;
        float inverseMass;
    };
    
    Body body = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -9.81f, 1.0f, 1.0f};
    float dt = 1.0f / 60.0f;
    
    for (int i = 0; i < 60; ++i) {
        body.vx += body.ax * dt;
        body.vy += body.ay * dt;
        body.x += body.vx * dt;
        body.y += body.vy * dt;
    }
    
    float expectedY = 0.5f * (-9.81f) * 60.0f * 60.0f * dt * dt;
    
    REQUIRE(std::abs(body.y - expectedY) < 0.1f);
    REQUIRE(std::abs(body.vy - (-9.81f * 60.0f * dt)) < 1.0f);
}

TEST_CASE("Physics Joint - Distance Constraint", "[physics][regression]")
{
    struct JointState {
        float ax, ay;
        float bx, by;
        float restLength;
        float stiffness;
    };
    
    JointState joint = {0.0f, 0.0f, 5.0f, 0.0f, 5.0f, 100.0f};
    float dt = 1.0f / 60.0f;
    
    for (int i = 0; i < 120; ++i) {
        float dx = joint.bx - joint.ax;
        float dy = joint.by - joint.ay;
        float dist = std::sqrt(dx*dx + dy*dy);
        
        if (dist > 0.001f) {
            float diff = (dist - joint.restLength) / dist;
            float cx = dx * diff * 0.5f * joint.stiffness * dt;
            float cy = dy * diff * 0.5f * joint.stiffness * dt;
            
            joint.ax += cx;
            joint.ay += cy;
            joint.bx -= cx;
            joint.by -= cy;
        }
    }
    
    float dx = joint.bx - joint.ax;
    float dy = joint.by - joint.ay;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    REQUIRE(std::abs(dist - joint.restLength) < 0.1f);
}

TEST_CASE("Physics Regression - Step Count Reproducibility", "[physics][regression]")
{
    float position = 0.0f;
    float velocity = 0.0f;
    float gravity = -9.81f;
    float dt = 1.0f / 60.0f;
    
    auto runSimulation = [&](int steps) {
        float p = 0.0f, v = 0.0f;
        for (int i = 0; i < steps; ++i) {
            v += gravity * dt;
            p += v * dt;
        }
        return p;
    };
    
    float result1 = runSimulation(60);
    float result2 = runSimulation(60);
    
    REQUIRE(std::abs(result1 - result2) < 0.0001f);
    
    float result60 = runSimulation(60);
    float result120 = runSimulation(120);
    
    REQUIRE(result120 < result60);
}