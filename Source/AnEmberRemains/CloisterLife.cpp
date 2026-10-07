#include "IsoPrototype.h"
#include "Engine/Canvas.h"
using namespace IsoDungeon;

void AIsoPrototypeGameMode::BuildLife(){
    Details.Empty();Rats.Empty();AmbientTime=0;LoreIndex=-1;AmbientRandom.Initialize(Seed^0x37a916);
    auto Add=[&](FIntPoint C,int Art,float Size){
        if(!Grid.Walkable(C.X,C.Y)||(Center(C)-Center(Grid.Exit)).Size()<3)return;
        for(int Y=-1;Y<=1;++Y)for(int X=-1;X<=1;++X)if(!Grid.Floor(C.X+X,C.Y+Y))return;
        Details.Add({Center(C),Art,Size,AmbientRandom.FRand()*6.28f});
    };
    for(int I=0;I<Grid.Rooms.Num();++I){const auto& R=Grid.Rooms[I];const int Purpose=I==0?0:(I-1)%4;
        Add({R.X+2,R.Y+2},1,2.1f);
        Add({R.X+R.W-3,R.Y+2},Purpose==0?2:Purpose==3?3:0,1.9f);
        Add({R.X+2,R.Y+R.H-3},Purpose==2?4:Purpose==3?6:5,1.f);
        Add({R.X+R.W-3,R.Y+R.H-3},Purpose==0?7:Purpose==3?6:4,1.f);
        for(int J=0;J<5;++J){FIntPoint C(AmbientRandom.RandRange(R.X+2,R.X+R.W-3),AmbientRandom.RandRange(R.Y+2,R.Y+R.H-3));Add(C,J==0?1:Purpose==0&&J%2?2:Purpose==3&&J==1?3:0,AmbientRandom.FRandRange(1.1f,2.f));}
        for(int J=0;J<2;++J){FIntPoint C(R.X+2+J,R.Y+R.H-3);if(Grid.Fits(Center(C)))Rats.Add({Center(C),Rotate45({1.,0.}),float(J),0,0,false});}
    }
    for(int Y=2;Y<FGrid::Size-2;++Y)for(int X=2;X<FGrid::Size-2;++X)if(RoomIndex({X,Y})<0&&Grid.Floor(X,Y)&&(X*17+Y*31+Seed)%29==0)Add({X,Y},0,1.2f);
}

void AIsoPrototypeGameMode::TickLife(float Dt,FVector2D Player){
    AmbientTime+=Dt;
    for(auto& R:Rats){
        const auto Away=R.P-Player;const bool Near=Away.Size()<3.8&&Grid.Sight(Player,R.P);
        R.Fear=Near?1.3f:FMath::Max(0.f,R.Fear-Dt);R.Decision-=Dt;
        if(R.Decision<=0){
            R.Decision=R.Fear>0?.18f:AmbientRandom.FRandRange(.6f,2.f);R.Moving=R.Fear>0||AmbientRandom.FRand()<.58f;
            float Best=-1.e9;FVector2D Pick=R.Direction;
            for(int I=0;I<8;++I){const double A=I*PI/4;const FVector2D V(FMath::Cos(A),FMath::Sin(A));const auto End=Grid.Move(R.P,V*.8);const float Travel=(End-R.P).Size();
                if(Travel<.55f)continue;const float Score=R.Fear>0?float((End-Player).Size())+float(FVector2D::DotProduct(V,R.Direction))*.1f:AmbientRandom.FRand();if(Score>Best){Best=Score;Pick=V;}}
            if(Best< -1.e8)R.Moving=false;else R.Direction=Pick;
        }
        if(R.Moving){const auto Before=R.P;R.P=Grid.Move(R.P,R.Direction*Dt*(R.Fear>0?3.9:1.05));const float Distance=(R.P-Before).Size();R.Travel+=Distance;if(Distance<.0001){R.Moving=false;R.Decision=0;}}
    }
}

void AIsoPrototypeHUD::FloorDetail(const FCloisterDetail& D,float Alpha,float Time){
    const FVector2D UV((D.Art%4)*.25,(D.Art/4)*.5);
    if(D.Art<4){TArray<FVector2D> Q;for(auto V:{FVector2D(-.5,-.5),FVector2D(.5,-.5),FVector2D(.5,.5),FVector2D(-.5,.5)})Q.Add(Screen(D.P+Rotate45(V*D.Size)));
        Quad(Texture(TEXT("CloisterLife")),Q,UV,{.25,.5},FLinearColor(1,1,1,Alpha*(D.Art==1?.57f:.63f)));
        if(D.Art==1){
            const float Phase=FMath::Frac(Time*.4f+D.Phase);const auto P=Screen(D.P);const float Radius=(2+Phase*20)*Zoom;
            if(Phase<.12f)Polygon({P+FVector2D(-.8,-(1-Phase/.12f)*32)*Zoom,P+FVector2D(.8,-(1-Phase/.12f)*32)*Zoom,P+FVector2D(.3,4-(1-Phase/.12f)*32)*Zoom},FLinearColor(.55f,.66f,.7f,Alpha*.5f));
            for(int I=0;I<24;++I){const float A=I*2*PI/24,B=(I+1)*2*PI/24;const FVector2D U(FMath::Cos(A),FMath::Sin(A)*.5f),V(FMath::Cos(B),FMath::Sin(B)*.5f);Polygon({P+U*Radius,P+V*Radius,P+V*(Radius+.65f*Zoom),P+U*(Radius+.65f*Zoom)},FLinearColor(.48f,.59f,.65f,Alpha*(1-Phase)*.38f));}
        }
    }else{const auto P=Screen(D.P);const float S=83*D.Size*Zoom;DrawTexture(Texture(TEXT("CloisterLife")),P.X-S*.5f,P.Y-S*.68f,S,S,UV.X,UV.Y,.25,.5,FLinearColor(1,1,1,Alpha*.9f),BLEND_Translucent);
        if(D.Art==7)Glow(P-FVector2D(0,S*.15),15,FLinearColor(1,.23f,.025f,Alpha*(.6f+.25f*FMath::Sin(Time*2+D.Phase))));
    }
}

void AIsoPrototypeHUD::Rat(const FCloisterRat& R,float Alpha){
    const auto P=Screen(R.P);const auto D=IsoDungeon::Project(R.Direction);const bool Left=D.X<0;const int Row=D.Y< -FMath::Abs(D.X)*.35?1:0,Frame=R.Moving?int(R.Travel*7)%4:0;
    const float S=61*Zoom;Glow(P,12,FLinearColor(0,0,0,Alpha));
    const float Anchor=Left?.34f:.66f;DrawTexture(Texture(TEXT("CloisterRats")),P.X-S*Anchor,P.Y-S*.74f,S,S,(Frame+(Left?1:0))*.25f,Row*.5f,Left?-.25f:.25f,.5f,FLinearColor(.82f,.82f,.78f,Alpha),BLEND_Translucent);
}
