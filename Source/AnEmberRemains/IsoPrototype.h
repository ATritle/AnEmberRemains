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
    float EvadeAge=-1,EvadeCooldown=0;
    bool Rolling=false;
    bool IsPhasing() const{return Rolling&&EvadeAge>=0;}
    bool ReceiveHit(float Damage);
    FVector2D PhaseOrigin,PhaseEnd;
    float PhaseAfter=0;
    static constexpr float PhaseDuration=.4025f; // 15% more range at twice the speed.
    static constexpr float PhaseSpeed=12.4f;
    FVector2D EvadeDirection;
    bool BeginEvade(bool Roll,FVector2D Direction);
    void TickEvade(float Dt);
    int Facing=4,Combo=0;
    bool Moving=false,Sprinting=false,AttackHit=false;
    int ActiveSpell=0;
    float SpellCooldown[6]={},ChannelAge=0,ChannelClock=0;
    bool Channeling=false;
    FVector2D SpellTarget;
    void UpdateAim();
    bool BeginSpell(int Spell);
    void StopChannel();
    FVector2D StaffTip() const;
    bool IsIceProtected() const;
};
struct FIsoEnemy {FVector2D P,Aim;float HP=70,Attack=-1,Cooldown=0,Hurt=0,Death=-1,Walk=0,Slow=0;bool Alert=false,Hit=false;};
struct FIsoNumber {FVector2D P;float Age=0;int Value=0;bool Healing=false;};
struct FEmberBolt {FVector2D P,Velocity;float Life=1.4f,FXClock=0;FVector2D TipOffset;float Age=0;};
struct FIcePillar {FVector2D P;float Life=6;};
struct FCloisterProp {FIntPoint Cell;int Art=0;float Size=140;};
struct FCloisterArch {FVector2D P;bool Flip=false;};
struct FCloisterDetail {FVector2D P;int Art=0;float Size=1,Phase=0;};
struct FCloisterRat {FVector2D P,Direction{1,0};float Decision=0,Travel=0,Fear=0;bool Moving=false;};
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
    void ReleaseSpell(AIsoPrototypePawn* Hero,int Spell);
    void TickSpells(float Dt);
    void ChannelSpell(AIsoPrototypePawn* Hero,float Dt);
    void DamageEnemy(FIsoEnemy& Enemy,float Damage);
    bool SpellSight(FVector2D A,FVector2D B) const;
    bool IceBlocks(FVector2D P,float Radius=.22f) const;
    FVector2D MoveWithIce(FVector2D P,FVector2D Delta) const;
    void VerifySpells(int& Checks,int& Errors,FString& Failures);
    TArray<FIcePillar> IcePillars;
    FVector2D IceCenter;
    float IceGuardTime=0,SpellHintTime=0;
    FString SpellHint;
    void ShatterIce();
    float SpellReviewClock=0;
    int SpellReviewIndex=-1;
    FString RoomName(FIntPoint Cell) const;
    void Verify();
    void DressFloor();
    void BuildLife();
    void TickLife(float Dt,FVector2D Player);
    void BuildLighting();
    bool BeginDescent(AIsoPrototypePawn* Hero);
    void NextFloor();
    int RoomIndex(FIntPoint Cell) const;
    IsoDungeon::FGrid Grid;
    TArray<FIsoEnemy> Enemies;
    TArray<FIsoNumber> Numbers;
    TArray<FEmberBolt> Bolts;
    TArray<int> Flow;
    TArray<uint8> Seen;
    TArray<float> Reveal;
    TArray<FCloisterProp> Props;
    TArray<FCloisterArch> Arches;
    TArray<FCloisterDetail> Details;
    TArray<FCloisterRat> Rats;
    FRandomStream AmbientRandom;
    float AmbientTime=0;
    int LoreIndex=-1;
    int FloorNumber=1;
    float DescentTime=-1,ArrivalFade=0;
    FVector2D DescentStart;
    TArray<int> CandleProps;
    TArray<TArray<float>> CandleWeights;
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
    TMap<int,float> WallOpacity;
    UTexture2D* Texture(const FString& Name);
    FVector2D Screen(FVector2D P) const;
    void Polygon(const TArray<FVector2D>& P,FLinearColor Color);
    void Label(const FString& Text,FVector2D P,FLinearColor Color,float Size=1);
    void Tile(FIntPoint Cell,FLinearColor Color,float Height=0);
    void Quad(UTexture2D* T,const TArray<FVector2D>& P,FVector2D UV,FVector2D Span,FLinearColor Tint);
    void Glow(FVector2D P,float Radius,FLinearColor Color);
    void Prop(const FCloisterProp& P,float Alpha);
    void Arch(const FCloisterArch& A,float Alpha);
    void FloorDetail(const FCloisterDetail& D,float Alpha,float Time);
    void Rat(const FCloisterRat& R,float Alpha);
    void Hero(AIsoPrototypePawn* H);
    void PrepareLighting(AIsoPrototypeGameMode* G,AIsoPrototypePawn* H);
    FLinearColor Shade(FVector2D W) const;
    void Atmosphere(AIsoPrototypeGameMode* G,AIsoPrototypePawn* H);
    TArray<FLinearColor> CellLighting;
    float Zoom=1;
    FVector2D Offset,CameraCenter;
};
