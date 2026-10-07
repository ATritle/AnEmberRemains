#pragma once
#include "CoreMinimal.h"

// Standalone prototype math: grid cells -> 45-degree world plane -> 2:1 canvas.
namespace IsoDungeon
{
inline FVector2D Rotate45(FVector2D P) { return {(P.X-P.Y)*UE_INV_SQRT_2,(P.X+P.Y)*UE_INV_SQRT_2}; }
inline FVector2D Grid(FVector2D P) { return {(P.X+P.Y)*UE_INV_SQRT_2,(P.Y-P.X)*UE_INV_SQRT_2}; }
inline FVector2D Project(FVector2D P) { return {P.X*64,P.Y*32}; }
inline FVector2D Unproject(FVector2D P) { return {P.X/64,P.Y/32}; }
inline FVector2D Input(float Right,float Down) { return Rotate45(FVector2D(Right,Down).GetClampedToMaxSize(1)); }
inline int Direction(FVector2D World) { const auto P=Project(World);return (FMath::RoundToInt(FMath::Atan2(P.X,-P.Y)/(PI/4))+8)%8; }
inline FIntPoint Cell(FVector2D World) { const auto G=Grid(World);return {int32(FMath::FloorToInt(G.X)),int32(FMath::FloorToInt(G.Y))}; }
inline FVector2D Center(FIntPoint P) {return Rotate45(FVector2D(P.X+.5,P.Y+.5));}
inline bool WallOccludes(FIntPoint Wall,FVector2D Player){
    if(Center(Wall).Y<=Player.Y+.01)return false;
    TArray<FVector2D> Top;for(auto V:{FVector2D(0,0),FVector2D(1,0),FVector2D(1,1),FVector2D(0,1)})Top.Add(Project(Rotate45(FVector2D(Wall)+V)-Player)-FVector2D(0,115));
    auto Inside=[](FVector2D P,const TArray<FVector2D>& Q){bool Pos=false,Neg=false;for(int I=0;I<4;++I){const auto A=Q[I],B=Q[(I+1)%4];const double C=(B.X-A.X)*(P.Y-A.Y)-(B.Y-A.Y)*(P.X-A.X);Pos|=C>.001;Neg|=C<-.001;}return !(Pos&&Neg);};
    // Test the rendered cap and two visible faces against the character body.
    for(float X:{-18.f,0.f,18.f})for(float Y:{-15.f,-50.f,-85.f}){const FVector2D P(X,Y);if(Inside(P,Top))return true;for(int I=1;I<3;++I)if(Inside(P,{Top[I],Top[I+1],Top[I+1]+FVector2D(0,115),Top[I]+FVector2D(0,115)}))return true;}
    return false;
}
struct FRoom { int X,Y,W,H;FIntPoint Center() const{return {X+W/2,Y+H/2};} };

struct FGrid
{
    static constexpr int Size=64;
    TArray<uint8> Tiles;
    TArray<FRoom> Rooms;
    TSet<int> Obstructions;
    // Continuous grid-space bases, independent of the larger placement clearance.
    TArray<FBox2D> PropBases;
    bool PropBlocks(FVector2D World,float Radius=.22f) const{
        const auto P=Grid(World);
        for(const auto& B:PropBases)if(P.X+Radius>B.Min.X&&P.X-Radius<B.Max.X&&P.Y+Radius>B.Min.Y&&P.Y-Radius<B.Max.Y)return true;
        return false;
    }
    FIntPoint Start,Exit;
    bool StairsActive=false;
    FVector2D StairEntry() const{return Center(Exit)+Rotate45(FVector2D(-.1,1.7));}
    bool StairBlocks(FVector2D World,float Radius=0) const{
        if(!StairsActive)return false;const auto P=Grid(World)-FVector2D(Exit.X+.5,Exit.Y+.5);
        auto Box=[&](double L,double T,double R,double B){return P.X+Radius>L&&P.X-Radius<R&&P.Y+Radius>T&&P.Y-Radius<B;};
        // Measured footprint of the rendered stair surround. The deep steps
        // and pit are solid to normal movement; only the first-step alcove opens.
        return Box(-1.3,-2.3,1.1,1.2)||Box(-1.3,1.2,-.8,1.85)||Box(.6,1.2,1.1,1.85);
    }
    bool AtStairEntry(FVector2D World) const{const auto P=Grid(World)-FVector2D(Exit.X+.5,Exit.Y+.5);return StairsActive&&P.X>=-.48&&P.X<=.28&&P.Y>=1.4&&P.Y<=2.05&&Fits(World);}
    bool Floor(int X,int Y) const{return X>=0&&Y>=0&&X<Size&&Y<Size&&Tiles.IsValidIndex(Y*Size+X)&&Tiles[Y*Size+X]!=0;}
    bool Walkable(int X,int Y) const{return Floor(X,Y)&&!Obstructions.Contains(Y*Size+X)&&!StairBlocks(Center({X,Y}),.22f)&&!PropBlocks(Center({X,Y}));}
    void Carve(int X,int Y){if(X>0&&Y>0&&X<Size-1&&Y<Size-1)Tiles[Y*Size+X]=1;}
    void Corridor(FIntPoint A,FIntPoint B){
        // Five-cell passages leave room for later combat and spell effects.
        for(int X=FMath::Min(A.X,B.X);X<=FMath::Max(A.X,B.X);++X)for(int K=-2;K<=2;++K)Carve(X,A.Y+K);
        for(int Y=FMath::Min(A.Y,B.Y);Y<=FMath::Max(A.Y,B.Y);++Y)for(int K=-2;K<=2;++K)Carve(B.X+K,Y);
    }
    FIntPoint Split(FRoom R,int Depth,FRandomStream& Rand){
        if(Depth<4&&(R.W>=28||R.H>=28)){
            const bool Vertical=R.W>=28&&(R.H<28||R.W>R.H||(R.W==R.H&&Rand.RandRange(0,1)));
            FRoom A=R,B=R;
            if(Vertical){const int Cut=Rand.RandRange(13,R.W-13);A.W=Cut;B.X+=Cut;B.W-=Cut;}
            else{const int Cut=Rand.RandRange(13,R.H-13);A.H=Cut;B.Y+=Cut;B.H-=Cut;}
            const auto PA=Split(A,Depth+1,Rand),PB=Split(B,Depth+1,Rand);Corridor(PA,PB);return PA;
        }
        const int W=Rand.RandRange(FMath::Max(10,R.W-6),R.W-3),H=Rand.RandRange(FMath::Max(10,R.H-6),R.H-3);
        const FRoom Room{R.X+Rand.RandRange(1,R.W-W-1),R.Y+Rand.RandRange(1,R.H-H-1),W,H};
        Rooms.Add(Room);for(int Y=Room.Y;Y<Room.Y+H;++Y)for(int X=Room.X;X<Room.X+W;++X)Carve(X,Y);
        return Room.Center();
    }
    TArray<int> Distances(FIntPoint From) const{
        TArray<int> D;D.Init(-1,Size*Size);if(!Walkable(From.X,From.Y))return D;
        TArray<FIntPoint> Q;Q.Add(From);D[From.Y*Size+From.X]=0;
        const FIntPoint Steps[]={{1,0},{-1,0},{0,1},{0,-1}};
        for(int I=0;I<Q.Num();++I)for(auto S:Steps){auto P=Q[I]+S;const int N=P.Y*Size+P.X;if(Walkable(P.X,P.Y)&&D[N]<0){D[N]=D[Q[I].Y*Size+Q[I].X]+1;Q.Add(P);}}
        return D;
    }
    void Generate(int Seed){
        StairsActive=false;Tiles.Init(0,Size*Size);Rooms.Empty();Obstructions.Empty();PropBases.Empty();FRandomStream Rand(Seed);Start=Split({2,2,60,60},0,Rand);
        const auto D=Distances(Start);Exit=Start;int Far=0;for(const auto& R:Rooms){const auto C=R.Center();if(D[C.Y*Size+C.X]>Far){Far=D[C.Y*Size+C.X];Exit=C;}}
    }
    bool Fits(FVector2D P,float Radius=.22f) const{
        if(StairBlocks(P,Radius)||PropBlocks(P,Radius))return false;
        const auto G=Grid(P);
        for(int Y=FMath::FloorToInt(G.Y-Radius);Y<=FMath::FloorToInt(G.Y+Radius);++Y)
            for(int X=FMath::FloorToInt(G.X-Radius);X<=FMath::FloorToInt(G.X+Radius);++X)if(!Floor(X,Y)||Obstructions.Contains(Y*Size+X))return false;
        return true;
    }
    FVector2D Move(FVector2D P,FVector2D Delta) const{
        const int Steps=FMath::Max(1,FMath::CeilToInt(Delta.Size()/.12));const auto Step=Grid(Delta)/Steps;
        auto G=Grid(P);for(int I=0;I<Steps;++I){auto Next=G+FVector2D(Step.X,0);if(Fits(Rotate45(Next)))G=Next;Next=G+FVector2D(0,Step.Y);if(Fits(Rotate45(Next)))G=Next;}
        return Rotate45(G);
    }
    bool Sight(FVector2D A,FVector2D B) const{
        const int Steps=FMath::Max(1,FMath::CeilToInt((B-A).Size()/.15));
        for(int I=0;I<=Steps;++I){const auto C=Cell(FMath::Lerp(A,B,double(I)/Steps));if(!Floor(C.X,C.Y))return false;}return true;
    }
};
}
