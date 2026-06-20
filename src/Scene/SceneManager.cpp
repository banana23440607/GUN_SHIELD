#include "SceneManager.h"

void SceneManager::RegisterFactory(SceneID id, std::function<std::unique_ptr<SceneBase>()> factory)
{
    m_factories[id] = std::move(factory);
}

void SceneManager::Change(SceneID id)
{
    auto it = m_factories.find(id);
    if (it == m_factories.end()) return;

    m_next = it->second();
    m_changeReady = true;
}

void SceneManager::Update(float dt)
{
    if (m_changeReady)
    {
        ApplyPendingChange();
        m_changeReady = false;
    }
    if (m_current) m_current->Update(dt);
}

void SceneManager::Draw()
{
    if (m_current) m_current->Draw();
}

void SceneManager::ApplyPendingChange()
{
    if (m_current) m_current->Shutdown();
    m_current = std::move(m_next);
    if (m_current) m_current->Init();
}
