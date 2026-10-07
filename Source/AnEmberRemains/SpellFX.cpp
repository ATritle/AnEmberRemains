#include "SpellFX.h"
#include "IsoPrototype.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#if WITH_EDITOR
#include "NiagaraSpriteRendererProperties.h"
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitializeParticle.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#endif
ASpellFX::ASpellFX(){
    PrimaryActorTick.bCanEverTick=true;
    Camera=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SpellCapture"));RootComponent=Camera;
    Camera->ProjectionType=ECameraProjectionMode::Orthographic;Camera->OrthoWidth=1280;
    Camera->CaptureSource=ESceneCaptureSource::SCS_SceneColorHDR;
    Camera->bCaptureEveryFrame=false;Camera->bCaptureOnMovement=false;
    Camera->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Camera->ShowFlags.SetAtmosphere(false);Camera->ShowFlags.SetFog(false);Camera->ShowFlags.SetPostProcessing(false);
    Camera->ShowFlags.SetLighting(false);Camera->ShowFlags.SetMotionBlur(false);
}
ASpellFX* ASpellFX::Find(UWorld* W,bool Create){
    for(TActorIterator<ASpellFX> I(W);I;++I)return *I;
    if(!Create||!FApp::CanEverRender())return nullptr;
    auto* F=W->SpawnActor<ASpellFX>();F->SetActorRotation(FRotator(-90,0,0));
    F->System=LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/Effects/Spells/NS_CloisterSpell.NS_CloisterSpell"));
    F->Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Effects/Spells/M_CloisterSpell.M_CloisterSpell"));
    F->FlameMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Effects/Spells/M_FlameFlipbook.M_FlameFlipbook"));
    F->IceMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Effects/Spells/M_IceChunks.M_IceChunks"));
    auto* Composite=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Effects/Spells/M_SpellComposite.M_SpellComposite"));
    if(Composite){F->FrontComposite=UMaterialInstanceDynamic::Create(Composite,F);F->BackComposite=UMaterialInstanceDynamic::Create(Composite,F);}
    F->Target=NewObject<UTextureRenderTarget2D>(F);F->Target->ClearColor=FLinearColor::Black;
    F->Target->InitCustomFormat(1280,800,PF_FloatRGBA,false);F->Target->UpdateResourceImmediate(true);F->Camera->TextureTarget=F->Target;
    F->BackTarget=NewObject<UTextureRenderTarget2D>(F);F->BackTarget->ClearColor=FLinearColor::Black;F->BackTarget->InitCustomFormat(1280,800,PF_FloatRGBA,false);F->BackTarget->UpdateResourceImmediate(true);
    UE_LOG(LogTemp,Display,TEXT("SPELL_NIAGARA: system=%d material=%d"),F->System!=nullptr,F->Material!=nullptr);return F;
}
void ASpellFX::Emit(FVector2D W,float Height,FVector2D Dir,int Style,float Size,float Life,float Strength,FVector2D Velocity,float Delay,int Variant){
    if(!System||!Material||Particles.Num()>=256)return;
    const auto P=IsoDungeon::Project(W)-FVector2D(0,Height),D=IsoDungeon::Project(Dir).GetSafeNormal();
    auto* C=NewObject<UNiagaraComponent>(this);C->bAutoActivate=false;C->SetAsset(System);C->SetAutoDestroy(false);C->RegisterComponent();C->SetWorldLocation(FVector(-P.Y,P.X,0));
    auto* Chosen=Style==2?IceMaterial:(Style==1||Style==7)?FlameMaterial:Material;if(!Chosen){C->DestroyComponent();return;}
    auto* M=UMaterialInstanceDynamic::Create(Chosen,C);M->SetScalarParameterValue(TEXT("Angle"),FMath::Atan2(D.Y,D.X));M->SetScalarParameterValue(TEXT("Style"),Style);M->SetScalarParameterValue(TEXT("Strength"),Strength);M->SetScalarParameterValue(TEXT("Seed"),FMath::Fmod(GetWorld()->GetTimeSeconds()*7.31f+P.X*.071f+P.Y*.019f,100.f));M->SetScalarParameterValue(TEXT("Variant"),Variant);M->SetScalarParameterValue(TEXT("Age"),-Delay/Life);
    C->TranslucencySortPriority=Style==2?FMath::RoundToInt(W.Y*100):100000;
    C->SetVariableMaterial(TEXT("User.Material"),M);C->SetWorldScale3D(FVector(Size/128));Camera->ShowOnlyComponent(C);C->Activate(true);
    Particles.Add(C);Materials.Add(M);Ages.Add(-Delay);Lifetimes.Add(FMath::Clamp(Life,.05f,7.f));Origins.Add(W);Velocities.Add(Velocity);Styles.Add(Style);
}
void ASpellFX::Clear(int Style){for(int I=Particles.Num()-1;I>=0;--I)if(Style<0||Styles[I]==Style){Camera->RemoveShowOnlyComponent(Particles[I]);Particles[I]->DestroyComponent();Particles.RemoveAtSwap(I);Materials.RemoveAtSwap(I);Ages.RemoveAtSwap(I);Lifetimes.RemoveAtSwap(I);Origins.RemoveAtSwap(I);Velocities.RemoveAtSwap(I);Styles.RemoveAtSwap(I);}}
void ASpellFX::Tick(float Dt){
    Super::Tick(Dt);auto* G=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!G||!H||G->DescentTime>=0||H->Health<=0){Clear();return;}
    const bool Pause=G->Paused||G->MapOpen||G->LoreIndex>=0||G->Won;
    for(int I=Particles.Num()-1;I>=0;--I){Particles[I]->SetPaused(Pause);if(!Pause)Ages[I]+=FMath::Min(Dt,.05f);Materials[I]->SetScalarParameterValue(TEXT("Age"),Ages[I]/Lifetimes[I]);
        if(!Pause&&Ages[I]>=0&&!Velocities[I].IsNearlyZero()){const float Step=FMath::Min(Dt,.05f);const auto Delta=IsoDungeon::Project(Velocities[I])*Step;Particles[I]->AddWorldOffset(FVector(-Delta.Y,Delta.X,0));Origins[I]+=Velocities[I]*Step;}
        if(Ages[I]>=Lifetimes[I]){Camera->RemoveShowOnlyComponent(Particles[I]);Particles[I]->DestroyComponent();Particles.RemoveAtSwap(I);Materials.RemoveAtSwap(I);Ages.RemoveAtSwap(I);Lifetimes.RemoveAtSwap(I);Origins.RemoveAtSwap(I);Velocities.RemoveAtSwap(I);Styles.RemoveAtSwap(I);}}
}
UMaterialInterface* ASpellFX::Composite(FVector2D Center,bool Behind){
    if(Particles.IsEmpty()||!FrontComposite||!BackComposite)return nullptr;auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return nullptr;
    Camera->ClearShowOnlyComponents();int Count=0;for(int I=0;I<Particles.Num();++I){const bool Back=Styles[I]==2&&Origins[I].Y<H->Position.Y;if(Back==Behind){Camera->ShowOnlyComponent(Particles[I]);++Count;}}
    if(!Count)return nullptr;const auto P=IsoDungeon::Project(Center);SetActorLocation(FVector(40-P.Y,P.X,1000));auto* RT=Behind?BackTarget:Target;Camera->TextureTarget=RT;Camera->CaptureScene();auto* M=Behind?BackComposite:FrontComposite;M->SetTextureParameterValue(TEXT("SpellRT"),RT);return M;
}
bool ASpellFX::BuildAssets(){
#if WITH_EDITOR
    auto Save=[](UObject* O){auto* P=O->GetOutermost();P->MarkPackageDirty();FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;return UPackage::SavePackage(P,O,*FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension()),A);};
    auto* MP=CreatePackage(TEXT("/Game/Effects/Spells/M_CloisterSpell"));MP->FullyLoad();auto* M=NewObject<UMaterial>(MP,TEXT("M_CloisterSpell"),RF_Public|RF_Standalone);
    M->BlendMode=BLEND_Additive;M->SetShadingModel(MSM_Unlit);M->TwoSided=true;M->SetUsageByFlag(MATUSAGE_NiagaraSprites,true);
    auto* UV=NewObject<UMaterialExpressionTextureCoordinate>(M);M->GetExpressionCollection().AddExpression(UV);
    auto* Code=NewObject<UMaterialExpressionCustom>(M);Code->OutputType=CMOT_Float3;Code->Inputs.Reset();
    auto Input=[&](const TCHAR* N,UMaterialExpression* E){FCustomInput I;I.InputName=N;I.Input.Expression=E;Code->Inputs.Add(I);};Input(TEXT("UV"),UV);
    for(const TCHAR* N:{TEXT("Age"),TEXT("Angle"),TEXT("Style"),TEXT("Strength"),TEXT("Seed")}){auto* E=NewObject<UMaterialExpressionScalarParameter>(M);E->ParameterName=N;E->DefaultValue=FString(N)==TEXT("Strength")?1:0;M->GetExpressionCollection().AddExpression(E);Input(N,E);}
    // Continuous, time-evolving silhouettes: no static decal or frame rotation.
    Code->Code=TEXT(R"(
float2 p=(UV-.5)*2; float c=cos(Angle),s=sin(Angle);float2 q=float2(c*p.x+s*p.y,-s*p.x+c*p.y);
float t=saturate(Age),fade=pow(1-t,1.3),r=length(p);float3 outc=0;
if(Style<.5){ // Arcane comet: two twisting filaments and a hot core.
 float bend=sin(q.x*15-t*16)*(.05+t*.09);
 float env=exp(-pow(q.x/.72,4));float tail=exp(-pow((q.y-bend)/(.02+t*.045),2))*env;
 float core=exp(-dot(q,q)/(.014+t*.018));
 outc=(float3(.08,.65,1)*tail*.8+float3(.65,.95,1)*core*1.4)*fade;
}else if(Style<1.5){ // Turbulent fire plume, cooling red at its curling edge.
 float x=q.x+.15-t*.36;float y=q.y+sin(x*6-t*9+Seed)*.14;
 float noise=.60+.23*sin(x*13+y*17-t*11+Seed)+.17*sin(x*21-y*11+t*9+Seed*3);
 float plume=exp(-pow(x/.76,4)-pow(y/(.22+t*.36),2))*saturate(noise);
 outc=lerp(float3(1,.045,.008),float3(1,.47,.06),saturate(plume*1.4))*plume*fade*.7;
}else if(Style<2.5){ // Growing faceted ice spire, crack lines, then a quick dissolve.
 float grow=saturate(t*12),end=saturate((1-t)*8);float y=p.y;
 float width=.42*saturate((y+.88)/.65)*saturate((.91-y)/.20);
 float mask=saturate((width-abs(p.x))*80)*step(-.88,y)*step(y,.9)*step(1-grow,(y+1)*.5);
 float facet=lerp(.27,.8,step(0,p.x))+.12*sin(y*17+p.x*9);
 float edge=exp(-pow((abs(p.x)-width)/.025,2))*.45;
 float crack=exp(-pow((p.x-sin(y*12)*.09)/.012,2))*.3;
 outc=(float3(.13,.55,.85)*facet+float3(.5,.9,1)*(edge+crack))*mask*end;
}else if(Style<3.5){ // Branched electric discharge, refreshed along a clipped ray.
 float cell=(q.x+1)*7;float k=floor(cell);float h0=frac(sin(k*127.1+Seed*31.7)*43758.5)*2-1;float h1=frac(sin((k+1)*127.1+Seed*31.7)*43758.5)*2-1;
 float zig=lerp(h0,h1,frac(cell))*.13*saturate(1-abs(q.x));
 float bolt=exp(-pow((q.y-zig)/.012,2));float corona=exp(-pow((q.y-zig)/.07,2))*.10;
 float branch=exp(-pow((q.y-zig-abs(q.x)*.24)/.010,2))*.2;
 outc=(float3(.58,.4,1)*(corona+branch)+float3(.76,.87,1)*bolt)*exp(-pow(q.x/.84,8))*fade;
}else if(Style<4.5){ // Frost shockwave on the floor plane and outward splinters.
 float2 z=p*float2(1,1.65);float d=length(z),a=atan2(z.y,z.x),rad=.08+t*.84;
 float ring=exp(-pow((d-rad)/(.017+t*.025),2));
 float shards=pow(saturate(cos(a*13+sin(a*3))),32)*exp(-pow((d-rad*.88)/.09,2));
 outc=(float3(.12,.52,1)*ring*.75+float3(.7,.92,1)*shards)*fade;
}else if(Style<5.5){ // Verdant restoration: soft expanding ground ring and rising motes.
 float d=length(p*float2(1,1.65)),rad=.18+t*.62;
 float light=exp(-pow((d-rad)/.04,2))*.28;
 for(int i=0;i<9;i++){float a=i*2.39996;float2 v=float2(cos(a)*.45,sin(a)*.20-t*.68);float2 z=p-v;light+=exp(-dot(z,z)/.002)*sin(t*3.14159);}
 outc=float3(.12,1,.46)*light*fade;
}else if(Style>7.5){ // Compact hot ignition at the measured staff crystal.
 float core=exp(-dot(p,p)/.075);float tip=exp(-pow((q.x-.25)/.35,2)-pow(q.y/.13,2));
 outc=(float3(.45,.8,1)*core+float3(1,.65,.18)*tip)*fade;
}else{ // Impact petals and radial sparks, no lasting floor decal.
 float a=atan2(p.y,p.x),rad=.05+t*.8;
 float ring=exp(-pow((r-rad)/(.02+t*.02),2))*.4;
 float rays=pow(saturate(cos(a*9+t)),28)*exp(-pow((r-rad*.7)/.13,2));
 outc=float3(.12,.7,1)*(ring+rays)*fade;
}
return max(0,outc)*Strength;
)");
    M->GetExpressionCollection().AddExpression(Code);M->GetEditorOnlyData()->EmissiveColor.Expression=Code;M->GetEditorOnlyData()->Opacity.Constant=1;M->PostEditChange();if(!Save(M))return false;
    auto* T=LoadObject<UNiagaraSystem>(nullptr,TEXT("/Niagara/DefaultAssets/Templates/Systems/MinimalLightweight.MinimalLightweight"));if(!T)return false;
    auto* NP=CreatePackage(TEXT("/Game/Effects/Spells/NS_CloisterSpell"));NP->FullyLoad();auto* N=DuplicateObject<UNiagaraSystem>(T,NP,TEXT("NS_CloisterSpell"));N->SetFlags(RF_Public|RF_Standalone);
    for(auto& H:N->GetEmitterHandles()){auto* E=H.GetStatelessEmitter();if(!E)return false;
        for(auto& Mod:E->GetModules())Mod->SetIsModuleEnabled(Mod->IsA<UNiagaraStatelessModule_InitializeParticle>());
        auto* Init=Cast<UNiagaraStatelessModule_InitializeParticle>(E->GetModule(UNiagaraStatelessModule_InitializeParticle::StaticClass()));if(!Init)return false;
        Init->LifetimeDistribution.InitConstant(8.f);Init->SpriteSizeDistribution.InitConstant(FVector2f(128,128));
        for(int I=0;I<E->GetNumSpawnInfos();++I)E->GetSpawnInfoByIndex(I)->bEnabled=false;
        auto& Spawn=E->AddSpawnInfo();Spawn.Type=ENiagaraStatelessSpawnInfoType::Burst;Spawn.Amount=FNiagaraDistributionRangeInt(1);
        auto* Prop=FindFProperty<FStructProperty>(E->GetClass(),TEXT("EmitterState"));auto* State=Prop->ContainerPtrToValuePtr<FNiagaraEmitterStateData>(E);
        State->LoopBehavior=ENiagaraLoopBehavior::Once;State->LoopDuration=FNiagaraDistributionRangeFloat(.01f);
        for(auto* R:E->GetRenderers())if(auto* S=Cast<UNiagaraSpriteRendererProperties>(R)){S->Material=M;S->MaterialUserParamBinding.Parameter=FNiagaraVariable(FNiagaraTypeDefinition(UMaterialInterface::StaticClass()),TEXT("User.Material"));N->GetExposedParameters().AddParameter(S->MaterialUserParamBinding.Parameter);}
        E->PostEditChange();
    }
    N->PostEditChange();N->RequestCompile(true);N->WaitForCompilationComplete(true,false);return Save(N)&&BuildTexturedAssets();
#else
    return false;
#endif
}
