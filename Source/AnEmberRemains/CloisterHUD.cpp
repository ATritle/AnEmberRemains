#include "IsoPrototype.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "MaraArtMetrics.h"
#include "Engine/Texture2D.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "SpellFX.h"
#include "Engine/TextureRenderTarget2D.h"
#include "CanvasItem.h"
using namespace IsoDungeon;

void AIsoPrototypeHUD::DrawHUD()
{
    Super::DrawHUD();auto* G=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));if(!G||!H)return;
    Zoom=FMath::Min(Canvas->ClipX/1280.f,Canvas->ClipY/800.f);Offset={(Canvas->ClipX-1280*Zoom)/2,(Canvas->ClipY-800*Zoom)/2};CameraCenter=H->CameraCenter;
    DrawRect(FLinearColor(.006f,.009f,.012f),0,0,Canvas->ClipX,Canvas->ClipY);
    if(FParse::Param(FCommandLine::Get(),TEXT("MaraGallery"))){
        int Frame=0;FParse::Value(FCommandLine::Get(),TEXT("MaraFrame="),Frame);Frame=FMath::Clamp(Frame,0,5);
        const TCHAR* Actions[]={TEXT("IDLE"),TEXT("WALK"),TEXT("RUN"),TEXT("DODGE"),TEXT("ROLL"),TEXT("CAST")};
        for(int A=0;A<6;++A)Label(Actions[A],{120.+A*190,8},FLinearColor::White);
        for(int D=0;D<8;++D){Label(MaraArt::Names[D],{8,57.+D*92},FLinearColor(.6f,.8f,.8f));auto* T=Texture(MaraArt::Names[D]);if(!T)continue;
            for(int A=0;A<6;++A){const auto& R=MaraArt::Frames[D][A*6+Frame];const float K=MaraArt::Scale[D]*Zoom*.68f;const auto P=Offset+FVector2D(142+A*190,111+D*92)*Zoom;
                DrawTexture(T,P.X-R.PX*K,P.Y-R.PY*K,R.W*K,R.H*K,R.X/T->GetSizeX(),R.Y/T->GetSizeY(),R.W/T->GetSizeX(),R.H/T->GetSizeY(),FLinearColor::White,BLEND_Translucent);
            }
        }return;
    }
    const float Time=GetWorld()->GetTimeSeconds();
    PrepareLighting(G,H);
    auto Visible=[&](FVector2D World){const auto P=Screen(World);return P.X>-220*Zoom&&P.Y>-100*Zoom&&P.X<Canvas->ClipX+220*Zoom&&P.Y<Canvas->ClipY+300*Zoom;};
    auto Reveal=[&](FIntPoint C){return G->Grid.Floor(C.X,C.Y)&&G->Reveal.IsValidIndex(C.Y*FGrid::Size+C.X)?G->Reveal[C.Y*FGrid::Size+C.X]:0.f;};
    for(int Sum=0;Sum<FGrid::Size*2;++Sum)for(int X=0;X<FGrid::Size;++X){const int Y=Sum-X;FIntPoint C(X,Y);const float Alpha=Reveal(C);if(Alpha<=0||!Visible(Center(C)))continue;
        const auto World=Center(C);auto Color=Shade(World);Color.A=Alpha;const int RI=G->RoomIndex(C);
        if(RI>=0){const auto& R=G->Grid.Rooms[RI];
            // Chapel/processional inlays are part of the same world-space floor,
            // with no separate floating carpet or visible per-cell grid outline.
            const bool Edge=C.X==R.X||C.Y==R.Y||C.X==R.X+R.W-1||C.Y==R.Y+R.H-1;
            if(Edge){Color.R*=.63f;Color.G*=.62f;Color.B*=.57f;}
            if((RI==0||((RI-1)%4)==0||R.Center()==G->Grid.Exit)&&FMath::Abs(C.X-R.Center().X)<=1){Color.R*=.75f;Color.G*=.42f;Color.B*=.38f;}
        }
        Tile(C,Color);
        // Soft contact shade where floor meets masonry.
        for(const auto Step:{FIntPoint(1,0),FIntPoint(0,1),FIntPoint(-1,0),FIntPoint(0,-1)})if(!G->Grid.Floor(X+Step.X,Y+Step.Y)){
            auto P=Screen(World+Rotate45(FVector2D(Step)*.42));Glow(P,26,FLinearColor(0,0,0,Alpha*.8f));
        }
    }
    // Candle light flickers independently, without consuming gameplay randomness.
    for(const auto& D:G->Details)if(D.Art<4&&Reveal(Cell(D.P))>0&&Visible(D.P))FloorDetail(D,Reveal(Cell(D.P)),G->AmbientTime);
    for(const auto& P:G->Props)if(P.Art==0&&Reveal(P.Cell)>0&&Visible(Center(P.Cell)))Glow(Screen(Center(P.Cell)),160,FLinearColor(1,.38f,.075f,Reveal(P.Cell)*(.9f+.22f*FMath::Sin(G->AmbientTime*5.1f+P.Cell.X))));
    if(Reveal(G->Grid.Exit)>0){const auto P=Screen(Center(G->Grid.Exit));DrawTexture(Texture(TEXT("CloisterStairs")),P.X-140*Zoom,P.Y-108*Zoom,280*Zoom,187*Zoom,0,0,1,1,FLinearColor(.7f,.65f,.53f,Reveal(G->Grid.Exit)),BLEND_Translucent);}
    struct Entry{float Depth;int Kind,Index;FIntPoint Cell;float Alpha;};TArray<Entry> Draws;
    for(int Y=1;Y<FGrid::Size-1;++Y)for(int X=1;X<FGrid::Size-1;++X){FIntPoint C(X,Y);if(G->Grid.Floor(X,Y)||!Visible(Center(C)))continue;float A=0;for(auto S:{FIntPoint(1,0),FIntPoint(-1,0),FIntPoint(0,1),FIntPoint(0,-1)})A=FMath::Max(A,Reveal(C+S));if(A>0)Draws.Add({float(X+Y+1),0,0,C,A});}
    const auto HeroGrid=Grid(H->Position);Draws.Add({float(HeroGrid.X+HeroGrid.Y),1,0,{},1});
    for(int I=0;I<G->Props.Num();++I){const auto& P=G->Props[I];const float A=Reveal(P.Cell);const float BaseOffset=P.Art==1?-1.99f*(P.Size/170.f):0.f;if(A>0&&Visible(Center(P.Cell)))Draws.Add({float(P.Cell.X+P.Cell.Y+1)+BaseOffset,2,I,P.Cell,A});}
    // Arch art retained in the project, but omitted from this exploration build.
    for(int I=0;I<G->Details.Num();++I){const auto& D=G->Details[I];const float A=Reveal(Cell(D.P));if(D.Art>=4&&A>0&&Visible(D.P)){const auto C=Grid(D.P);Draws.Add({float(C.X+C.Y),4,I,{},A});}}
    for(int I=0;I<G->Rats.Num();++I){const auto& R=G->Rats[I];const float A=Reveal(Cell(R.P));if(A>0&&Visible(R.P)&&G->Grid.Sight(H->Position,R.P)){const auto C=Grid(R.P);Draws.Add({float(C.X+C.Y),5,I,{},A});}}
    Draws.Sort([](const Entry& A,const Entry& B){return A.Depth==B.Depth?A.Kind<B.Kind:A.Depth<B.Depth;});
    if(auto* FX=ASpellFX::Find(GetWorld(),false))if(auto* M=FX->Composite(CameraCenter,true))DrawMaterialSimple(M,Offset.X,Offset.Y,1280*Zoom,800*Zoom);
    // Fade by projected overlap with the whole character, not just distance
    // from his feet. Isometric walls can hide the torso from several cells away.
    auto Occlusion=[&](FVector2D W,float HalfWidth,float Height){
        const auto Delta=IsoDungeon::Project(W-H->Position);if(Delta.Y<=0||Delta.Y>Height+70)return 1.f;
        const float X=FMath::Clamp((FMath::Abs(float(Delta.X))-HalfWidth)/(70.f),0.f,1.f);
        const float Y=FMath::Clamp((float(Delta.Y)-Height)/70.f,0.f,1.f);
        const float Edge=FMath::Max(X,Y);const float Smooth=Edge*Edge*(3-2*Edge);
        return FMath::Lerp(.055f,1.f,Smooth);
    };
    for(const auto& D:Draws){
        if(D.Kind==4){FloorDetail(G->Details[D.Index],D.Alpha,G->AmbientTime);continue;}
        if(D.Kind==5){Rat(G->Rats[D.Index],D.Alpha);continue;}
        if(D.Kind==0){const bool Blocked=WallOccludes(D.Cell,H->Position);const int Key=D.Cell.Y*FGrid::Size+D.Cell.X;
            if(!WallOpacity.Contains(Key))WallOpacity.Add(Key,1.f);float& A=WallOpacity[Key];
            if(Center(D.Cell).Y<=H->Position.Y+.01)A=1.f;else A=FMath::FInterpTo(A,Blocked?.055f:1.f,GetWorld()->GetDeltaSeconds(),12.f);
            FLinearColor Color(.12f,.145f,.18f);for(auto S:{FIntPoint(1,0),FIntPoint(-1,0),FIntPoint(0,1),FIntPoint(0,-1)})if(G->Grid.Floor(D.Cell.X+S.X,D.Cell.Y+S.Y)){const auto L=Shade(Center(D.Cell+S));Color.R=FMath::Max(Color.R,L.R);Color.G=FMath::Max(Color.G,L.G);Color.B=FMath::Max(Color.B,L.B);}Color.A=D.Alpha*A;
            Tile(D.Cell,Color,115);continue;}
        if(D.Kind==1){Glow(Screen(H->Position),27,FLinearColor(0,0,0,2));Hero(H);continue;}
        if(D.Kind==2){const auto& P=G->Props[D.Index];const auto Base=Center(P.Cell)+(P.Art==1?Rotate45(FVector2D(-1.025,-.965)*(P.Size/170.f)):FVector2D::ZeroVector);Prop(P,D.Alpha*Occlusion(Base,P.Size*.35f,P.Size));continue;}
        const auto& A=G->Arches[D.Index];Arch(A,D.Alpha*Occlusion(A.P,160,275));
    }
    Atmosphere(G,H);
    if(auto* FX=ASpellFX::Find(GetWorld(),false))if(auto* M=FX->Composite(CameraCenter))DrawMaterialSimple(M,Offset.X,Offset.Y,1280*Zoom,800*Zoom);
    for(const auto& N:G->Numbers){const auto P=(Screen(N.P)-Offset)/Zoom-FVector2D(10,70+N.Age*35);auto Color=N.Healing?FLinearColor(.2f,1,.5f,1-N.Age):N.Value<0?FLinearColor(1,.2f,.1f,1-N.Age):FLinearColor(.8f,.9f,1,1-N.Age);Label(FString::Printf(TEXT("%s%d"),N.Healing?TEXT("+"):TEXT(""),FMath::Abs(N.Value)),P,Color,1.3f);}
    const TCHAR* Spells[]={TEXT("ARCANE"),TEXT("FLAME"),TEXT("ICE RING"),TEXT("LIGHTNING"),TEXT("FROST NOVA"),TEXT("MEND")};
    for(int I=0;I<6;++I){const FVector2D P(282+I*120,709);const bool Cooling=H->SpellCooldown[I]>0;
        DrawRect(FLinearColor(.012f,.022f,.03f,.84f),Offset.X+P.X*Zoom,Offset.Y+P.Y*Zoom,113*Zoom,47*Zoom);
        Label(FString::Printf(TEXT("%d  %s"),I+1,Spells[I]),P+FVector2D(7,6),Cooling?FLinearColor(.45f,.51f,.56f):FLinearColor(.6f,.87f,.88f),.9f);
        Label(Cooling?FString::Printf(TEXT("%.1fs"),H->SpellCooldown[I]):(I==1||I==3)?TEXT("HOLD"):TEXT("READY"),P+FVector2D(7,26),FLinearColor(.63f,.63f,.56f),.85f);
    }
    if(G->SpellHintTime>0)Label(G->SpellHint,{310,684},FLinearColor(.67f,.87f,1.f),.95f);
    if(H->IsIceProtected())Label(FString::Printf(TEXT("PROTECTED  %.1fs"),G->IceGuardTime),{565,661},FLinearColor(.5f,.85f,1.f),1.1f);
    // Quiet exploration HUD; no combat health panel in this enemy-free art test.
    DrawRect(FLinearColor(.008f,.009f,.012f,.9f),Offset.X,Offset.Y,1280*Zoom,72*Zoom);
    Label(TEXT("A N   E M B E R   R E M A I N S"),{28,13},FLinearColor(.91f,.7f,.42f),1.3f);
    Label(FString::Printf(TEXT("HUSHED CLOISTERS / DEPTH %d   -   %s"),G->FloorNumber,*G->RoomName(Cell(H->Position)).ToUpper()),{28,43},FLinearColor(.65f,.65f,.6f),1.0f);
    // Player-centred circular radar, with genuinely clipped explored geometry.
    const FVector2D MiniCenter(1186,98);constexpr float MiniRadius=73;
    auto Mini=[&](FVector2D W){return IsoDungeon::Project(W-H->Position)*.11;};
    auto MiniDraw=[&](const TArray<FVector2D>& Points,FLinearColor Color){TArray<FVector2D> Q;for(auto P:Points)Q.Add(Offset+(MiniCenter+P)*Zoom);Polygon(Q,Color);};
    TArray<FVector2D> Disc;for(int I=0;I<64;++I){const float A=I*2*PI/64;Disc.Add(FVector2D(FMath::Cos(A),FMath::Sin(A))*MiniRadius);}MiniDraw(Disc,FLinearColor(.012f,.018f,.025f,.42f));
    for(int I=0;I<64;++I){const auto A=Disc[I],B=Disc[(I+1)%64];MiniDraw({A,B,B*1.025,A*1.025},FLinearColor(.48f,.40f,.25f,.32f));}
    auto ClipCircle=[&](TArray<FVector2D> Q){
        for(int I=0;I<64&&!Q.IsEmpty();++I){const float A=(I+.5f)*2*PI/64;const FVector2D N(FMath::Cos(A),FMath::Sin(A));const float Limit=MiniRadius*FMath::Cos(PI/64);TArray<FVector2D> Out;
            auto Prev=Q.Last();double PD=FVector2D::DotProduct(Prev,N)-Limit;
            for(auto P:Q){const double D=FVector2D::DotProduct(P,N)-Limit;if((D<=0)!=(PD<=0))Out.Add(FMath::Lerp(Prev,P,PD/(PD-D)));if(D<=0)Out.Add(P);Prev=P;PD=D;}Q=MoveTemp(Out);
        }return Q;
    };
    for(int Y=0;Y<FGrid::Size;++Y)for(int X=0;X<FGrid::Size;++X)if(G->Seen[Y*FGrid::Size+X]){
        const auto P=Mini(Center({X,Y}));if(P.Size()>MiniRadius+10)continue;TArray<FVector2D> Q;
        for(auto V:{FVector2D(0,0),FVector2D(1,0),FVector2D(1,1),FVector2D(0,1)})Q.Add(Mini(Rotate45(FVector2D(X,Y)+V)));
        MiniDraw(ClipCircle(Q),FLinearColor(.83f,.54f,.29f,.52f));
    }
    // No markers through unexplored walls: alive, nearby and currently detected.
    for(const auto& E:G->Enemies){const auto C=Cell(E.P);if(E.HP<=0||(E.P-H->Position).Size()>11||!G->Grid.Floor(C.X,C.Y)||!G->Seen[C.Y*FGrid::Size+C.X]||!G->Grid.Sight(H->Position,E.P))continue;
        const auto P=Mini(E.P);if(P.Size()>MiniRadius-5)continue;MiniDraw({P+FVector2D(0,-3),P+FVector2D(3,0),P+FVector2D(0,3),P+FVector2D(-3,0)},FLinearColor(1,.08f,.055f,1));
    }
    const float Heading=H->Facing*PI/4;const FVector2D Forward(FMath::Sin(Heading),-FMath::Cos(Heading)),Side(-Forward.Y,Forward.X);
    MiniDraw({Forward*7,-Forward*5+Side*5,-Forward*2,-Forward*5-Side*5},FLinearColor(.06f,.07f,.08f,1));
    MiniDraw({Forward*5,-Forward*3+Side*3,-Forward,-Forward*3-Side*3},FLinearColor(1,.96f,.78f,1));
    DrawRect(FLinearColor(.008f,.009f,.012f,.86f),Offset.X,Offset.Y+765*Zoom,1280*Zoom,35*Zoom);
    Label(TEXT("WASD Move   SHIFT Run   1-6 Spells / aim cursor   LMB Arcane   SPACE Phase   M Map   ESC Pause   R New layout"),{28,778},FLinearColor(.7f,.68f,.61f));
    if(G->Grid.AtStairEntry(H->Position)&&G->DescentTime<0)Label(TEXT("E  Descend to the lower cloisters"),{480,711},FLinearColor(1,.73f,.4f),1.1f);
    if(Reveal(G->Grid.Exit)>0){const auto S=Mini(Center(G->Grid.Exit));if(S.Size()<MiniRadius-6)for(int I=0;I<3;++I)MiniDraw({S+FVector2D(-4+I,-4+I*3),S+FVector2D(4-I,-4+I*3),S+FVector2D(4-I,-2+I*3),S+FVector2D(-4+I,-2+I*3)},FLinearColor(.7f,.86f,.9f,1));}
    if(G->MapOpen){
        DrawRect(FLinearColor(.013f,.016f,.02f,.98f),Offset.X+180*Zoom,Offset.Y+85*Zoom,920*Zoom,645*Zoom);
        Label(TEXT("THE HUSHED CLOISTERS"),{450,105},FLinearColor(.95f,.73f,.45f),1.4f);Label(TEXT("Only your explored paths are shown"),{468,134},FLinearColor(.65f,.65f,.6f));
        for(int Y=0;Y<FGrid::Size;++Y)for(int X=0;X<FGrid::Size;++X)if(Reveal({X,Y})>0)DrawRect(FLinearColor(.36f,.31f,.24f),Offset.X+(385+X*8)*Zoom,Offset.Y+(170+Y*8)*Zoom,7*Zoom,7*Zoom);
        for(int I=0;I<G->Grid.Rooms.Num();++I){const auto C=G->Grid.Rooms[I].Center();if(Reveal(C)>0){const FVector2D P(385+C.X*8,170+C.Y*8);Label(FString::FromInt(I+1),P-FVector2D(2,3),FLinearColor(.98f,.81f,.5f));}}
        const auto C=Cell(H->Position);DrawRect(FLinearColor(1,.52f,.13f),Offset.X+(385+C.X*8)*Zoom,Offset.Y+(170+C.Y*8)*Zoom,9*Zoom,9*Zoom);
        Label(TEXT("M to return  |  The warm mark is your position"),{444,698},FLinearColor(.8f,.75f,.64f));
    }
    const float Transition=G->DescentTime>=0?FMath::Clamp((G->DescentTime-.8f)/.8f,0.f,1.f):G->ArrivalFade;
    if(Transition>0)DrawRect(FLinearColor(0,0,0,Transition),0,0,Canvas->ClipX,Canvas->ClipY);
    if(!G->MapOpen&&!G->Paused&&!G->Won&&G->DescentTime<0){
        if(G->LoreIndex<0){for(const auto& D:G->Details)if(D.Art>=4&&(D.P-H->Position).Size()<1.7&&G->Grid.Sight(H->Position,D.P)){Label(TEXT("E  Inspect what was left behind"),{470,731},FLinearColor(.86f,.73f,.52f));break;}}
        else if(G->Details.IsValidIndex(G->LoreIndex)){
            const int Art=G->Details[G->LoreIndex].Art;
            DrawRect(FLinearColor(0,0,0,.58f),0,0,Canvas->ClipX,Canvas->ClipY);
            const int Note=FMath::Clamp(Art-4,0,3);
            DrawTexture(Texture(TEXT("CloisterNotes")),Offset.X+480*Zoom,Offset.Y+222*Zoom,320*Zoom,320*Zoom,(Note%2)*.5f,(Note/2)*.5f,.5f,.5f,FLinearColor::White,BLEND_Translucent);
            Label(TEXT("E / Esc  Put the note away"),{539,558},FLinearColor(.8f,.75f,.63f));
        }
    }
    if(G->Won||G->Paused){DrawRect(FLinearColor(0,0,0,.87f),Offset.X+310*Zoom,Offset.Y+295*Zoom,660*Zoom,165*Zoom);Label(G->Won?TEXT("BELOW THE BELLS"):TEXT("PAUSED"),{440,330},FLinearColor(.95f,.72f,.4f),1.8f);Label(G->Won?TEXT("Caldris awaits a later build. R explores a new layout."):TEXT("Esc to resume. Close the window to quit."),{370,400},FLinearColor(.8f,.8f,.75f));}
}
