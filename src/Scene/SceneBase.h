#pragma once
class SceneBase{
public:
    virtual ~SceneBase()=default;
    virtual bool Init()=0;
    virtual void Update(float dt)=0;
    virtual void Draw()=0;
    virtual void Shutdown()=0;
};
