#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "IsoDungeonGrid.h"
#include "IsoPrototype.generated.h"

class UCameraComponent;
class UTexture2D;
UCLASS()
class AIsoPrototypePawn : public APawn
{
    GENERATED_BODY()
public:
    AIsoPrototypePawn();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    UPROPERTY() UCameraComponent* Camera;
    FVector2D Position,CameraCenter,Aim{0,-1};
    float Health=150,WalkDistance=0,AttackAge=-1,Hurt=0;
    int Facing=4,Combo=0;
    bool Moving=false,Sprinting=false,AttackHit=false;
};
struct FIsoEnemy {FVector2D P,Aim;float HP=70,Attack=-1,Cooldown=0,Hurt=0,Death=-1,Walk=0;bool Alert=false,Hit=false;};
struct FIsoNumber {FVector2D P;float Age=0;int Value=0;};
struct FEmberBolt {FVector2D P,Velocity;float Life=1.4f;};
UCLASS()
class AIsoPrototypeGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AIsoPrototypeGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    void Generate(int Seed);
    void CastEmber(AIsoPrototypePawn* Hero);
    void TickBolts(float Dt);
    FString RoomName(FIntPoint Cell) const;
    void Verify();
    IsoDungeon::FGrid Grid;
    TArray<FIsoEnemy> Enemies;
    TArray<FIsoNumber> Numbers;
    TArray<FEmberBolt> Bolts;
    TArray<int> Flow;
    TArray<uint8> Seen;
    int Seed=4816,Kills=0;
    float FlowClock=0,ReviewClock=0;
    bool MapOpen=false,Won=false,Paused=false;
};
UCLASS()
class AIsoPrototypeHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    UPROPERTY() TMap<FString,UTexture2D*> Textures;
    UTexture2D* Texture(const FString& Name);
    FVector2D Screen(FVector2D P) const;
    void Polygon(const TArray<FVector2D>& P,FLinearColor Color);
    void Label(const FString& Text,FVector2D P,FLinearColor Color,float Size=1);
    void Tile(FIntPoint Cell,FLinearColor Color,float Height=0);
    void Hero(AIsoPrototypePawn* H);
    float Zoom=1;
    FVector2D Offset,CameraCenter;
};
