#pragma once
#include "SceneBase.h"
#include <memory>
#include <functional>

// シーン種別
enum class SceneID
{
    Title,
    Lobby,
    Battle,
    Result
};

class SceneManager
{
public:
    static SceneManager& Get()
    {
        static SceneManager inst;
        return inst;
    }

    void RegisterFactory(SceneID id, std::function<std::unique_ptr<SceneBase>()> factory);
    void Change(SceneID id);

    void Update(float dt);
    void Draw();

private:
    SceneManager() = default;
    void ApplyPendingChange();

    std::unique_ptr<SceneBase> m_current;
    std::unique_ptr<SceneBase> m_next;
    bool m_changeReady = false;

    std::unordered_map<SceneID, std::function<std::unique_ptr<SceneBase>()>> m_factories;
};
