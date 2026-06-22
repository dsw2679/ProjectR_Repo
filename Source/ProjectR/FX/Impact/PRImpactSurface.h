// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 임팩트 Surface 구현)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PRImpactTypes.h"
#include "PRImpactSurface.generated.h"

UINTERFACE(BlueprintType)
class PROJECTR_API UPRImpactSurface : public UInterface
{
	GENERATED_BODY()
};

// 피격 대상이나 피격 컴포넌트가 자체 상태에 맞는 Impact 태그만 Manager에 제공하기 위한 인터페이스
class PROJECTR_API IPRImpactSurface
{
	GENERATED_BODY()

public:
	// 전달된 충돌 위치와 표면 법선을 기준으로 Registry 조회에 사용할 Impact 태그 반환
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ProjectR|Impact")
	FPRImpactResult ResolveImpactSurface(const FPRImpactContext& Context) const;
	virtual FPRImpactResult ResolveImpactSurface_Implementation(const FPRImpactContext& Context) const;
};
