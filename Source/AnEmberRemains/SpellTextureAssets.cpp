#include "SpellFX.h"
#if WITH_EDITOR
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Engine/Texture2D.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#endif
bool ASpellFX::BuildTexturedAssets(){
#if WITH_EDITOR
    auto Save=[](UObject* O){auto* P=O->GetOutermost();P->MarkPackageDirty();FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;return UPackage::SavePackage(P,O,*FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension()),A);};
    for(bool Ice:{false,true}){
        const FString Name=Ice?TEXT("M_IceChunks"):TEXT("M_FlameFlipbook");auto* P=CreatePackage(*(TEXT("/Game/Effects/Spells/")+Name));P->FullyLoad();
        auto* M=NewObject<UMaterial>(P,*Name,RF_Public|RF_Standalone);M->BlendMode=BLEND_Translucent;M->SetShadingModel(MSM_Unlit);M->TwoSided=true;M->SetUsageByFlag(MATUSAGE_NiagaraSprites,true);
        auto* UV=NewObject<UMaterialExpressionTextureCoordinate>(M);M->GetExpressionCollection().AddExpression(UV);
        auto* Tex=NewObject<UMaterialExpressionTextureObject>(M);Tex->Texture=LoadObject<UTexture2D>(nullptr,Ice?TEXT("/Game/Effects/Spells/IceAtlas.IceAtlas"):TEXT("/Game/Effects/Spells/FlameAtlas.FlameAtlas"));if(!Tex->Texture)return false;M->GetExpressionCollection().AddExpression(Tex);
        auto* C=NewObject<UMaterialExpressionCustom>(M);C->OutputType=CMOT_Float4;C->Inputs.Reset();
        auto Input=[&](const TCHAR* N,UMaterialExpression* E){FCustomInput I;I.InputName=N;I.Input.Expression=E;C->Inputs.Add(I);};Input(TEXT("UV"),UV);Input(TEXT("Atlas"),Tex);
        for(const TCHAR* N:{TEXT("Age"),TEXT("Angle"),TEXT("Strength"),TEXT("Variant"),TEXT("Style")}){auto* E=NewObject<UMaterialExpressionScalarParameter>(M);E->ParameterName=N;E->DefaultValue=FString(N)==TEXT("Strength")?1:0;M->GetExpressionCollection().AddExpression(E);Input(N,E);}
        C->Code=Ice?TEXT(R"(
if(Age<0||Age>=1)return 0;
float grow=.15+.85*smoothstep(0,.15,Age);float2 uv=(UV-float2(.5,.9375))/grow+float2(.5,.9375);
if(any(uv<0)||any(uv>1))return 0;
float f=fmod(Variant,4);float2 tile=float2(fmod(f,2),floor(f/2));
float4 a=Texture2DSample(Atlas,AtlasSampler,(tile+uv)*.5);
float fade=saturate((1-Age)*8);return float4(a.rgb*.70,a.a*fade*saturate(Strength));
)"):TEXT(R"(
if(Age<0||Age>=1)return 0;
float2 p=UV-.5;float c=cos(Angle),s=sin(Angle);float2 uv=float2(c*p.x+s*p.y,-s*p.x+c*p.y)+.5;
// Original hot edge is on the left; turn it toward the travel direction.
uv.x=1-uv.x;if(any(uv<.001)||any(uv>.999))return 0;
float f=saturate(Age)*15,lo=floor(f),hi=min(15,lo+1);
float2 a=float2(fmod(lo,4),floor(lo/4)),b=float2(fmod(hi,4),floor(hi/4));
float4 color=lerp(Texture2DSample(Atlas,AtlasSampler,(a+uv)*.25),Texture2DSample(Atlas,AtlasSampler,(b+uv)*.25),frac(f));
float fire=saturate((color.r-color.b)*4);float3 rgb=color.rgb*(.45+fire*1.1);
if(Style>6.5){float l=dot(color.rgb,float3(.3,.59,.11));rgb=float3(.38,.66,.86)*(.35+l);}
return float4(rgb,color.a*saturate(Age*20)*saturate((1-Age)*5)*saturate(Strength));
)");
        M->GetExpressionCollection().AddExpression(C);
        auto* RGB=NewObject<UMaterialExpressionComponentMask>(M);RGB->Input.Expression=C;RGB->R=RGB->G=RGB->B=true;RGB->A=false;M->GetExpressionCollection().AddExpression(RGB);
        auto* Alpha=NewObject<UMaterialExpressionComponentMask>(M);Alpha->Input.Expression=C;Alpha->R=Alpha->G=Alpha->B=false;Alpha->A=true;M->GetExpressionCollection().AddExpression(Alpha);
        M->GetEditorOnlyData()->EmissiveColor.Expression=RGB;M->GetEditorOnlyData()->Opacity.Expression=Alpha;M->PostEditChange();if(!Save(M))return false;
    }
    // Scene capture RGB is premultiplied, A is inverse opacity. Preserve dark smoke
    // and solid ice rather than incorrectly adding their RGB to the dungeon.
    auto* P=CreatePackage(TEXT("/Game/Effects/Spells/M_SpellComposite"));P->FullyLoad();auto* M=NewObject<UMaterial>(P,TEXT("M_SpellComposite"),RF_Public|RF_Standalone);
    M->BlendMode=BLEND_AlphaComposite;M->SetShadingModel(MSM_Unlit);M->TwoSided=true;
    auto* Tex=NewObject<UMaterialExpressionTextureSampleParameter2D>(M);Tex->ParameterName=TEXT("SpellRT");Tex->SamplerType=SAMPLERTYPE_Color;
    Tex->Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Effects/Spells/FlameAtlas.FlameAtlas"));M->GetExpressionCollection().AddExpression(Tex);
    auto* Inv=NewObject<UMaterialExpressionOneMinus>(M);Inv->Input.Expression=Tex;Inv->Input.OutputIndex=4;M->GetExpressionCollection().AddExpression(Inv);
    M->GetEditorOnlyData()->EmissiveColor.Expression=Tex;M->GetEditorOnlyData()->Opacity.Expression=Inv;M->PostEditChange();return Save(M);
#else
    return false;
#endif
}
