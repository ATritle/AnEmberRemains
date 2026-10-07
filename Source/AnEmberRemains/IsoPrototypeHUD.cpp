#include "IsoPrototype.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "CanvasItem.h"
#include "RenderUtils.h"
#include "Kismet/GameplayStatics.h"
#include "FullBodyArtMetrics.h"
#include "MaraArtMetrics.h"
#include "MaraChannelMetrics.h"

using namespace IsoDungeon;
UTexture2D* AIsoPrototypeHUD::Texture(const FString& Name){
    if(auto* T=Textures.Find(Name))return *T;
    const auto Folder=Name.StartsWith(TEXT("Mara_"))?TEXT("MaraVey"):Name.StartsWith(TEXT("Cloister"))?TEXT("Cloisters"):TEXT("V2");
    auto* T=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/%s/%s.%s"),Folder,*Name,*Name));Textures.Add(Name,T);return T;
}
FVector2D AIsoPrototypeHUD::Screen(FVector2D P) const{return Offset+(IsoDungeon::Project(P-CameraCenter)+FVector2D(640,440))*Zoom;}
void AIsoPrototypeHUD::Polygon(const TArray<FVector2D>& P,FLinearColor Color){
    for(int I=1;I+1<P.Num();++I){FCanvasTriangleItem T(P[0],P[I],P[I+1],GWhiteTexture);T.SetColor(Color);T.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(T);}
}
void AIsoPrototypeHUD::Label(const FString& Text,FVector2D P,FLinearColor Color,float Size){DrawText(Text,Color,Offset.X+P.X*Zoom,Offset.Y+P.Y*Zoom,GEngine->GetSmallFont(),Size*Zoom,false);}
void AIsoPrototypeHUD::Quad(UTexture2D* T,const TArray<FVector2D>& P,FVector2D UV,FVector2D Span,FLinearColor Tint){
    if(!T||!T->GetResource())return;const FVector2D U[]={UV,UV+FVector2D(Span.X,0),UV+Span,UV+FVector2D(0,Span.Y)};
    for(int I=1;I<3;++I){FCanvasTriangleItem Triangle(P[0],P[I],P[I+1],U[0],U[I],U[I+1],T->GetResource());Triangle.SetColor(Tint);Triangle.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Triangle);}
}
void AIsoPrototypeHUD::Tile(FIntPoint Cell,FLinearColor Color,float Height){
    TArray<FVector2D> P;for(auto V:{FVector2D(0,0),FVector2D(1,0),FVector2D(1,1),FVector2D(0,1)})P.Add(Screen(Rotate45(FVector2D(Cell)+V))-FVector2D(0,Height*Zoom));
    const FVector2D UV((Cell.X%4)*.25,(Cell.Y%4)*.25);
    if(Height>0){
        for(int I=1;I<3;++I){auto Tint=Color*(I==1?.68f:.82f);Tint.A=Color.A;
            Quad(Texture(TEXT("CloisterWall")),{P[I],P[I+1],P[I+1]+FVector2D(0,Height*Zoom),P[I]+FVector2D(0,Height*Zoom)},{((Cell.X+Cell.Y)%4)*.25,0},{.25,1},Tint);
            auto* Game=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();
            const FIntPoint Near=Cell+(I==1?FIntPoint(1,0):FIntPoint(0,1));
            if(Game&&Game->Grid.Floor(Near.X,Near.Y)){
                const uint32 Hash=uint32(Cell.X*73+Cell.Y*193+I*11+Game->Seed);const int Room=Game->RoomIndex(Near);
                const int Art=Hash%5==0?((Room>0&&(Room-1)%4==2)?3:1):Hash%7==0?2:0;
                if(Hash%3==0||Art!=0){
                    const float Top=Art==2?35:8,Bottom=Art==2?105:108;
                    const auto A=FMath::Lerp(P[I],P[I+1],.04),B=FMath::Lerp(P[I],P[I+1],.96);
                    Quad(Texture(TEXT("CloisterWallLife")),{A+FVector2D(0,Top*Zoom),B+FVector2D(0,Top*Zoom),B+FVector2D(0,Bottom*Zoom),A+FVector2D(0,Bottom*Zoom)},{(Art%2)*.5,(Art/2)*.5},{.5,.5},FLinearColor(.82f,.82f,.76f,Color.A*(Art==0?.58f:.9f)));
                }
            }
            // Carved cap and low plinth, aligned to the wall plane.
            // Canvas lines do not reliably honor alpha. Use translucent quads
            // for moulding too, otherwise bright rails remain across the hero.
            for(float H:{4.f,Height-9.f}){const auto A=P[I]+FVector2D(0,H*Zoom),B=P[I+1]+FVector2D(0,H*Zoom);Polygon({A,B,B+FVector2D(0,3*Zoom),A+FVector2D(0,3*Zoom)},FLinearColor(.38f,.34f,.27f,Color.A));}
        }
    }
    Quad(Texture(TEXT("CloisterFloor")),P,UV,{.25,.25},Color);
}
void AIsoPrototypeHUD::Glow(FVector2D P,float Radius,FLinearColor Color){
    for(int R=7;R>0;--R){TArray<FVector2D> Circle;for(int I=0;I<20;++I){float A=I*2*PI/20;Circle.Add(P+FVector2D(FMath::Cos(A),FMath::Sin(A)*.55f)*Radius*Zoom*R/7);}auto C=Color;C.A*=.038f;Polygon(Circle,C);}
}
void AIsoPrototypeHUD::Prop(const FCloisterProp& Item,float Alpha){
    const auto P=Screen(Center(Item.Cell));const float S=Item.Size*Zoom;
    auto Tint=Shade(Center(Item.Cell));Tint.A=Alpha;DrawTexture(Texture(TEXT("CloisterProps")),P.X-S*.5f,P.Y-S*.94f,S,S,(Item.Art%4)*.25f,(Item.Art/4)*.5f,.25f,.5f,Tint,BLEND_Translucent);
    if(Item.Art==0){const float T=GetWorld()->GetTimeSeconds();const auto Flame=P-FVector2D(0,S*.83f);Glow(Flame,20,FLinearColor(1,.36f,.06f,Alpha));for(int I=0;I<3;++I){const float Age=FMath::Frac(T*.35f+I*.31f);const auto E=Flame+FVector2D(FMath::Sin(T+I)*5,-Age*40)*Zoom;DrawRect(FLinearColor(1,.54f,.13f,Alpha*(1-Age)),E.X,E.Y,1.5f*Zoom,2*Zoom);}}
}
void AIsoPrototypeHUD::Arch(const FCloisterArch& Item,float Alpha){
    auto P=Screen(Item.P);const float W=285*Zoom,H=258*Zoom;const float S=(Item.Flip?1:-1)*40*Zoom;
    // UV bounds select the complete arch without editing its generated alpha.
    Quad(Texture(TEXT("CloisterArch")),{P+FVector2D(-W*.5,-H-S),P+FVector2D(W*.5,-H+S),P+FVector2D(W*.5,20*Zoom+S),P+FVector2D(-W*.5,20*Zoom-S)},Item.Flip?FVector2D(.755,.04):FVector2D(.29,.04),{Item.Flip?-.465:.465,.92},FLinearColor(1,1,1,Alpha));
}
void AIsoPrototypeHUD::Hero(AIsoPrototypePawn* H){
    const int D=FMath::Clamp(H->Facing,0,7);auto* G=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();const float Time=G?G->AmbientTime:0;
    if(H->Channeling){const int Loop[]={0,1,2,1};const int Frame=Loop[int(H->ChannelAge*7)%4];const auto P=Screen(H->Position);auto* T=Texture(TEXT("Mara_Channel"));
        DrawTexture(T,P.X-128*Zoom,P.Y-225*Zoom,256*Zoom,256*Zoom,Frame/3.f,D/8.f,1/3.f,1/8.f,FLinearColor(.8f,.8f,.8f),BLEND_Translucent);return;}
    const int Breath[]={0,1,2,3,4,5,4,3,2,1};
    int C=H->Moving?(H->Sprinting?2:1):0,F=H->Moving?int(H->WalkDistance*(H->Sprinting?2.5f:3.5f))%6:Breath[int(Time*3)%10];
    if(H->AttackAge>=0){C=5;F=FMath::Clamp(int(H->AttackAge*10),0,5);}
    if(H->EvadeAge>=0){C=H->Rolling?0:3;F=H->Rolling?2:FMath::Clamp(int(H->EvadeAge/.55f*6),0,5);}
    const auto& R=MaraArt::Frames[D][C*6+F];auto* T=Texture(MaraArt::Names[D]);if(!T)return;
    // Restore the taller, slender turnaround silhouette without changing foot anchors.
    const float K=MaraArt::Scale[D]*Zoom,KX=K*.92f,KY=K*1.18f;const auto P=Screen(H->Position);auto TL=P-FVector2D(R.PX*KX,R.PY*KY);auto Tint=Shade(H->Position);Tint.R=FMath::Max(.55f,Tint.R);Tint.G=FMath::Max(.53f,Tint.G);Tint.B=FMath::Max(.5f,Tint.B);
    if(G&&G->DescentTime>=0){Tint.A=1-FMath::Clamp(G->DescentTime/1.3f,0.f,1.f);TL.Y+=G->DescentTime*22*Zoom;}
    auto Body=[&](FVector2D Shift,FLinearColor Color,EBlendMode Mode){if(Mode==BLEND_Additive){Color.R*=Color.A*.22f;Color.G*=Color.A*.22f;Color.B*=Color.A*.22f;Color.A=1;}DrawTexture(T,TL.X+Shift.X,TL.Y+Shift.Y,R.W*KX,R.H*KY,R.X/T->GetSizeX(),R.Y/T->GetSizeY(),R.W/T->GetSizeX(),R.H/T->GetSizeY(),Color,Mode);};
    auto Mist=[&](FVector2D At,float Age,float Life){if(Age<0||Age>=Life)return;const float Q=Age/Life,Fade=FMath::Sin(Q*PI);const auto Base=Screen(At);
        for(int I=0;I<13;++I){const float A=I*2.39996f;const FVector2D Drift(FMath::Cos(A)*(8+Q*33),-18-I*5-Q*25+FMath::Sin(A)*12);
            Glow(Base+Drift*Zoom,(12+Q*23)*(I%2?.85f:1.f),FLinearColor(.08f,.45f,1.f,Fade*.9f));}
    };
    if(H->IsPhasing()){
        const float Q=H->EvadeAge/H->PhaseDuration;const float Aura=FMath::Min(FMath::Clamp(Q/.17f,0.f,1.f),FMath::Clamp((1-Q)/.18f,0.f,1.f));
        const auto Trail=IsoDungeon::Project(H->EvadeDirection)*Zoom;
        for(int I=5;I>=1;--I)Body(-Trail*(I*.075f),FLinearColor(.04f,.55f,1.f,Aura*.11f*(6-I)),BLEND_Additive);
        for(int I=0;I<8;++I){const float A=I*PI/4;Body(FVector2D(FMath::Cos(A)*3,FMath::Sin(A)*3)*Zoom,FLinearColor(.06f,.6f,1.f,Aura*.35f),BLEND_Additive);}
        Body({},FLinearColor(.12f,.8f,1.f,Aura*.95f),BLEND_Additive);
        Mist(H->PhaseOrigin,H->EvadeAge,.32f);Mist(H->Position,H->EvadeAge-(H->PhaseDuration-.20f),.48f);
        Tint.A*=1-Aura;Body({},Tint,BLEND_Translucent);
    }else{Body({},Tint,BLEND_Translucent);if(H->PhaseAfter>0)Mist(H->PhaseEnd,.48f-H->PhaseAfter,.48f);}
}
