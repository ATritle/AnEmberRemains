#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpellFX.generated.h"
class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;
class USceneCaptureComponent2D;
// World-anchored Niagara, orthographically composited into the 2D dungeon.
UCLASS()
class ASpellFX : public AActor {
    GENERATED_BODY()
public:
    ASpellFX();
    static ASpellFX* Find(UWorld* World,bool Create=true);
    static bool BuildAssets();
    static bool BuildTexturedAssets();
    void Emit(FVector2D World,float Height,FVector2D Direction,int Style,float Size,float Life,float Strength=1,FVector2D Velocity=FVector2D::ZeroVector,float Delay=0,int Variant=0);
    void Clear(int Style=-1);
    virtual void Tick(float Dt) override;
    UMaterialInterface* Composite(FVector2D Center,bool Behind=false);
private:
    UPROPERTY() USceneCaptureComponent2D* Camera;
    UPROPERTY() UTextureRenderTarget2D* Target;
    UPROPERTY() UTextureRenderTarget2D* BackTarget;
    UPROPERTY() UNiagaraSystem* System;
    UPROPERTY() UMaterialInterface* Material;
    UPROPERTY() UMaterialInterface* FlameMaterial;
    UPROPERTY() UMaterialInterface* IceMaterial;
    UPROPERTY() UMaterialInstanceDynamic* FrontComposite;
    UPROPERTY() UMaterialInstanceDynamic* BackComposite;
    UPROPERTY() TArray<UNiagaraComponent*> Particles;
    UPROPERTY() TArray<UMaterialInstanceDynamic*> Materials;
    TArray<float> Ages,Lifetimes;
    TArray<FVector2D> Origins,Velocities;
    TArray<int> Styles;
};
