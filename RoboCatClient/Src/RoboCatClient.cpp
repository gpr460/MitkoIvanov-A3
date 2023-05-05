#include <RoboCatClientPCH.h>



RoboCatClient::RoboCatClient() :
	mTimeLocationBecameOutOfSync( 0.f ),
	mTimeVelocityBecameOutOfSync( 0.f )
{
	mSpriteComponent.reset( new SpriteComponent( this ) );
	mSpriteComponent->SetTexture( TextureManager::sInstance->GetTexture( "cat" ) );
}

void RoboCatClient::HandleDying()
{
	RoboCat::HandleDying();

	//and if we're local, tell the hud so our health goes away!
	if( GetPlayerId() == NetworkManagerClient::sInstance->GetPlayerId() )
	{
		HUD::sInstance->SetPlayerHealth( 0 );
	}
}


void RoboCatClient::Update()
{


	//this is where we implement dead reckoning simulation on client side
	//float xVeloSqrd = GetVelocity().mX * GetVelocity().mX;
	//float yVeloSqrd = GetVelocity().mY * GetVelocity().mY;
	//float zVeloSqrd = GetVelocity().mZ * GetVelocity().mZ;
	//float veloMag = sqrt(xVeloSqrd + yVeloSqrd + zVeloSqrd);//calculation for magnitude of current velocity
	//Vector3 newVelo = Vector3(xVeloSqrd, yVeloSqrd, zVeloSqrd);

	MoveList& moveList = InputManager::sInstance->GetMoveList();
	for (const Move& unprocessedMove : moveList)
	{
		const InputState& currentState = unprocessedMove.GetInputState();
		float deltaTime = unprocessedMove.GetDeltaTime();
		if (GetPlayerId() == NetworkManagerClient::sInstance->GetPlayerId())
		{
			ProcessInput(deltaTime, currentState);
		}
		//SimulateMovement(deltaTime);
		AdjustVelocityByThrust(deltaTime);

		SetLocation(GetLocation() + (GetVelocity() * 0.5f) * deltaTime);

		ProcessCollisions();

	}
	

	HandleShooting();

}

void RoboCatClient::HandleShooting()
{
	float time = Timing::sInstance.GetFrameStartTime();
	if (mIsShooting && Timing::sInstance.GetFrameStartTime() > mTimeOfNextShot)
	{
		//not exact, but okay
		mTimeOfNextShot = time + mTimeBetweenShots;

		//fire!
		YarnPtr yarn = std::static_pointer_cast<Yarn>(GameObjectRegistry::sInstance->CreateGameObject('YARN'));
		yarn->InitFromShooter(this);
	}
}



void RoboCatClient::Read( InputMemoryBitStream& inInputStream )
{
	bool stateBit;
	
	uint32_t readState = 0;

	inInputStream.Read( stateBit );
	if( stateBit )
	{
		uint32_t playerId;
		inInputStream.Read( playerId );
		SetPlayerId( playerId );
		readState |= ECRS_PlayerId;
	}

	float oldRotation = GetRotation();
	Vector3 oldLocation = GetLocation();
	Vector3 oldVelocity = GetVelocity();

	float replicatedRotation;
	Vector3 replicatedLocation;
	Vector3 replicatedVelocity;

	inInputStream.Read( stateBit );
	if( stateBit )
	{
		inInputStream.Read( replicatedVelocity.mX );
		inInputStream.Read( replicatedVelocity.mY );

		SetVelocity( replicatedVelocity );

		inInputStream.Read( replicatedLocation.mX );
		inInputStream.Read( replicatedLocation.mY );

		SetLocation( replicatedLocation );

		inInputStream.Read( replicatedRotation );
		SetRotation( replicatedRotation );

		readState |= ECRS_Pose;
	}

	inInputStream.Read( stateBit );
	if( stateBit )
	{
		inInputStream.Read( stateBit );
		mThrustDir = stateBit ? 1.f : -1.f;
	}
	else
	{
		mThrustDir = 0.f;
	}

	inInputStream.Read( stateBit );
	if( stateBit )
	{
		Vector3 color;
		inInputStream.Read( color );
		SetColor( color );
		readState |= ECRS_Color;
	}

	inInputStream.Read( stateBit );
	if( stateBit )
	{
		mHealth = 0;
		inInputStream.Read( mHealth, 4 );
		readState |= ECRS_Health;
	}

	if( GetPlayerId() == NetworkManagerClient::sInstance->GetPlayerId() )
	{
		//did we get health? if so, tell the hud!
		if( ( readState & ECRS_Health ) != 0 )
		{
			HUD::sInstance->SetPlayerHealth( mHealth );
		}
	}	
}
