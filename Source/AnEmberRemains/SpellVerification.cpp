#include "IsoPrototype.h"
#include "Kismet/GameplayStatics.h"
#include "MaraCastMetrics.h"
#include "MaraChannelMetrics.h"
using namespace IsoDungeon;
void AIsoPrototypeGameMode::VerifySpells(int& Checks,int& Errors,FString& Failures){
    auto Check=[&](bool OK,const TCHAR* Name){++Checks;if(!OK){++Errors;Failures+=FString(TEXT("SPELL_VERIFY: "))+Name+TEXT("\n");}};
    auto* H=Cast<AIsoPrototypePawn>(UGameplayStatics::GetPlayerPawn(this,0));if(!H){Check(false,TEXT("pawn"));return;}
    auto Reset=[&](){Grid.Tiles.Init(1,FGrid::Size*FGrid::Size);Grid.Obstructions.Empty();Grid.PropBases.Empty();Grid.StairsActive=false;IcePillars.Empty();Enemies.Empty();Bolts.Empty();Numbers.Empty();Paused=MapOpen=Won=false;LoreIndex=-1;DescentTime=-1;H->Position=Center({20,20});H->Aim={1,0};H->SpellTarget=H->Position+H->Aim*3;H->Health=100;H->EvadeAge=-1;H->EvadeCooldown=0;H->AttackAge=-1;H->Channeling=false;H->Hurt=0;for(auto& C:H->SpellCooldown)C=0;};
    IceGuardTime=0;Reset();for(int I=1;I<=6;++I){H->AttackAge=-1;Check(H->BeginSpell(I),TEXT("each spell starts"));Check(!H->BeginSpell(I),TEXT("no double cast"));H->AttackAge=-1;Check(!H->BeginSpell(I),TEXT("cooldown blocks recast"));}
    Reset();H->Health=149;ReleaseSpell(H,6);Check(H->Health==150,TEXT("healing capped"));H->Health=0;ReleaseSpell(H,6);Check(H->Health==0,TEXT("healing cannot resurrect"));
    Reset();ReleaseSpell(H,3);Check(IcePillars.Num()==18,TEXT("closed ice ring"));const auto Start=H->Position;const auto Stop=MoveWithIce(Start,{7,0});Check((Stop-Start).Size()<1.5,TEXT("ice blocks swept movement"));Check(!IceBlocks(Stop),TEXT("no ice overlap"));
    for(int D=0;D<64;++D){const float A=D*2*PI/64;Check(!SpellSight(Start,Start+FVector2D(FMath::Cos(A),FMath::Sin(A))*5),TEXT("ring blocks all approach angles"));}
    Check(H->IsIceProtected()&&!H->ReceiveHit(90)&&H->Health==100,TEXT("ice prevents incoming damage inside"));H->Position=Start+FVector2D(3,0);Check(!H->IsIceProtected()&&H->ReceiveHit(10),TEXT("no protection outside"));H->Position=Start;H->Hurt=0;
    TickSpells(6.1f);Check(IcePillars.IsEmpty()&&!H->IsIceProtected()&&MoveWithIce(Start,{7,0}).Equals(Start+FVector2D(7,0),.01),TEXT("ice expires and releases path"));
    ReleaseSpell(H,3);H->SpellCooldown[2]=7;Check(H->BeginSpell(3)&&IcePillars.IsEmpty()&&IceGuardTime==0&&H->SpellCooldown[2]==7,TEXT("manual shatter retains cooldown"));
    for(int Spell:{2,4})for(float Step:{1.f/30,1.f/60,1.f/144}){
        Reset();FIsoEnemy E;E.P=H->Position+FVector2D(2,0);E.HP=500;Enemies.Add(E);E.P=H->Position+FVector2D(2,2);Enemies.Add(E);H->ActiveSpell=Spell;H->Channeling=true;
        for(float T=0;T<1.001f-Step;T+=Step)ChannelSpell(H,Step);
        Check(Enemies[0].HP<470&&Enemies[0].HP>450,TEXT("channel damage bounded across frame rates"));Check(Enemies[1].HP==500,TEXT("channel misses outside width"));
    }
    Reset();FIsoEnemy E;E.P=H->Position+FVector2D(2,0);Enemies.Add(E);E.P=H->Position+FVector2D(5,0);Enemies.Add(E);ReleaseSpell(H,5);Check(Enemies[0].HP==25&&Enemies[1].HP==70,TEXT("nova radius"));Check(Enemies[0].Slow==2.5f&&Enemies[1].Slow==0,TEXT("nova applies independent slow"));DamageEnemy(Enemies[0],1);Check(Enemies[0].Slow==2.5f,TEXT("other hits do not erase slow"));
    Reset();E.P=H->Position+FVector2D(3,0);E.HP=70;Enemies.Add(E);const auto Wall=Cell(H->Position+FVector2D(1.5,0));Grid.Tiles[Wall.Y*FGrid::Size+Wall.X]=0;H->ActiveSpell=4;H->Channeling=true;ChannelSpell(H,.2f);ReleaseSpell(H,5);Check(Enemies[0].HP==70,TEXT("wall occludes beam and nova"));
    Reset();H->ActiveSpell=2;H->Channeling=true;Check(H->BeginEvade(true,{1,0})&&!H->Channeling,TEXT("phase cancels channel"));
    Reset();for(bool Pause:{false,true}){Paused=Pause;MapOpen=!Pause;Check(!H->BeginSpell(1),TEXT("blocked while menus open"));}Reset();H->Health=0;Check(!H->BeginSpell(1),TEXT("dead cannot cast"));
    Reset();for(int D=0;D<8;++D){H->Facing=D;for(int F=0;F<6;++F){H->AttackAge=F*.1f+.001f;H->Channeling=false;const auto P=H->StaffTip();const auto& T=MaraCast::Tips[D][F];Check(P.Equals({T[0],T[1]},.001),TEXT("cast crystal anchor matches frame"));Check(P.Y<0&&FMath::Abs(P.X)<150,TEXT("cast anchor within character bounds"));}for(int F=0;F<3;++F){H->Channeling=true;H->ChannelAge=(F+.01f)/7;const auto& T=MaraChannel::Tips[D][F];Check(H->StaffTip().Equals({T[0],T[1]},.001),TEXT("channel crystal anchor matches frame"));}}
}
