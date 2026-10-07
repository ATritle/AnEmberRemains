#include "IsoPrototype.h"
#include "SpellFX.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "MaraChannelMetrics.h"
#include "MaraCastMetrics.h"
using namespace IsoDungeon;
namespace { const float Cooldowns[]={.6f,4.f,8.f,3.f,6.f,10.f}; }
FVector2D AIsoPrototypePawn::StaffTip() const{
    if(Channeling){const int Loop[]={0,1,2,1};const auto& P=MaraChannel::Tips[FMath::Clamp(Facing,0,7)][Loop[int(ChannelAge*7)%4]];return {P[0],P[1]};}
    const auto& P=MaraCast::Tips[FMath::Clamp(Facing,0,7)][FMath::Clamp(int(FMath::Max(0.f,AttackAge)*10),0,5)];return {P[0],P[1]};
}
bool AIsoPrototypePawn::IsIceProtected() const{const auto* G=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();return G&&G->IceGuardTime>0&&(Position-G->IceCenter).Size()<1.45;}
void AIsoPrototypePawn::UpdateAim(){
    auto* PC=Cast<APlayerController>(GetController());if(!PC)return;float X,Y;int W,H;PC->GetViewportSize(W,H);const float Scale=FMath::Min(W/1280.f,H/800.f);
    if(Scale>0&&PC->GetMousePosition(X,Y)){const auto P=(FVector2D(X,Y)-FVector2D((W-1280*Scale)/2,(H-800*Scale)/2))/Scale;
        SpellTarget=CameraCenter+Unproject(P-FVector2D(640,440));const auto V=(SpellTarget-Position).GetSafeNormal();if(!V.IsNearlyZero())Aim=V;}
}
bool AIsoPrototypePawn::BeginSpell(int Spell){
    auto* G=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();
    if(G&&Spell==3&&G->IceGuardTime>0&&Health>0&&!G->Paused&&!G->MapOpen&&G->LoreIndex<0&&G->DescentTime<0){G->ShatterIce();return true;}
    if(!G||Spell<1||Spell>6||Health<=0||AttackAge>=0||EvadeAge>=0||Channeling||SpellCooldown[Spell-1]>0||G->Paused||G->MapOpen||G->LoreIndex>=0||G->DescentTime>=0)return false;
    ActiveSpell=Spell;Facing=Direction(Aim);AttackAge=0;AttackHit=false;SpellCooldown[Spell-1]=Cooldowns[Spell-1];ChannelAge=ChannelClock=0;return true;
}
void AIsoPrototypePawn::StopChannel(){Channeling=false;ChannelAge=ChannelClock=0;if(ActiveSpell==2||ActiveSpell==4){AttackAge=.42f;AttackHit=true;}}
bool AIsoPrototypeGameMode::IceBlocks(FVector2D P,float Radius) const{for(const auto& I:IcePillars)if(I.Life>0&&(I.P-P).Size()<.32f+Radius)return true;return false;}
FVector2D AIsoPrototypeGameMode::MoveWithIce(FVector2D P,FVector2D Delta) const{
    const int N=FMath::Max(1,FMath::CeilToInt(Delta.Size()/.08));const auto Step=Delta/N;
    for(int I=0;I<N;++I){const auto Next=Grid.Move(P,Step);if(!IceBlocks(Next))P=Next;else{auto X=Grid.Move(P,{Step.X,0});if(!IceBlocks(X))P=X;auto Y=Grid.Move(P,{0,Step.Y});if(!IceBlocks(Y))P=Y;}}return P;
}
bool AIsoPrototypeGameMode::SpellSight(FVector2D A,FVector2D B) const{
    if(!Grid.Sight(A,B))return false;const int N=FMath::Max(1,FMath::CeilToInt((B-A).Size()/.1));
    for(int I=0;I<=N;++I)if(IceBlocks(FMath::Lerp(A,B,double(I)/N),0))return false;return true;
}
void AIsoPrototypeGameMode::DamageEnemy(FIsoEnemy& E,float Damage){
    if(E.HP<=0||Damage<=0)return;E.HP=FMath::Max(0.f,E.HP-Damage);E.Alert=true;E.Hurt=.15f;
    Numbers.Add({E.P,0,FMath::RoundToInt(Damage)});if(E.HP<=0){E.Death=0;++Kills;}
}
void AIsoPrototypeGameMode::ReleaseSpell(AIsoPrototypePawn* H,int Spell){
    if(!H||H->Health<=0)return;auto* FX=ASpellFX::Find(GetWorld());
    if(Spell==1){CastEmber(H);return;}
    if(Spell==2||Spell==4){H->Channeling=true;H->ChannelAge=0;return;}
    if(FX)FX->Emit(H->Position+Unproject(H->StaffTip()),0,H->Aim,Spell==6?5:6,50,.25f,.75f);
    if(Spell==3){
        IceCenter=H->Position;IceGuardTime=6;IcePillars.Empty();if(FX)FX->Clear(2);
        for(int I=0;I<18;++I){const float A=I*2*PI/18;const auto P=IceCenter+FVector2D(FMath::Cos(A),FMath::Sin(A))*1.7;
            bool Clear=Grid.Fits(P)&&Grid.Sight(IceCenter,P);for(const auto& E:Enemies)if(E.HP>0&&(E.P-P).Size()<.65)Clear=false;
            if(!Clear)continue;IcePillars.Add({P,6});
            if(FX){const float Size=65+(I*17%27);FX->Emit(P,Size*.4375f,{1,0},2,Size,5.7f,.94f,{},.1f+(I%3)*.06f,I%4);FX->Emit(P,14,{1,0},2,32,6,.9f,{},0,(I+1)%4);FX->Emit(P,0,{1,0},4,70,.6f,.65f);}
        }
        SpellHint=TEXT("ICE SANCTUARY - protected inside for 6s / press 3 to shatter");SpellHintTime=4;
    }else if(Spell==5){
        if(FX){FX->Emit(H->Position,0,{1,0},4,470,.9f,2.f);
            for(int I=0;I<16;++I){const float A=I*2*PI/16;const FVector2D D(FMath::Cos(A),FMath::Sin(A));if(SpellSight(H->Position,H->Position+D*3.2))FX->Emit(H->Position+D*.35,8,D,7,110,.75f,.55f,D*3.8);}}
        for(auto& E:Enemies)if((E.P-H->Position).Size()<=3.2&&SpellSight(H->Position,E.P)){DamageEnemy(E,45);E.Slow=2.5f;}
        SpellHint=TEXT("FROST NOVA - 45 nearby damage / 55% slow for 2.5s");SpellHintTime=4;
    }else if(Spell==6){
        const float Before=H->Health;H->Health=FMath::Min(150.f,H->Health+35);if(H->Health>Before)Numbers.Add({H->Position,0,FMath::RoundToInt(H->Health-Before),true});
        if(FX){FX->Emit(H->Position,0,{1,0},5,230,1.2f);FX->Emit(H->Position,50,{1,0},5,135,1.3f,.65f);}
    }
}
void AIsoPrototypeGameMode::ChannelSpell(AIsoPrototypePawn* H,float Dt){
    if(!H||!H->Channeling)return;H->ChannelClock+=Dt;if(H->ChannelClock<.05f)return;const float DamageTime=H->ChannelClock;H->ChannelClock=0;
    const bool Fire=H->ActiveSpell==2;const float Range=Fire?4.8f:7.5f;auto* FX=ASpellFX::Find(GetWorld());
    float Length=0;for(float S=.15f;S<=Range;S+=.15f){if(!SpellSight(H->Position,H->Position+H->Aim*S))break;Length=S;}
    for(auto& E:Enemies){const auto V=E.P-H->Position;const float Along=FVector2D::DotProduct(V,H->Aim);const float Side=FMath::Abs(V.X*H->Aim.Y-V.Y*H->Aim.X);
        if(Along>0&&Along<=Length&&Side<(Fire?.30f+Along*.16f:.4f)&&SpellSight(H->Position,E.P))DamageEnemy(E,DamageTime*(Fire?36.f:44.f));}
    const auto Tip=H->StaffTip();const auto OriginOffset=Unproject(Tip+FVector2D(0,55));
    if(FX&&Length>.1f){const auto Travel=H->Aim*Length-OriginOffset;
        if(Fire){const float Life=FMath::Clamp(Length/10.f,.12f,.5f);FX->Emit(H->Position+OriginOffset,55,Travel,1,100,Life,.85f,Travel/Life);FX->Emit(H->Position+Unproject(Tip),0,Travel,8,25,.12f,.8f);}
        else for(float S=.28f;S<Length;S+=.5f){const auto Center=H->Position+H->Aim*S+OriginOffset*(1-S/FMath::Max(.1f,Length));FX->Emit(Center,55,Travel,3,54,.075f,1.f);}
    }
}
void AIsoPrototypeGameMode::ShatterIce(){IceGuardTime=0;IcePillars.Empty();if(auto* FX=ASpellFX::Find(GetWorld(),false)){FX->Clear(2);FX->Emit(IceCenter,0,{1,0},4,260,.5f,.8f);}}
void AIsoPrototypeGameMode::TickSpells(float Dt){
    IceGuardTime=FMath::Max(0.f,IceGuardTime-Dt);SpellHintTime=FMath::Max(0.f,SpellHintTime-Dt);
    for(auto& I:IcePillars)I.Life-=Dt;IcePillars.RemoveAll([](const FIcePillar& I){return I.Life<=0;});
    if(FParse::Param(FCommandLine::Get(),TEXT("SpellReview"))){
        auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));if(!H)return;SpellReviewClock+=Dt;
        const int Index=FMath::Min(5,int(SpellReviewClock/3.5f));
        if(Index!=SpellReviewIndex){H->StopChannel();H->AttackAge=-1;H->EvadeAge=-1;for(float& C:H->SpellCooldown)C=0;H->Aim=(Index==2||Index==3)?FVector2D(.70710678,-.70710678):FVector2D(1,0);int D=-1;if(FParse::Value(FCommandLine::Get(),TEXT("SpellFacing="),D)){const float A=FMath::Clamp(D,0,7)*PI/4;H->Aim=Unproject({FMath::Sin(A),-FMath::Cos(A)}).GetSafeNormal();}H->SpellTarget=H->Position+H->Aim*3;H->Health=100;H->BeginSpell(Index+1);SpellReviewIndex=Index;}
    }
}
