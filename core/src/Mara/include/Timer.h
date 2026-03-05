#pragma once

class Timer
{
public:
    Timer();

    void Update();

    float GetDeltaTime() const { return m_DeltaTime; }
    float GetTime() const { return m_CurrentTime; }

private:
    float m_CurrentTime = 0.0f;
    float m_LastTime = 0.0f;
    float m_DeltaTime = 0.0f;
};