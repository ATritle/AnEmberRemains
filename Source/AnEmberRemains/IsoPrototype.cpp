#include "IsoPrototype.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

using namespace IsoDungeon;
AIsoPrototypePawn::AIsoPrototypePawn(){
    PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("IsometricFollow"));Camera->SetupAttachment(RootComponent);
    Camera->SetAbsolute(true,true,true);Camera->ProjectionMode=ECameraProjectionMode::Orthographic;Camera->OrthoWidth=2000;
    Camera->SetWorldRotation(FRotator(-90,0,0));
}
void AIsoPrototypePawn::BeginPlay(){
    Super::BeginPlay();if(auto* PC=Cast<APlayerController>(GetController())){PC->bShowMouseCursor=true;PC->SetInputMode(FInputModeGameOnly());}
}
void AIsoPrototypePawn::Tick(float Dt){
    Super::Tick(Dt);auto* G=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();auto* PC=Cast<APlayerController>(GetController());if(!G||!PC||G->Grid.Tiles.IsEmpty())return;
    if(PC->WasInputKeyJustPressed(EKeys::Escape))G->Paused=!G->Paused;
    if(PC->WasInputKeyJustPressed(EKeys::M))G->MapOpen=!G->MapOpen;
    if(PC->WasInputKeyJustPressed(EKeys::R)){G->Generate(G->Seed+1);return;}
    if(G->Paused||G->MapOpen||G->Won)return;
    Dt=FMath::Min(Dt,.05f);Hurt=FMath::Max(0.f,Hurt-Dt);Moving=false;
    if(Health>0){
        const auto V=Input(float(PC->IsInputKeyDown(EKeys::D))-float(PC->IsInputKeyDown(EKeys::A)),float(PC->IsInputKeyDown(EKeys::S))-float(PC->IsInputKeyDown(EKeys::W)));
        Sprinting=PC->IsInputKeyDown(EKeys::LeftShift);const auto Prev=Position;
        Position=G->Grid.Move(Position,V*Dt*(Sprinting?4.7:3.1)*(AttackAge>=0?.42:1.));
        const float Travel=(Position-Prev).Size();WalkDistance+=Travel;Moving=Travel>.0001;
        if(Moving&&AttackAge<0)Facing=Direction(V);
        if(PC->IsInputKeyDown(EKeys::LeftMouseButton)&&AttackAge<0){
            float MX,MY;int SX,SY;PC->GetViewportSize(SX,SY);const float Scale=FMath::Min(SX/1280.f,SY/800.f);
            if(Scale>0&&PC->GetMousePosition(MX,MY)){
                const auto ScreenPoint=(FVector2D(MX,MY)-FVector2D((SX-1280*Scale)/2,(SY-800*Scale)/2))/Scale;
                const auto Target=CameraCenter+Unproject(ScreenPoint-FVector2D(640,440));
                const auto NewAim=(Target-Position).GetSafeNormal();if(!NewAim.IsNearlyZero())Aim=NewAim;
            }
            Facing=Direction(Aim);AttackAge=0;AttackHit=false;Combo=(Combo+1)%3;
        }
        if(AttackAge>=0){AttackAge+=Dt;if(AttackAge>=.22f&&!AttackHit){AttackHit=true;G->CastEmber(this);}if(AttackAge>=.48f)AttackAge=-1;}
        if((Position-Center(G->Grid.Exit)).Size()<1&&PC->WasInputKeyJustPressed(EKeys::E))G->Won=true;
    }
    // Exponential, frame-rate-independent camera lag. Both canvas and world camera
    // consume this center, so pointer aiming and following cannot drift apart.
    CameraCenter=FMath::Lerp(CameraCenter,Position,double(1-FMath::Exp(-8.f*Dt)));
    SetActorLocation(FVector(Position.X*100,Position.Y*100,0));
    Camera->SetWorldLocation(FVector(CameraCenter.X*100,CameraCenter.Y*100,1500));
}
AIsoPrototypeGameMode::AIsoPrototypeGameMode(){PrimaryActorTick.bCanEverTick=true;DefaultPawnClass=AIsoPrototypePawn::StaticClass();HUDClass=AIsoPrototypeHUD::StaticClass();}
void AIsoPrototypeGameMode::BeginPlay(){Super::BeginPlay();FParse::Value(FCommandLine::Get(),TEXT("IsoSeed="),Seed);Generate(Seed);if(FParse::Param(FCommandLine::Get(),TEXT("IsoVerify")))Verify();}
void AIsoPrototypeGameMode::Generate(int NewSeed){
    Seed=NewSeed;Grid.Generate(Seed);Seen.Init(0,FGrid::Size*FGrid::Size);Enemies.Empty();Numbers.Empty();Bolts.Empty();Kills=0;Won=MapOpen=Paused=false;FlowClock=0;
    if(auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0))){H->Position=H->CameraCenter=Center(Grid.Start);H->Health=150;H->AttackAge=-1;H->Hurt=0;H->WalkDistance=0;}
    for(int I=1;I<Grid.Rooms.Num();++I){const auto& R=Grid.Rooms[I];for(int J=0;J<2;++J){FIsoEnemy E;E.P=Center(R.Center())+Rotate45(FVector2D(J?1:-1,0));Enemies.Add(E);}}
}
void AIsoPrototypeGameMode::CastEmber(AIsoPrototypePawn* H){
    if(H&&H->Health>0&&!H->Aim.IsNearlyZero()&&Bolts.Num()<32)Bolts.Add({H->Position,H->Aim.GetSafeNormal()*9,1.4f});
}
void AIsoPrototypeGameMode::TickBolts(float Dt){
    for(auto& Bolt:Bolts){
        const int Steps=FMath::Max(1,FMath::CeilToInt(Bolt.Velocity.Size()*Dt/.1));
        for(int I=0;I<Steps&&Bolt.Life>0;++I){
            const auto Next=Bolt.P+Bolt.Velocity*(Dt/Steps);if(!Grid.Sight(Bolt.P,Next)){Bolt.Life=0;break;}Bolt.P=Next;
            for(auto& E:Enemies)if(E.HP>0&&(E.P-Bolt.P).Size()<.5){E.HP-=28;E.Hurt=.2f;E.Alert=true;Numbers.Add({E.P,0,28});Bolt.Life=0;if(E.HP<=0){E.Death=0;++Kills;}break;}
        }
        Bolt.Life-=Dt;
    }
    Bolts.RemoveAll([](const FEmberBolt& B){return B.Life<=0;});
}
FString AIsoPrototypeGameMode::RoomName(FIntPoint Cell) const{
    for(int I=0;I<Grid.Rooms.Num();++I){const auto& R=Grid.Rooms[I];if(Cell.X<R.X||Cell.Y<R.Y||Cell.X>=R.X+R.W||Cell.Y>=R.Y+R.H)continue;
        if(I==0)return TEXT("Hidden Cathedral Stair");if(R.Center()==Grid.Exit)return TEXT("Mother Caldris's Sanctum");
        const TCHAR* Names[]={TEXT("Burial Chapel"),TEXT("Confiscated Relics"),TEXT("Holding Cells"),TEXT("Failed Healers' Infirmary")};return Names[(I-1)%4];
    }
    return TEXT("Processional Passage");
}
void AIsoPrototypeGameMode::Tick(float Dt){
    Super::Tick(Dt);auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));if(!H||Grid.Tiles.IsEmpty())return;
    if(FParse::Param(FCommandLine::Get(),TEXT("IsoReview"))){ReviewClock+=Dt;if(ReviewClock>3&&ReviewClock<3.2f)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("IsoPrototype.png"),false,false);if(ReviewClock>5)FPlatformMisc::RequestExit(false);}
    if(Paused||MapOpen||Won||H->Health<=0)return;Dt=FMath::Min(Dt,.05f);
    TickBolts(Dt);for(auto& N:Numbers)N.Age+=Dt;Numbers.RemoveAll([](const FIsoNumber& N){return N.Age>1;});
    FlowClock-=Dt;if(FlowClock<=0){Flow=Grid.Distances(Cell(H->Position));FlowClock=.25f;
        const auto C=Cell(H->Position);for(int Y=C.Y-7;Y<=C.Y+7;++Y)for(int X=C.X-7;X<=C.X+7;++X)if(Grid.Floor(X,Y)&&(FVector2D(X-C.X,Y-C.Y)).Size()<=7&&Grid.Sight(H->Position,Center({X,Y})))Seen[Y*FGrid::Size+X]=1;
    }
    const FIntPoint Steps[]={{1,0},{-1,0},{0,1},{0,-1}};
    for(auto& E:Enemies){
        if(E.HP<=0){E.Death+=Dt;continue;}E.Hurt=FMath::Max(0.f,E.Hurt-Dt);E.Cooldown=FMath::Max(0.f,E.Cooldown-Dt);
        const auto Delta=H->Position-E.P;const float Distance=Delta.Size();if(Distance<7&&Grid.Sight(E.P,H->Position))E.Alert=true;if(!E.Alert||Distance>22)continue;
        if(E.Attack>=0){E.Attack+=Dt;if(E.Attack>=.55f&&!E.Hit){E.Hit=true;if(Distance<1.2&&Grid.Sight(E.P,H->Position)&&H->Hurt<=0){H->Health=FMath::Max(0.f,H->Health-12);H->Hurt=.45f;Numbers.Add({H->Position,0,-12});}}if(E.Attack>.9f){E.Attack=-1;E.Cooldown=.5f;}continue;}
        if(Distance<1.05&&E.Cooldown<=0&&Grid.Sight(E.P,H->Position)){E.Attack=0;E.Hit=false;E.Aim=Delta.GetSafeNormal();continue;}
        if(Distance<.85)continue;
        auto Target=H->Position;if(!Grid.Sight(E.P,H->Position)){const auto C=Cell(E.P);int Best=MAX_int32;Target=E.P;for(auto S:Steps){const auto N=C+S;if(!Grid.Floor(N.X,N.Y))continue;const int D=Flow[N.Y*FGrid::Size+N.X];if(D>=0&&D<Best){Best=D;Target=Center(N);}}}
        auto Velocity=(Target-E.P).GetSafeNormal();for(const auto& Other:Enemies)if(&Other!=&E&&Other.HP>0){const auto Apart=E.P-Other.P;const float L=Apart.Size();if(L>.01&&L<.65)Velocity+=Apart/L*(.65-L)*2;}
        const auto Prev=E.P;E.P=Grid.Move(E.P,Velocity.GetClampedToMaxSize(1)*Dt*(E.Hurt>0?.4:1.65));E.Walk+=(E.P-Prev).Size();E.Aim=Delta.GetSafeNormal();
    }
}
void AIsoPrototypeGameMode::Verify(){
    int Errors=0,Checks=0;auto Check=[&](bool OK){++Checks;if(!OK)++Errors;};
    for(int S=0;S<128;++S){FGrid A,B;A.Generate(S);B.Generate(S);Check(A.Tiles==B.Tiles);const auto D=A.Distances(A.Start);for(int I=0;I<A.Tiles.Num();++I)if(A.Tiles[I])Check(D[I]>=0);Check(A.Fits(Center(A.Start)));Check(D[A.Exit.Y*FGrid::Size+A.Exit.X]>10);Check(A.Rooms.Num()>=4);
        for(auto V:{Input(1,0),Input(0,-1),Input(1,-1)}){Check(FMath::IsNearlyEqual(V.Size(),1.,.0001));Check(A.Fits(A.Move(Center(A.Start),V*100)));}
    }
    Check(Input(0,-1).X>0&&Input(0,-1).Y<0);Check(Input(-1,0).X<0&&Input(-1,0).Y<0);
    Check(Unproject(Project({3.2,-7.1})).Equals({3.2,-7.1},.00001));
    Grid.Tiles.Init(1,FGrid::Size*FGrid::Size);Enemies.Empty();auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));
    if(H){H->Position=Center({20,20});H->Aim={1,0};FIsoEnemy E;E.P=H->Position+FVector2D(3,0);Enemies.Add(E);E.P=H->Position-FVector2D(3,0);Enemies.Add(E);CastEmber(H);Check(Enemies[0].HP==70);TickBolts(.4f);Check(Enemies[0].HP==42&&Enemies[1].HP==70&&Bolts.IsEmpty());const auto C=Cell(H->Position+FVector2D(1,0));Grid.Tiles[C.Y*FGrid::Size+C.X]=0;CastEmber(H);TickBolts(.4f);Check(Enemies[0].HP==42&&Bolts.IsEmpty());}else Check(false);
    const FString Result=FString::Printf(TEXT("ISO_VERIFY checks=%d errors=%d\n"),Checks,Errors);UE_LOG(LogTemp,Display,TEXT("%s"),*Result);FFileHelper::SaveStringToFile(Result,*(FPaths::ProjectSavedDir()/TEXT("IsoVerify.txt")));FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
}
