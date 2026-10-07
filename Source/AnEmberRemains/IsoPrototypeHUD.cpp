#include "IsoPrototype.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "CanvasItem.h"
#include "RenderUtils.h"
#include "Kismet/GameplayStatics.h"
#include "FullBodyArtMetrics.h"

using namespace IsoDungeon;
UTexture2D* AIsoPrototypeHUD::Texture(const FString& Name){
    if(auto* T=Textures.Find(Name))return *T;
    auto* T=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Art/V2/%s.%s"),*Name,*Name));Textures.Add(Name,T);return T;
}
FVector2D AIsoPrototypeHUD::Screen(FVector2D P) const{return Offset+(IsoDungeon::Project(P-CameraCenter)+FVector2D(640,440))*Zoom;}
void AIsoPrototypeHUD::Polygon(const TArray<FVector2D>& P,FLinearColor Color){
    for(int I=1;I+1<P.Num();++I){FCanvasTriangleItem T(P[0],P[I],P[I+1],GWhiteTexture);T.SetColor(Color);T.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(T);}
}
void AIsoPrototypeHUD::Label(const FString& Text,FVector2D P,FLinearColor Color,float Size){DrawText(Text,Color,Offset.X+P.X*Zoom,Offset.Y+P.Y*Zoom,GEngine->GetSmallFont(),Size*Zoom,false);}
void AIsoPrototypeHUD::Tile(FIntPoint Cell,FLinearColor Color,float Height){
    TArray<FVector2D> Points;for(const auto V:{FVector2D(0,0),FVector2D(1,0),FVector2D(1,1),FVector2D(0,1)})Points.Add(Screen(Rotate45(FVector2D(Cell)+V))-FVector2D(0,Height*Zoom));
    if(Height>0){for(int I=1;I<3;++I)Polygon({Points[I],Points[I+1],Points[I+1]+FVector2D(0,Height*Zoom),Points[I]+FVector2D(0,Height*Zoom)},Color*(I==1?.55f:.7f));}
    Polygon(Points,Color);for(int I=0;I<4;++I)DrawLine(Points[I].X,Points[I].Y,Points[(I+1)%4].X,Points[(I+1)%4].Y,FLinearColor(.035f,.05f,.055f,.8f),Zoom);
}
void AIsoPrototypeHUD::Hero(AIsoPrototypePawn* H){
    using namespace FullBodyArt;const int D=H->Facing;
    int C=H->Moving?(H->Sprinting?Run:Walk):Idle,F=H->Moving?int(H->WalkDistance*4)%8:int(GetWorld()->GetTimeSeconds()*5)%8;
    if(H->AttackAge>=0){C=TeaThrow;F=H->AttackAge<.22f?FMath::Clamp(int(H->AttackAge/.22f*4),0,3):FMath::Clamp(4+int((H->AttackAge-.22f)/.26f*4),4,7);}
    if(H->Health<=0){C=DeathRecover;F=4;}
    FString Name=Names[C*8+D];auto* T=Texture(Name);if(!T)return;
    const float K=.49f*Zoom;const auto P=Screen(H->Position),TL=P-FVector2D(128,232)*K;
    DrawTexture(T,TL.X,TL.Y,256*K,256*K,(F%4)*.25f,(F/4)*.5f,.25f,.5f,H->Hurt>0?FLinearColor(1,.5,.4):FLinearColor::White,BLEND_Translucent);
}
void AIsoPrototypeHUD::DrawHUD(){
    Super::DrawHUD();auto* G=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));if(!G||!H)return;
    Zoom=FMath::Min(Canvas->ClipX/1280.f,Canvas->ClipY/800.f);Offset={(Canvas->ClipX-1280*Zoom)/2,(Canvas->ClipY-800*Zoom)/2};CameraCenter=H->CameraCenter;
    DrawRect(FLinearColor(.009,.014,.019),0,0,Canvas->ClipX,Canvas->ClipY);
    auto Visible=[&](FIntPoint C){const auto P=Screen(Center(C));return P.X>-100*Zoom&&P.Y>-80*Zoom&&P.X<Canvas->ClipX+100*Zoom&&P.Y<Canvas->ClipY+180*Zoom;};
    auto Known=[&](FIntPoint C){return G->Grid.Floor(C.X,C.Y)&&G->Seen.IsValidIndex(C.Y*FGrid::Size+C.X)&&G->Seen[C.Y*FGrid::Size+C.X];};
    for(int Sum=0;Sum<FGrid::Size*2;++Sum)for(int X=0;X<FGrid::Size;++X){const int Y=Sum-X;FIntPoint C(X,Y);if(Y<0||Y>=FGrid::Size||!Visible(C)||!Known(C))continue;
        const float Variation=((X*17+Y*31)%9)*.008f;const float Dist=(Center(C)-H->Position).Size();const float Light=FMath::Clamp(1.15f-Dist*.06f,.35f,1.f);
        Tile(C,FLinearColor(.16f+Variation,.15f+Variation,.135f+Variation)*Light);
        if(C==G->Grid.Exit){const auto P=Screen(Center(C));DrawRect(FLinearColor(.1,.8,.55,.65),P.X-14*Zoom,P.Y-7*Zoom,28*Zoom,14*Zoom);}
    }
    // Front walls and actors share a painter-depth queue. Nearby front walls
    // become translucent so the hero never disappears behind a corridor edge.
    struct Entry{float Depth;int Kind,Index;FIntPoint Cell;};TArray<Entry> Draws;
    for(int Y=1;Y<FGrid::Size-1;++Y)for(int X=1;X<FGrid::Size-1;++X){FIntPoint C(X,Y);if(G->Grid.Floor(X,Y)||!Visible(C))continue;bool Border=false;for(auto S:{FIntPoint(1,0),FIntPoint(-1,0),FIntPoint(0,1),FIntPoint(0,-1)})Border|=Known(C+S);if(Border)Draws.Add({float(X+Y+1),0,0,C});}
    Draws.Add({float(Grid(H->Position).X+Grid(H->Position).Y),1,0,{}});
    for(int I=0;I<G->Enemies.Num();++I){const auto& E=G->Enemies[I];if(Known(Cell(E.P))&&(E.Death<0||E.Death<1.2))Draws.Add({float(Grid(E.P).X+Grid(E.P).Y),2,I,{}});}
    Draws.Sort([](const Entry& A,const Entry& B){return A.Depth<B.Depth;});
    for(const auto& D:Draws){
        if(D.Kind==0){auto Color=FLinearColor(.24,.29,.28);if((Center(D.Cell)-H->Position).Size()<2.8)Color.A=.32;Tile(D.Cell,Color,44);continue;}
        const auto P=Screen(D.Kind==1?H->Position:G->Enemies[D.Index].P);Polygon({P+FVector2D(-21,0)*Zoom,P+FVector2D(0,-7)*Zoom,P+FVector2D(21,0)*Zoom,P+FVector2D(0,7)*Zoom},FLinearColor(0,0,0,.4));
        if(D.Kind==1){Hero(H);continue;}
        const auto& E=G->Enemies[D.Index];const int Frame=int(E.Walk*5)%4;const FString Name=FString::Printf(TEXT("%s_0_%d"),E.Attack>=0?TEXT("EnemyAttack"):TEXT("EnemyWalk"),E.Attack>=0?FMath::Clamp(int(E.Attack/.9f*4),0,3):Frame);
        const float Alpha=E.Death<0?1:FMath::Clamp(1-E.Death/1.2f,0.f,1.f);auto* T=Texture(Name);if(T)DrawTexture(T,P.X-45*Zoom,P.Y-82*Zoom,90*Zoom,90*Zoom,E.Aim.X<0?1:0,0,E.Aim.X<0?-1:1,1,E.Hurt>0?FLinearColor(1,.45,.3,Alpha):FLinearColor(1,1,1,Alpha),BLEND_Translucent);
        if(E.HP>0){DrawRect(FLinearColor(.05,.02,.02),P.X-20*Zoom,P.Y-82*Zoom,40*Zoom,3*Zoom);DrawRect(FLinearColor(.7,.13,.07),P.X-20*Zoom,P.Y-82*Zoom,40*Zoom*E.HP/70,3*Zoom);}
        if(E.Attack>=0&&E.Attack<.55){DrawRect(FLinearColor(1,.3,.1),P.X-18*Zoom,P.Y+8*Zoom,36*Zoom*E.Attack/.55f,3*Zoom);}
    }
    for(const auto& B:G->Bolts){const auto P=Screen(B.P)-FVector2D(0,35*Zoom),Tail=Screen(B.P-B.Velocity*.06)-FVector2D(0,35*Zoom);DrawLine(Tail.X,Tail.Y,P.X,P.Y,FLinearColor(1,.22f,.025f,.7f),7*Zoom);Polygon({P+FVector2D(-7,0)*Zoom,P+FVector2D(0,-7)*Zoom,P+FVector2D(7,0)*Zoom,P+FVector2D(0,7)*Zoom},FLinearColor(1,.7f,.25f));}
    for(const auto& N:G->Numbers){const auto P=Screen(N.P)-FVector2D(0,(65+N.Age*35)*Zoom);DrawText(FString::FromInt(FMath::Abs(N.Value)),N.Value<0?FLinearColor(1,.3,.25,1-N.Age):FLinearColor(1,.85,.5,1-N.Age),P.X,P.Y,GEngine->GetSmallFont(),1.3f*Zoom);}
    DrawRect(FLinearColor(.008,.015,.019,.93),Offset.X,Offset.Y,1280*Zoom,68*Zoom);
    Label(TEXT("AN EMBER REMAINS  |  THE HUSHED CLOISTERS"),{22,12},FLinearColor(.95f,.65f,.3f),1.2);
    Label(FString::Printf(TEXT("%s  |  Seed %d  |  Placeholder art"),*G->RoomName(Cell(H->Position)),G->Seed),{22,39},FLinearColor(.75f,.7f,.62f));
    DrawRect(FLinearColor(.15,.02,.02),Offset.X+1010*Zoom,Offset.Y+19*Zoom,240*Zoom,18*Zoom);DrawRect(FLinearColor(.75,.09,.08),Offset.X+1010*Zoom,Offset.Y+19*Zoom,240*Zoom*H->Health/150,18*Zoom);
    Label(FString::Printf(TEXT("Health %.0f / 150"),H->Health),{1060,41},FLinearColor::White);
    DrawRect(FLinearColor(.008,.015,.019,.93),Offset.X,Offset.Y+760*Zoom,1280*Zoom,40*Zoom);
    Label(TEXT("WASD move  |  Shift sprint  |  Hold LMB cast ember  |  M map  |  R reset seed  |  Esc pause"),{22,775},FLinearColor(.75,.8,.75));
    if((H->Position-Center(G->Grid.Exit)).Size()<2)Label(TEXT("Caldris's Sanctum - E to end prototype"),{450,660},FLinearColor(1,.7f,.3f),1.2);
    if(G->MapOpen){DrawRect(FLinearColor(.005,.018,.02,.96),Offset.X+220*Zoom,Offset.Y+80*Zoom,840*Zoom,660*Zoom);Label(TEXT("THE HUSHED CLOISTERS - EXPLORED MAP"),{380,99},FLinearColor(.95f,.65f,.3f),1.3);
        for(int Y=0;Y<FGrid::Size;++Y)for(int X=0;X<FGrid::Size;++X)if(Known({X,Y}))DrawRect(FLinearColor(.23,.47,.39),Offset.X+(385+X*8)*Zoom,Offset.Y+(165+Y*8)*Zoom,7*Zoom,7*Zoom);
        const auto C=Cell(H->Position);DrawRect(FLinearColor(1,.8,.25),Offset.X+(385+C.X*8)*Zoom,Offset.Y+(165+C.Y*8)*Zoom,8*Zoom,8*Zoom);Label(TEXT("M to return"),{575,700},FLinearColor::White);
    }
    if(H->Health<=0||G->Won||G->Paused){DrawRect(FLinearColor(0,0,0,.8),Offset.X+350*Zoom,Offset.Y+300*Zoom,580*Zoom,150*Zoom);Label(H->Health<=0?TEXT("YOU HAVE FALLEN"):G->Won?TEXT("THE DEPTHS HAVE BEEN EXPLORED"):TEXT("PAUSED"),{425,335},FLinearColor(1,.8,.5),1.6);Label(G->Paused?TEXT("Esc to resume — close the window to quit"):TEXT("R to explore a new seed"),{420,395},FLinearColor::White);}
}
