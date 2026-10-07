#include "IsoPrototype.h"
using namespace IsoDungeon;

int AIsoPrototypeGameMode::RoomIndex(FIntPoint C) const
{
    for(int I=0;I<Grid.Rooms.Num();++I){const auto& R=Grid.Rooms[I];if(C.X>=R.X&&C.Y>=R.Y&&C.X<R.X+R.W&&C.Y<R.Y+R.H)return I;}
    return INDEX_NONE;
}
void AIsoPrototypeGameMode::DressFloor()
{
    Props.Empty();Arches.Empty();Grid.Obstructions.Empty();Grid.PropBases.Empty();TSet<int> Reserved;
    auto Place=[&](FIntPoint C,int Art,float Size){
        if(!Grid.Walkable(C.X,C.Y)||C==Grid.Start||C==Grid.Exit)return;
        // Preserve the central processional aisles and all room-center navigation.
        const int RI=RoomIndex(C);if(RI<0)return;const auto Mid=Grid.Rooms[RI].Center();
        if(FMath::Abs(C.X-Mid.X)<=1||FMath::Abs(C.Y-Mid.Y)<=1)return;
        const bool Large=Art==1||Art==2||Art==3;const int Radius=Large?1:0;TArray<int> Added;
        for(int Y=-Radius;Y<=Radius;++Y)for(int X=-Radius;X<=Radius;++X){const FIntPoint P=C+FIntPoint(X,Y);if(!Grid.Walkable(P.X,P.Y)||Reserved.Contains(P.Y*FGrid::Size+P.X)||P==Grid.Start||P==Grid.Exit)return;}
        // The coffin's rendered anchor is its bottom edge, not its base center.
        // Match the stone plinth (170px artwork), keeping generous art placement
        // clearance separate from the player's precise collision footprint.
        if(Art==1){const FVector2D Origin(C.X+.5,C.Y+.5);const double Scale=Size/170.;Grid.PropBases.Add(FBox2D(Origin+FVector2D(-2.3,-1.5)*Scale,Origin+FVector2D(.25,-.43)*Scale));}
        for(int Y=-Radius;Y<=Radius;++Y)for(int X=-Radius;X<=Radius;++X){const int Key=(C.Y+Y)*FGrid::Size+C.X+X;Added.Add(Key);if(Art!=1)Grid.Obstructions.Add(Key);}
        const auto D=Grid.Distances(Grid.Start);bool Connected=true;
        for(int I=0;I<Grid.Tiles.Num();++I)if(Grid.Walkable(I%FGrid::Size,I/FGrid::Size)&&D[I]<0){Connected=false;break;}
        if(!Connected){for(int Key:Added)Grid.Obstructions.Remove(Key);if(Art==1)Grid.PropBases.Pop();return;}
        for(int Key:Added)Reserved.Add(Key);
        Props.Add({C,Art,Size});
    };
    for(int I=0;I<Grid.Rooms.Num();++I){
        const auto& R=Grid.Rooms[I];const bool Sanctum=R.Center()==Grid.Exit;
        const int Art=Sanctum?5:I==0?7:((I-1)%4==0?1:(I-1)%4==1?2:(I-1)%4==2?4:3);
        const float Size=Art==1?170:Art==3?155:Art==2?155:145;
        const int Inset=(Art==1||Art==2||Art==3)?3:1;
        Place({R.X+Inset,R.Y+Inset},Art,Size);Place({R.X+R.W-2,R.Y+1},0,118);
        Place({R.X+1,R.Y+R.H-2},0,118);Place({R.X+R.W-1-Inset,R.Y+R.H-1-Inset},Art,Size);
        if(R.W>10&&R.H>10){Place({R.X+3,R.Y+1},4,155);Place({R.X+R.W-4,R.Y+R.H-2},6,90);}
        // An arch only spans a real opening, never solid wall or an entire room.
        // Detect each contiguous threshold once rather than drawing per floor cell.
        for(int Side=0;Side<4;++Side){
            const int Length=Side%2?R.H:R.W;int Begin=-1;
            auto Outside=[&](int K){return Side==0?FIntPoint(R.X+K,R.Y-1):Side==1?FIntPoint(R.X+R.W,R.Y+K):Side==2?FIntPoint(R.X+K,R.Y+R.H):FIntPoint(R.X-1,R.Y+K);};
            for(int K=0;K<=Length;++K){
                const auto C=Outside(K);const bool Open=K<Length&&Grid.Floor(C.X,C.Y);
                if(Open&&Begin<0)Begin=K;
                if(!Open&&Begin>=0){const int Width=K-Begin;if(Width>=5&&Width<=6){
                    const double Along=(Begin+K)*.5;
                    const FVector2D P=Side==0?FVector2D(R.X+Along,R.Y):Side==1?FVector2D(R.X+R.W,R.Y+Along):Side==2?FVector2D(R.X+Along,R.Y+R.H):FVector2D(R.X,R.Y+Along);
                    const auto World=Rotate45(P);bool Duplicate=false;for(const auto& A:Arches)if((A.P-World).Size()<2)Duplicate=true;
                    if(!Duplicate)Arches.Add({World,Side%2==0});
                }Begin=-1;}
            }
        }
    }
}
