#pragma once

#include "EnemyActor.h"

class NoctisActor;

// One of Noctis's likenesses in the reckoning: the king to the life, but
// casting no shadow and standing a hair off the ground. It goes where the
// king puts it, round the hall. Struck, it bursts - holding him fast where
// he stands while the king drinks from it. When the reckoning runs out it
// rushes at him with the king, and is gone.
class NoctisLikenessActor : public EnemyActor
{
public:
	explicit NoctisLikenessActor(NoctisActor* owner);

	float TakeHit(float damage) override;
	float BodyRadius() const override;

	// Where the king wants it, in tiles; it glides there.
	void MoveTo(float col, float row);
	// Time's up: a marked rush along this line at him, then gone.
	void Rush(float dirCol, float dirRow);
	// The reckoning broken: it comes apart without harm.
	void Dissolve();

protected:
	void OnUpdate(const ActorContext& ctx) override;
	void OnDraw(DrawList& out, const ActorContext& ctx) const override;

private:
	enum class State
	{
		Circle,
		Rush,
		Gone,
	};

	// The king outlives every likeness it makes: they all go with the region.
	NoctisActor* m_Owner;
	State m_State = State::Circle;
	float m_StateTime = 0.f;
	float m_Age = 0.f;
	float m_ToCol = 0.f;
	float m_ToRow = 0.f;
	bool m_HasTarget = false;
	float m_DirCol = 1.f;
	float m_DirRow = 0.f;
	bool m_Burst = false;    // struck: bursts on him next frame
	bool m_Struck = false;   // its rush has landed
};
