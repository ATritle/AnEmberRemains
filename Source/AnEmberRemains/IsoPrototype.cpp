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
#include "MaraArtMetrics.h"
#include "SpellFX.h"

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
bool AIsoPrototypePawn::ReceiveHit(float Damage){
    if(Damage<=0||Health<=0||Hurt>0||IsPhasing()||IsIceProtected())return false;
    Health=FMath::Max(0.f,Health-Damage);Hurt=.45f;return true;
}
bool AIsoPrototypePawn::BeginEvade(bool Roll,FVector2D V){
    auto* G=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();
    if(!G||Health<=0||EvadeAge>=0||EvadeCooldown>0||G->Paused||G->MapOpen||G->LoreIndex>=0||G->DescentTime>=0)return false;
    if(V.IsNearlyZero()){const double A=Facing*PI/4;V=Unproject({FMath::Sin(A),-FMath::Cos(A)});}
    StopChannel();EvadeDirection=V.GetSafeNormal();Facing=Direction(EvadeDirection);Rolling=Roll;EvadeAge=0;EvadeCooldown=1.05f;AttackAge=-1;PhaseOrigin=Position;PhaseAfter=0;return true;
}
void AIsoPrototypePawn::TickEvade(float Dt){
    if(EvadeAge<0)return;auto* G=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();if(!G)return;
    const float Duration=Rolling?PhaseDuration:.55f;const float Previous=EvadeAge;EvadeAge=FMath::Min(Duration,EvadeAge+Dt);
    // Actual displacement uses the same swept collision as walking, never teleporting through props.
    const float Travel=(Rolling?PhaseSpeed:5.8f)*Duration/PI*(FMath::Cos(Previous/Duration*PI)-FMath::Cos(EvadeAge/Duration*PI));
    Position=G->MoveWithIce(Position,EvadeDirection*Travel);Moving=false;
    CameraCenter=FMath::Lerp(CameraCenter,Position,double(1-FMath::Exp(-8.f*Dt)));
    if(EvadeAge>=Duration){if(Rolling){PhaseEnd=Position;PhaseAfter=.28f;}EvadeAge=-1;}
}
void AIsoPrototypePawn::Tick(float Dt){
    Super::Tick(Dt);auto* G=GetWorld()->GetAuthGameMode<AIsoPrototypeGameMode>();auto* PC=Cast<APlayerController>(GetController());if(!G||!PC||G->Grid.Tiles.IsEmpty())return;
    if(G->DescentTime>=0)return;
    if(Health<=0){EvadeAge=-1;PhaseAfter=0;Channeling=false;AttackAge=-1;}
    if(G->LoreIndex>=0){if(PC->WasInputKeyJustPressed(EKeys::Escape)||PC->WasInputKeyJustPressed(EKeys::E))G->LoreIndex=-1;return;}
    if(EvadeAge<0&&!G->Paused&&!G->MapOpen&&PC->WasInputKeyJustPressed(EKeys::E)&&G->BeginDescent(this))return;
    if(EvadeAge<0&&!G->Paused&&!G->MapOpen&&!G->Won&&PC->WasInputKeyJustPressed(EKeys::E))for(int I=0;I<G->Details.Num();++I){const auto& D=G->Details[I];if(D.Art>=4&&(D.P-Position).Size()<1.7&&G->Grid.Sight(Position,D.P)){G->LoreIndex=I;return;}}
    if(PC->WasInputKeyJustPressed(EKeys::Escape))G->Paused=!G->Paused;
    if(PC->WasInputKeyJustPressed(EKeys::M))G->MapOpen=!G->MapOpen;
    if(PC->WasInputKeyJustPressed(EKeys::R)){G->FloorNumber=1;G->Generate(G->Seed+1);return;}
    if(G->Paused||G->MapOpen||G->Won)return;
    Dt=FMath::Min(Dt,.05f);Hurt=FMath::Max(0.f,Hurt-Dt);Moving=false;
    EvadeCooldown=FMath::Max(0.f,EvadeCooldown-Dt);
    PhaseAfter=FMath::Max(0.f,PhaseAfter-Dt);
    for(float& C:SpellCooldown)C=FMath::Max(0.f,C-Dt);
    const auto EvadeInput=Input(float(PC->IsInputKeyDown(EKeys::D))-float(PC->IsInputKeyDown(EKeys::A)),float(PC->IsInputKeyDown(EKeys::S))-float(PC->IsInputKeyDown(EKeys::W)));
    if(PC->WasInputKeyJustPressed(EKeys::SpaceBar))BeginEvade(true,EvadeInput);
    else if(PC->WasInputKeyJustPressed(EKeys::LeftAlt))BeginEvade(false,EvadeInput);
    if(EvadeAge>=0){TickEvade(Dt);return;}
    if(Health>0){
        const auto V=Input(float(PC->IsInputKeyDown(EKeys::D))-float(PC->IsInputKeyDown(EKeys::A)),float(PC->IsInputKeyDown(EKeys::S))-float(PC->IsInputKeyDown(EKeys::W)));
        Sprinting=PC->IsInputKeyDown(EKeys::LeftShift);const auto Prev=Position;
        Position=G->MoveWithIce(Position,V*Dt*(Sprinting?4.7:3.1)*(Channeling?0.:AttackAge>=0?.42:1.));
        const float Travel=(Position-Prev).Size();WalkDistance+=Travel;Moving=Travel>.0001;
        if(Moving&&AttackAge<0)Facing=Direction(V);
        const bool Review=FParse::Param(FCommandLine::Get(),TEXT("SpellReview"));
        if(!Review)UpdateAim();
        const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six};
        for(int I=0;I<6;++I)if(PC->WasInputKeyJustPressed(Keys[I]))BeginSpell(I+1);
        if(PC->IsInputKeyDown(EKeys::LeftMouseButton))BeginSpell(1);
        if(Channeling){
            Facing=Direction(Aim);ChannelAge+=Dt;
            if((!Review&&!PC->IsInputKeyDown(Keys[ActiveSpell-1]))||ChannelAge>=(ActiveSpell==2?2.5f:2.f))StopChannel();
            else G->ChannelSpell(this,Dt);
        }else if(AttackAge>=0){
            AttackAge+=Dt;
            if(AttackAge>=.3f&&!AttackHit){AttackHit=true;G->ReleaseSpell(this,ActiveSpell);}
            if(AttackAge>=.6f)AttackAge=-1;
        }
    }
    // Exponential, frame-rate-independent camera lag. Both canvas and world camera
    // consume this center, so pointer aiming and following cannot drift apart.
    CameraCenter=FMath::Lerp(CameraCenter,Position,double(1-FMath::Exp(-8.f*Dt)));
    SetActorLocation(FVector(Position.X*100,Position.Y*100,0));
    Camera->SetWorldLocation(FVector(CameraCenter.X*100,CameraCenter.Y*100,1500));
}
AIsoPrototypeGameMode::AIsoPrototypeGameMode(){PrimaryActorTick.bCanEverTick=true;DefaultPawnClass=AIsoPrototypePawn::StaticClass();HUDClass=AIsoPrototypeHUD::StaticClass();}
void AIsoPrototypeGameMode::BeginPlay(){Super::BeginPlay();if(FParse::Param(FCommandLine::Get(),TEXT("BuildSpellNiagara"))){const bool OK=ASpellFX::BuildAssets();UE_LOG(LogTemp,Display,TEXT("SPELL_BUILD: %s"),OK?TEXT("SUCCESS"):TEXT("FAILED"));FPlatformMisc::RequestExit(false);return;}if(FParse::Param(FCommandLine::Get(),TEXT("SpellCapture"))){FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./30.);}FParse::Value(FCommandLine::Get(),TEXT("IsoSeed="),Seed);Generate(Seed);if(FParse::Param(FCommandLine::Get(),TEXT("IsoVerify")))Verify();}
void AIsoPrototypeGameMode::Generate(int NewSeed){
    Seed=NewSeed;Grid.Generate(Seed);DressFloor();Grid.StairsActive=true;BuildLife();BuildLighting();DescentTime=-1;ArrivalFade=0;Seen.Init(0,FGrid::Size*FGrid::Size);Reveal.Init(0,FGrid::Size*FGrid::Size);Enemies.Empty();Numbers.Empty();Bolts.Empty();Kills=0;Won=MapOpen=Paused=false;FlowClock=0;
    if(auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0)))H->PhaseAfter=0;
    IcePillars.Empty();IceGuardTime=SpellHintTime=0;if(auto* FX=ASpellFX::Find(GetWorld(),false))FX->Clear();
    if(auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0))){H->Channeling=false;H->ActiveSpell=0;for(float& C:H->SpellCooldown)C=0;}
    if(auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0))){H->Position=H->CameraCenter=Center(Grid.Start);H->Health=150;H->AttackAge=-1;H->EvadeAge=-1;H->EvadeCooldown=0;H->Hurt=0;H->WalkDistance=0;}
    // Environment-first build. Existing enemy logic/art retained for later testing.
    if(FParse::Param(FCommandLine::Get(),TEXT("ReviewRearWall")))if(auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0))){const auto& R=Grid.Rooms[0];for(int X=R.X+2;X<R.X+R.W-2;++X){FIntPoint C(X,R.Y);if(Grid.Fits(Center(C))&&!Grid.Floor(X,R.Y-1)){H->Position=H->CameraCenter=Center(C);break;}}}
    int ReviewRoom=-1;if(FParse::Value(FCommandLine::Get(),TEXT("ReviewRoom="),ReviewRoom)&&Grid.Rooms.IsValidIndex(ReviewRoom))
        if(auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0)))H->Position=H->CameraCenter=Center(Grid.Rooms[ReviewRoom].Center());
    if(FParse::Param(FCommandLine::Get(),TEXT("ReviewExit")))if(auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0)))H->Position=H->CameraCenter=Grid.StairEntry();
    if(FParse::Param(FCommandLine::Get(),TEXT("ReviewStairSide")))if(auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0)))H->Position=H->CameraCenter=Grid.Move(Center(Grid.Exit)+Rotate45({2,0}),Rotate45({-2,0}));
    if(FParse::Param(FCommandLine::Get(),TEXT("ReviewCoffin")))if(auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0)))for(const auto& P:Props)if(P.Art==1){const auto Base=Center(P.Cell)+Rotate45({-1.025,-.965});H->Position=H->CameraCenter=Grid.Move(Base+Rotate45({3,0}),Rotate45({-3,0}));H->Facing=Direction(Base-H->Position);break;}
    if(FParse::Param(FCommandLine::Get(),TEXT("ReviewClue")))for(int I=0;I<Details.Num();++I)if(Details[I].Art==6){LoreIndex=I;break;}
    if(FParse::Param(FCommandLine::Get(),TEXT("ReviewNearWall")))if(auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0))){
        const auto& R=Grid.Rooms[0];bool Found=false;
        for(int Y=R.Y+R.H-1;Y>=R.Y&&!Found;--Y)for(int X=R.X+2;X<R.X+R.W-2;++X){const FIntPoint C(X,Y);if(Grid.Fits(Center(C))&&!Grid.Floor(C.X,C.Y+1)){H->Position=H->CameraCenter=Center(C);Found=true;break;}}
    }
}
void AIsoPrototypeGameMode::CastEmber(AIsoPrototypePawn* H){
    if(H&&H->Health>0&&!H->Aim.IsNearlyZero()&&Bolts.Num()<32){const auto Offset=Unproject(H->StaffTip()+FVector2D(0,55));Bolts.Add({H->Position,H->Aim.GetSafeNormal()*9,1.4f,0,Offset,0});if(auto* FX=ASpellFX::Find(GetWorld()))FX->Emit(H->Position+Offset,55,H->Aim,6,44,.2f);}
}
void AIsoPrototypeGameMode::TickBolts(float Dt){
    for(auto& Bolt:Bolts){
        const int Steps=FMath::Max(1,FMath::CeilToInt(Bolt.Velocity.Size()*Dt/.1));
        for(int I=0;I<Steps&&Bolt.Life>0;++I){
            const auto Next=Bolt.P+Bolt.Velocity*(Dt/Steps);if(!SpellSight(Bolt.P,Next)){Bolt.Life=0;break;}Bolt.P=Next;
            for(auto& E:Enemies)if(E.HP>0&&(E.P-Bolt.P).Size()<.5){E.HP-=28;E.Hurt=.2f;E.Alert=true;Numbers.Add({E.P,0,28});Bolt.Life=0;if(E.HP<=0){E.Death=0;++Kills;}break;}
        }
        Bolt.Life-=Dt;Bolt.FXClock+=Dt;Bolt.Age+=Dt;
        const auto Visual=Bolt.P+Bolt.TipOffset*FMath::Max(0.f,1-Bolt.Age/.3f);
        if(auto* FX=ASpellFX::Find(GetWorld())){if(Bolt.Life<=0)FX->Emit(Visual,55,Bolt.Velocity,6,100,.45f);else if(Bolt.FXClock>=.025f){Bolt.FXClock=0;FX->Emit(Visual,55,Bolt.Velocity,0,54,.12f);}}
    }
    Bolts.RemoveAll([](const FEmberBolt& B){return B.Life<=0;});
}
FString AIsoPrototypeGameMode::RoomName(FIntPoint Cell) const{
    for(int I=0;I<Grid.Rooms.Num();++I){const auto& R=Grid.Rooms[I];if(Cell.X<R.X||Cell.Y<R.Y||Cell.X>=R.X+R.W||Cell.Y>=R.Y+R.H)continue;
        if(I==0)return TEXT("Hidden Cathedral Stair");if(R.Center()==Grid.Exit)return TEXT("The Lower Descent");
        const TCHAR* Names[]={TEXT("Burial Chapel"),TEXT("Confiscated Relics"),TEXT("Holding Cells"),TEXT("Failed Healers' Infirmary")};return Names[(I-1)%4];
    }
    return TEXT("Processional Passage");
}
void AIsoPrototypeGameMode::Tick(float Dt){
    Super::Tick(Dt);auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));if(!H||Grid.Tiles.IsEmpty())return;
    if(FParse::Param(FCommandLine::Get(),TEXT("IsoReview"))){ReviewClock+=Dt;float At=3;FParse::Value(FCommandLine::Get(),TEXT("ReviewAt="),At);if(ReviewClock>At&&ReviewClock<At+.1f)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("IsoPrototype.png"),false,false);if(ReviewClock>At+1)FPlatformMisc::RequestExit(false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("SpellCapture"))){static int Frame=0;if(Frame%2==0)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SpellPreviewFrames/Frame%04d.png"),Frame/2),false,false);++Frame;if(SpellReviewClock>21.f)FPlatformMisc::RequestExit(false);}
    float PhaseReview=-1;if(FParse::Value(FCommandLine::Get(),TEXT("ReviewPhase="),PhaseReview)){H->Rolling=true;H->EvadeAge=FMath::Clamp(PhaseReview,0.f,H->PhaseDuration-.01f);H->PhaseOrigin=Center(Grid.Start);H->EvadeDirection={1,0};H->Facing=2;H->Position=H->PhaseOrigin+H->EvadeDirection*(H->PhaseSpeed*H->PhaseDuration/PI*(1-FMath::Cos(H->EvadeAge/H->PhaseDuration*PI)));H->CameraCenter=H->Position;}
    if(Paused||MapOpen||Won||LoreIndex>=0||H->Health<=0)return;Dt=FMath::Min(Dt,.05f);
    if(FParse::Param(FCommandLine::Get(),TEXT("ReviewDescent"))&&FloorNumber==1&&ReviewClock>.6f&&DescentTime<0){H->Position=H->CameraCenter=Grid.StairEntry();BeginDescent(H);}
    ArrivalFade=FMath::Max(0.f,ArrivalFade-Dt);
    if(DescentTime>=0){DescentTime+=Dt;const float T=FMath::Clamp(DescentTime/1.2f,0.f,1.f);const auto Target=Center(Grid.Exit);H->Position=FMath::Lerp(DescentStart,Target,double(T));H->Facing=Direction(Target-DescentStart);H->Moving=true;H->WalkDistance+=Dt*1.5f;if(DescentTime>=1.6f)NextFloor();return;}
    TickLife(Dt,H->Position);
    TickSpells(Dt);
    for(int I=0;I<Seen.Num();++I)if(Seen[I])Reveal[I]=FMath::Min(1.f,Reveal[I]+Dt*2.5f);
    TickBolts(Dt);for(auto& N:Numbers)N.Age+=Dt;Numbers.RemoveAll([](const FIsoNumber& N){return N.Age>1;});
    FlowClock-=Dt;if(FlowClock<=0){Flow=Grid.Distances(Cell(H->Position));FlowClock=.25f;
        const auto C=Cell(H->Position);for(int Y=C.Y-11;Y<=C.Y+11;++Y)for(int X=C.X-11;X<=C.X+11;++X)if(Grid.Floor(X,Y)&&(FVector2D(X-C.X,Y-C.Y)).Size()<=11&&Grid.Sight(H->Position,Center({X,Y})))Seen[Y*FGrid::Size+X]=1;
    }
    const FIntPoint Steps[]={{1,0},{-1,0},{0,1},{0,-1}};
    for(auto& E:Enemies){
        if(E.HP<=0){E.Death+=Dt;continue;}E.Hurt=FMath::Max(0.f,E.Hurt-Dt);E.Slow=FMath::Max(0.f,E.Slow-Dt);E.Cooldown=FMath::Max(0.f,E.Cooldown-Dt);
        const auto Delta=H->Position-E.P;const float Distance=Delta.Size();if(Distance<7&&Grid.Sight(E.P,H->Position))E.Alert=true;if(!E.Alert||Distance>22)continue;
        if(E.Attack>=0){E.Attack+=Dt;if(E.Attack>=.55f&&!E.Hit){E.Hit=true;if(Distance<1.2&&Grid.Sight(E.P,H->Position)&&H->ReceiveHit(12)){Numbers.Add({H->Position,0,-12});}}if(E.Attack>.9f){E.Attack=-1;E.Cooldown=.5f;}continue;}
        if(Distance<1.05&&E.Cooldown<=0&&Grid.Sight(E.P,H->Position)){E.Attack=0;E.Hit=false;E.Aim=Delta.GetSafeNormal();continue;}
        if(Distance<.85)continue;
        auto Target=H->Position;if(!Grid.Sight(E.P,H->Position)){const auto C=Cell(E.P);int Best=MAX_int32;Target=E.P;for(auto S:Steps){const auto N=C+S;if(!Grid.Floor(N.X,N.Y))continue;const int D=Flow[N.Y*FGrid::Size+N.X];if(D>=0&&D<Best){Best=D;Target=Center(N);}}}
        auto Velocity=(Target-E.P).GetSafeNormal();for(const auto& Other:Enemies)if(&Other!=&E&&Other.HP>0){const auto Apart=E.P-Other.P;const float L=Apart.Size();if(L>.01&&L<.65)Velocity+=Apart/L*(.65-L)*2;}
        const auto Prev=E.P;E.P=MoveWithIce(E.P,Velocity.GetClampedToMaxSize(1)*Dt*(E.Hurt>0?.4:1.65)*(E.Slow>0?.45:1.));E.Walk+=(E.P-Prev).Size();E.Aim=Delta.GetSafeNormal();
    }
}
void AIsoPrototypeGameMode::Verify(){
    int Errors=0,Checks=0;FString Failures;auto CheckImpl=[&](bool OK,int Line){++Checks;if(!OK){++Errors;Failures+=FString::Printf(TEXT("Failure line=%d seed=%d\n"),Line,Seed);}};
#define Check(OK) CheckImpl((OK),__LINE__)
    for(int D=0;D<8;++D)for(int I=0;I<36;++I){const auto& R=MaraArt::Frames[D][I];Check(R.W>20&&R.H>20);Check(R.X>=0&&R.Y>=0&&R.X+R.W<=MaraArt::SheetWidth&&R.Y+R.H<=MaraArt::SheetHeight);Check(R.PX>=0&&R.PX<R.W&&R.PY>=0&&R.PY<R.H);}
    for(int S=0;S<128;++S){FGrid A,B;A.Generate(S);B.Generate(S);Check(A.Tiles==B.Tiles);const auto D=A.Distances(A.Start);for(int I=0;I<A.Tiles.Num();++I)if(A.Tiles[I])Check(D[I]>=0);Check(A.Fits(Center(A.Start)));Check(D[A.Exit.Y*FGrid::Size+A.Exit.X]>10);Check(A.Rooms.Num()>=4);
        for(auto V:{Input(1,0),Input(0,-1),Input(1,-1)}){Check(FMath::IsNearlyEqual(V.Size(),1.,.0001));Check(A.Fits(A.Move(Center(A.Start),V*100)));}
    }
    Check(Input(0,-1).X>0&&Input(0,-1).Y<0);Check(Input(-1,0).X<0&&Input(-1,0).Y<0);
    Check(Unproject(Project({3.2,-7.1})).Equals({3.2,-7.1},.00001));
    for(int S=0;S<128;++S){Generate(S);const auto D=Grid.Distances(Grid.Start);Check(Enemies.IsEmpty());Check(!Props.IsEmpty());for(int I=0;I<Grid.Tiles.Num();++I)if(Grid.Walkable(I%FGrid::Size,I/FGrid::Size))Check(D[I]>=0);const auto Entry=Cell(Grid.StairEntry());Check(D[Entry.Y*FGrid::Size+Entry.X]>0);for(const auto& R:Grid.Rooms)if(R.Center()!=Grid.Exit)Check(Grid.Fits(Center(R.Center())));for(const auto& P:Props)if(P.Art!=1)Check(!Grid.Fits(Center(P.Cell)));else{
            const auto Base=Center(P.Cell)+Rotate45({-1.025,-.965});Check(!Grid.Fits(Base));Check(Grid.Fits(Center(P.Cell)));
            for(auto Offset:{FVector2D(3,0),FVector2D(-3,0),FVector2D(0,2),FVector2D(0,-2)}){const auto From=Base+Rotate45(Offset);if(!Grid.Fits(From))continue;const auto Stop=Grid.Move(From,-Rotate45(Offset)*2);Check(Grid.Fits(Stop));Check(!Stop.Equals(Base,.1));Check((Stop-Base).Size()<1.65);}
        }
        Check(Grid.AtStairEntry(Grid.StairEntry()));Check(!Grid.Fits(Center(Grid.Exit)));
        const auto Approach=Grid.StairEntry()+Rotate45({0,1});Check(Grid.Move(Approach,Grid.StairEntry()-Approach).Equals(Grid.StairEntry(),.001));
        for(auto Offset:{FVector2D(-2,0),FVector2D(2,0),FVector2D(0,-3)}){const auto From=Center(Grid.Exit)+Rotate45(Offset);const auto Stop=Grid.Move(From,-Rotate45(Offset));Check(Grid.Fits(Stop));Check(!Grid.AtStairEntry(Stop));Check(!Stop.Equals(Center(Grid.Exit),.1));}
        const auto Stop=Grid.Move(Grid.StairEntry(),Rotate45({0,-8}));Check(Grid.Fits(Stop));Check(Grid.AtStairEntry(Stop));Check((IsoDungeon::Grid(Stop)-FVector2D(Grid.Exit.X+.5,Grid.Exit.Y+.5)).Y>=1.42-.001);
    }
    // Follow the complete entrance-to-sanctum route through the same swept
    // collision function used by player input, including all corridor turns.
    Generate(4816);const auto Route=Grid.Distances(Grid.Start);FIntPoint StepCell=Cell(Grid.StairEntry());
    while(StepCell!=Grid.Start){const int D=Route[StepCell.Y*FGrid::Size+StepCell.X];bool Advanced=false;
        for(const auto Offset:{FIntPoint(1,0),FIntPoint(-1,0),FIntPoint(0,1),FIntPoint(0,-1)}){const auto Next=StepCell+Offset;if(!Grid.Walkable(Next.X,Next.Y)||Route[Next.Y*FGrid::Size+Next.X]!=D-1)continue;
            const auto From=Center(StepCell),To=Center(Next);Check(Grid.Move(From,To-From).Equals(To,.001));StepCell=Next;Advanced=true;break;}
        Check(Advanced);if(!Advanced)break;
    }
    Check(!WallOccludes({20,20},Center({21,21})));
    Check(!WallOccludes({20,20},Center({19,21})));
    Check(WallOccludes({20,20},Center({19,19})));
    Check(!WallOccludes({20,20},Center({12,20})));
    // Ambient dressing is cosmetic; rat movement must stay on walkable ground.
    for(int S=0;S<8;++S){Generate(S);Check(!Details.IsEmpty()&&!Rats.IsEmpty());const auto Tiles=Grid.Tiles;const auto Obstacles=Grid.Obstructions.Num();
        for(const auto& D:Details)Check(Grid.Floor(Cell(D.P).X,Cell(D.P).Y));
        for(int T=0;T<240;++T){TickLife(1.f/30,Center(Grid.Start));for(const auto& R:Rats)Check(Grid.Fits(R.P));}
        Check(Grid.Tiles==Tiles&&Grid.Obstructions.Num()==Obstacles);
    }
    Generate(4816);Rats.Empty();const auto RP=Center(Grid.Start);Rats.Add({RP,{1,0},0,0,0,false});const auto Threat=RP-FVector2D(.5,0);TickLife(.05f,Threat);Check(Rats[0].Fear>0&&Rats[0].Moving);Check((Rats[0].P-Threat).Size()>.5);
    Grid.Obstructions.Empty();Grid.PropBases.Empty();Grid.StairsActive=false;
    Grid.Tiles.Init(1,FGrid::Size*FGrid::Size);Enemies.Empty();auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));
    if(H){H->Position=Center({20,20});H->Aim={1,0};FIsoEnemy E;E.P=H->Position+FVector2D(3,0);Enemies.Add(E);E.P=H->Position-FVector2D(3,0);Enemies.Add(E);CastEmber(H);Check(Enemies[0].HP==70);TickBolts(.4f);Check(Enemies[0].HP==42&&Enemies[1].HP==70&&Bolts.IsEmpty());const auto C=Cell(H->Position+FVector2D(1,0));Grid.Tiles[C.Y*FGrid::Size+C.X]=0;CastEmber(H);TickBolts(.4f);Check(Enemies[0].HP==42&&Bolts.IsEmpty());}else Check(false);
    if(H){
        Grid.Tiles.Init(1,FGrid::Size*FGrid::Size);Grid.Obstructions.Empty();Grid.PropBases.Empty();Grid.StairsActive=false;
        for(float Step:{1.f/30,1.f/60,1.f/144}){H->Position=Center({20,20});H->Health=150;H->Hurt=0;H->EvadeAge=-1;H->EvadeCooldown=0;const auto Before=H->Position;Check(H->BeginEvade(true,{1,0}));Check(H->IsPhasing());Check(!H->ReceiveHit(50)&&H->Health==150);float Elapsed=0;for(int I=0;I<300&&H->EvadeAge>=0;++I){H->TickEvade(Step);Elapsed+=Step;}Check(FMath::IsNearlyEqual(float((H->Position-Before).Size()),float(6.2*.7*2/PI*1.15),.002f));Check(Elapsed>=H->PhaseDuration&&Elapsed<H->PhaseDuration+Step+.001f);Check(!H->IsPhasing());Check(H->ReceiveHit(12)&&H->Health==138);}
        H->Hurt=0;H->EvadeAge=-1;H->EvadeCooldown=0;H->Position=Center({20,20});Grid.Obstructions.Add(20*FGrid::Size+21);Check(H->BeginEvade(true,Rotate45({1,0})));for(int I=0;I<50;++I)H->TickEvade(.02f);Check(Grid.Fits(H->Position));Check(Cell(H->Position).X==20);
        FloorNumber=1;Generate(4816);Check(!BeginDescent(H));
        for(bool Roll:{false,true}){H->Position=Center(Grid.Start);H->EvadeAge=-1;H->EvadeCooldown=0;const auto Before=H->Position;Check(H->BeginEvade(Roll,Rotate45({1,0})));Check(!H->BeginEvade(Roll,{1,0}));for(int T=0;T<30;++T)H->TickEvade(1.f/30);Check(H->EvadeAge<0);Check(Grid.Fits(H->Position));Check((H->Position-Before).Size()>.1);}
        for(int I=0;I<3;++I){const int PreviousSeed=Seed,PreviousFloor=FloorNumber;H->Health=93;H->Position=Center(Grid.Exit)+Rotate45({2,0});Check(!BeginDescent(H));H->Position=Grid.StairEntry();Check(BeginDescent(H));Check(!BeginDescent(H));
            for(int T=0;T<55;++T)Tick(1.f/30);
            Check(FloorNumber==PreviousFloor+1&&Seed!=PreviousSeed);Check(H->Health==93);Check(Grid.Fits(H->Position));const auto Entry=Cell(Grid.StairEntry());Check(Grid.Distances(Grid.Start)[Entry.Y*FGrid::Size+Entry.X]>0);Check(!Won&&DescentTime<0);}
    }
#undef Check
    VerifySpells(Checks,Errors,Failures);
    const FString Result=FString::Printf(TEXT("ISO_VERIFY checks=%d errors=%d\n"),Checks,Errors)+Failures;UE_LOG(LogTemp,Display,TEXT("%s"),*Result);FFileHelper::SaveStringToFile(Result,*(FPaths::ProjectSavedDir()/TEXT("IsoVerify.txt")));FPlatformMisc::RequestExitWithStatus(false,Errors?1:0);
}
