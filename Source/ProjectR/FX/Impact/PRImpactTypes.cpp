// Copyright ProjectR. All Rights Reserved.
// Author: 배유찬 (피격 임팩트 타입 및 구조체 정의)
#include "PRImpactTypes.h"

#include "Components/DecalComponent.h"
#include "NiagaraComponent.h"

DEFINE_LOG_CATEGORY(LogPRImpact);

bool FPRImpactNiagaraComponentLease::IsValid() const
{
	// Lease가 반환 또는 재생 대상으로 사용할 수 있는 Niagara Component를 보유했는지 확인함
	return ::IsValid(Component);
}

bool FPRImpactDecalComponentLease::IsValid() const
{
	// Lease가 반환 또는 표시 대상으로 사용할 수 있는 Decal Component를 보유했는지 확인함
	return ::IsValid(Component);
}
