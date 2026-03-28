#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace ge {
namespace debug {

struct ProfileNode {
    std::string Name;
    int64_t StartTime = 0;
    int64_t EndTime = 0;
    int64_t TotalTime = 0;
    int HitCount = 0;
    int ParentIndex = -1;
    std::vector<int> Children;
    
    float GetAverageTime() const {
        return HitCount > 0 ? (float)TotalTime / (float)HitCount : 0.0f;
    }
};

class Profiler {
public:
    static Profiler& Get() {
        static Profiler instance;
        return instance;
    }
    
    void BeginFrame() {
        m_CurrentFrame++;
        m_Nodes.clear();
        m_NodeStack.clear();
        m_RootIndex = -1;
    }
    
    void EndFrame() {
        while (!m_NodeStack.empty()) {
            EndScope("");
        }
        CalculateFrameStats();
    }
    
    void BeginScope(const char* name) {
        ProfileNode node;
        node.Name = name;
        node.StartTime = GetTimeMicroseconds();
        node.ParentIndex = m_NodeStack.empty() ? -1 : m_NodeStack.back();
        
        int nodeIndex = (int)m_Nodes.size();
        m_Nodes.push_back(node);
        
        if (!m_NodeStack.empty()) {
            m_Nodes[m_NodeStack.back()].Children.push_back(nodeIndex);
        } else {
            m_RootIndex = nodeIndex;
        }
        
        m_NodeStack.push_back(nodeIndex);
    }
    
    void EndScope(const char* name) {
        (void)name;
        if (m_NodeStack.empty()) return;
        
        int nodeIndex = m_NodeStack.back();
        m_NodeStack.pop_back();
        
        m_Nodes[nodeIndex].EndTime = GetTimeMicroseconds();
        m_Nodes[nodeIndex].TotalTime += m_Nodes[nodeIndex].EndTime - m_Nodes[nodeIndex].StartTime;
        m_Nodes[nodeIndex].HitCount++;
    }
    
    float GetFrameTime() const { return m_FrameTime; }
    float GetFPS() const { return m_FPS; }
    float GetAverageFPS() const { return m_AverageFPS; }
    float GetMinFPS() const { return m_MinFPS; }
    float GetMaxFPS() const { return m_MaxFPS; }
    
    const std::vector<ProfileNode>& GetNodes() const { return m_Nodes; }
    int GetRootIndex() const { return m_RootIndex; }
    
    int64_t GetTotalFrameMemory() const { return m_TotalMemory; }
    int64_t GetFrameMemoryUsage() const { return m_FrameMemory; }
    
private:
    Profiler() 
        : m_CurrentFrame(0)
        , m_FrameTime(0.0f)
        , m_FPS(0.0f)
        , m_AverageFPS(0.0f)
        , m_MinFPS(0.0f)
        , m_MaxFPS(0.0f)
        , m_LastFrameTime(0)
        , m_TotalMemory(0)
        , m_FrameMemory(0)
        , m_RootIndex(-1)
    {}
    
    int64_t GetTimeMicroseconds() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }
    
    void CalculateFrameStats() {
        m_FrameTime = (float)(GetTimeMicroseconds() - m_LastFrameTime) / 1000.0f;
        m_LastFrameTime = GetTimeMicroseconds();
        
        if (m_FrameTime > 0.0f) {
            m_FPS = 1000.0f / m_FrameTime;
        }
        
        m_FPSHistory.push_back(m_FPS);
        if (m_FPSHistory.size() > 100) {
            m_FPSHistory.erase(m_FPSHistory.begin());
        }
        
        float totalFPS = 0.0f;
        float minFPS = 99999.0f;
        float maxFPS = 0.0f;
        for (float fps : m_FPSHistory) {
            totalFPS += fps;
            minFPS = std::min(minFPS, fps);
            maxFPS = std::max(maxFPS, fps);
        }
        m_AverageFPS = totalFPS / (float)m_FPSHistory.size();
        m_MinFPS = minFPS;
        m_MaxFPS = maxFPS;
    }
    
    int m_CurrentFrame;
    float m_FrameTime;
    float m_FPS;
    float m_AverageFPS;
    float m_MinFPS;
    float m_MaxFPS;
    int64_t m_LastFrameTime;
    int64_t m_TotalMemory;
    int64_t m_FrameMemory;
    
    std::vector<ProfileNode> m_Nodes;
    std::vector<int> m_NodeStack;
    int m_RootIndex;
    
    std::vector<float> m_FPSHistory;
};

class ProfileScope {
public:
    ProfileScope(const char* name) : m_Name(name) {
        Profiler::Get().BeginScope(m_Name);
    }
    
    ~ProfileScope() {
        Profiler::Get().EndScope(m_Name);
    }
    
private:
    const char* m_Name;
};

} // namespace debug
} // namespace ge

#if defined(GE_DEBUG) || defined(GE_ENABLE_PROFILING)
    #define GE_PROFILE_SCOPE(name) ge::debug::ProfileScope _profile_scope(name)
    #define GE_PROFILE_FUNC() GE_PROFILE_SCOPE(__FUNCTION__)
#else
    #define GE_PROFILE_SCOPE(name)
    #define GE_PROFILE_FUNC()
#endif
