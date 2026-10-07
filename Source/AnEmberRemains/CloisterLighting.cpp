#include "IsoPrototype.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
using namespace IsoDungeon;
namespace {float Flame(float T,float Phase){return .88f+.16f*FMath::Sin(T*5.1f+Phase)+.085f*FMath::Sin(T*11.7f+Phase*2)+.04f*FMath::Sin(T*19.3f);}}

void AIsoPrototypeGameMode::BuildLighting(){
    CandleProps.Empty();CandleWeights.Empty();
    for(int I=0;I<Props.Num();++I)if(Props[I].Art==0){CandleProps.Add(I);auto& Weights=CandleWeights.AddDefaulted_GetRef();Weights.Init(0,FGrid::Size*FGrid::Size);const auto Origin=Center(Props[I].Cell);
        for(int Y=0;Y<FGrid::Size;++Y)for(int X=0;X<FGrid::Size;++X)if(Grid.Floor(X,Y)){const auto P=Center({X,Y});const float D=(P-Origin).Size();if(D<8&&Grid.Sight(Origin,P))Weights[Y*FGrid::Size+X]=FMath::Square(1-D/8);}
    }
}
bool AIsoPrototypeGameMode::BeginDescent(AIsoPrototypePawn* H){
    if(!H||H->Health<=0||DescentTime>=0||Paused||MapOpen||LoreIndex>=0||!Grid.AtStairEntry(H->Position))return false;
    DescentStart=H->Position;DescentTime=0;H->Channeling=false;H->AttackAge=-1;Bolts.Empty();return true;
}
void AIsoPrototypeGameMode::NextFloor(){
    auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));const float HP=H?H->Health:150;
    ++FloorNumber;Generate(Seed+7919);if(H){H->Health=HP;H->Moving=false;}ArrivalFade=1;
}
void AIsoPrototypeHUD::PrepareLighting(AIsoPrototypeGameMode* G,AIsoPrototypePawn* H){
    CellLighting.Init(FLinearColor(.12f,.145f,.18f),FGrid::Size*FGrid::Size);
    for(int Y=0;Y<FGrid::Size;++Y)for(int X=0;X<FGrid::Size;++X){const int K=Y*FGrid::Size+X;if(!G->Grid.Floor(X,Y))continue;const auto W=Center({X,Y});const float Personal=FMath::Max(0.f,1-float((W-H->Position).Size())/4)*.12f;
        auto& C=CellLighting[K];C+=FLinearColor(Personal,Personal*.86f,Personal*.7f,0);
        if(H->Channeling){const auto V=W-H->Position;const float Along=FVector2D::DotProduct(V,H->Aim),Side=FMath::Abs(V.X*H->Aim.Y-V.Y*H->Aim.X);if(Along>0&&Along<5&&Side<2&&G->SpellSight(H->Position,W)){const float F=(1-Side/2)*.28f;C+=H->ActiveSpell==2?FLinearColor(F,F*.29f,F*.04f,0):FLinearColor(F*.4f,F*.45f,F,0);}}
        for(int I=0;I<G->CandleProps.Num();++I){const auto& P=G->Props[G->CandleProps[I]];const float F=Flame(G->AmbientTime,P.Cell.X*.7f+P.Cell.Y)*G->CandleWeights[I][K];C+=FLinearColor(.85f*F,.53f*F,.24f*F,0);}
        C.R=FMath::Min(C.R,.95f);C.G=FMath::Min(C.G,.85f);C.B=FMath::Min(C.B,.7f);C.A=1;
    }
}
FLinearColor AIsoPrototypeHUD::Shade(FVector2D W) const{const auto C=Cell(W);const int K=C.Y*FGrid::Size+C.X;return CellLighting.IsValidIndex(K)?CellLighting[K]:FLinearColor(.15f,.17f,.2f);}
void AIsoPrototypeHUD::Atmosphere(AIsoPrototypeGameMode* G,AIsoPrototypePawn* H){
    for(int Index:G->CandleProps){const auto& C=G->Props[Index];const int K=C.Cell.Y*FGrid::Size+C.Cell.X;if(!G->Seen[K]||(Center(C.Cell)-H->Position).Size()>15)continue;
        const auto Ground=Screen(Center(C.Cell)),Tip=Ground-FVector2D(0,C.Size*.83f*Zoom);const float F=Flame(G->AmbientTime,C.Cell.X*.7f+C.Cell.Y);
        Glow(Tip,40,FLinearColor(1,.47f,.13f,.8f*F));
        // Layered soft scattering in the damp air, not a solid triangle.
        for(int J=0;J<15;++J){const float T=J/14.f;const auto P=FMath::Lerp(Tip,Ground+FVector2D(34*Zoom,0),double(T));Glow(P,12+T*42,FLinearColor(.85f,.49f,.23f,F*.19f*(1-T*.6f)));}
        for(int J=0;J<5;++J){const float A=FMath::Frac(G->AmbientTime*.08f+J*.19f+Index*.07f);const auto P=Tip+FVector2D(FMath::Sin(A*12+Index)*30,50-A*80)*Zoom;DrawRect(FLinearColor(.85f,.68f,.38f,F*.23f),P.X,P.Y,1*Zoom,1*Zoom);}
    }
    // Cool, low ground fog remains separate from warm candle halos.
    for(int I=0;I<64;++I){const FVector2D GP(FMath::Fmod(I*17.13+G->AmbientTime*.10,62.)+1,FMath::Fmod(I*11.7+G->AmbientTime*.045,62.)+1);const auto W=Rotate45(GP);const auto C=Cell(W);if(G->Grid.Floor(C.X,C.Y)&&G->Seen[C.Y*FGrid::Size+C.X]&&(W-H->Position).Size()<14)Glow(Screen(W),75,FLinearColor(.27f,.34f,.39f,.27f));}
}
