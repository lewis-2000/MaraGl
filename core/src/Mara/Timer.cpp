#include "Timer.h"
#include <GLFW/glfw3.h>

Timer::Timer()
{
    m_LastTime = (float)glfwGetTime();
}

void Timer::Update()
{
    m_CurrentTime = (float)glfwGetTime();
    m_DeltaTime = m_CurrentTime - m_LastTime;
    m_LastTime = m_CurrentTime;
}