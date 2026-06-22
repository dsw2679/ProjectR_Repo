// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (무기 강화 연출 및 재화/재료 소모 연동 구현)
// Author: 배유찬 (강화 UI 스택 제어 및 장착 무기 정보 연동 구현)
#include "PRWeaponUpgradeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Player/Components/PRCurrencyComponent.h"
#include "ProjectR/UI/Components/PRUIControllerComponent.h"
#include "ProjectR/UI/Inventory/PRInventoryItemListWidget.h"
#include "ProjectR/UI/Inventory/PRItemSlotWidget.h"
#include "ProjectR/UI/WeaponUpgrade/PRWeaponUpgradeMaterialCostWidget.h"
#include "ProjectR/ItemSystem/Components/PRWeaponUpgradeComponent.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

namespace
{
	FText GetWeaponUpgradeFailReasonText(EPRWeaponUpgradeFailReason FailReason)
	{
		switch (FailReason)
		{
		case EPRWeaponUpgradeFailReason::InvalidAuthority:
			return FText::FromString(TEXT("서버 권한이 없습니다."));
		case EPRWeaponUpgradeFailReason::InvalidUpgradeStation:
			return FText::FromString(TEXT("강화 대상 NPC가 유효하지 않습니다."));
		case EPRWeaponUpgradeFailReason::InvalidInteractor:
			return FText::FromString(TEXT("강화 NPC와 너무 멀리 떨어져 있습니다."));
		case EPRWeaponUpgradeFailReason::RequestThrottled:
			return FText::FromString(TEXT("잠시 후 다시 시도하세요."));
		case EPRWeaponUpgradeFailReason::InvalidWeapon:
			return FText::FromString(TEXT("강화할 수 없는 무기입니다."));
		case EPRWeaponUpgradeFailReason::MaxLevel:
			return FText::FromString(TEXT("이미 최대 강화 단계입니다."));
		case EPRWeaponUpgradeFailReason::MissingUpgradeData:
			return FText::FromString(TEXT("다음 강화 데이터가 없습니다."));
		case EPRWeaponUpgradeFailReason::MissingItemData:
			return FText::FromString(TEXT("강화 재료 데이터가 없습니다."));
		case EPRWeaponUpgradeFailReason::NotEnoughScrap:
			return FText::FromString(TEXT("고철이 부족합니다."));
		case EPRWeaponUpgradeFailReason::NotEnoughMaterial:
			return FText::FromString(TEXT("강화 재료가 부족합니다."));
		case EPRWeaponUpgradeFailReason::ConsumeFailed:
			return FText::FromString(TEXT("비용 소모에 실패했습니다."));
		case EPRWeaponUpgradeFailReason::None:
		default:
			return FText::FromString(TEXT("강화에 실패했습니다."));
		}
	}
}

UPRWeaponUpgradeWidget::UPRWeaponUpgradeWidget()
{
	Layer = EPRUILayer::Menu;
	InputMode = EPBUIInputMode::UIOnly;
	bShowMouseCursor = true;
}

void UPRWeaponUpgradeWidget::SetUpgradeContext(UPRWeaponUpgradeComponent* InUpgradeComponent)
{
	UnbindSourceEvents();

	UpgradeComponent = InUpgradeComponent;
	SelectedWeaponItem = nullptr;
	CurrentPreview = FPRWeaponUpgradePreview();

	ResolvePlayerSources();
	BindSourceEvents();
	RefreshWeaponUpgradeView();
}

void UPRWeaponUpgradeWidget::RequestUpgradeWeapon(UPRItemInstance_Weapon* WeaponItem)
{
	UPRItemInstance_Weapon* TargetWeaponItem = IsValid(WeaponItem) ? WeaponItem : SelectedWeaponItem.Get();
	if (!IsValid(UpgradeComponent) || !IsValid(TargetWeaponItem))
	{
		return;
	}

	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->RequestUpgradeWeapon(UpgradeComponent, TargetWeaponItem);
}

void UPRWeaponUpgradeWidget::SelectWeapon(UPRItemInstance_Weapon* WeaponItem)
{
	SelectedWeaponItem = WeaponItem;

	if (IsValid(UpgradeResultText))
	{
		UpgradeResultText->SetText(FText::GetEmpty());
	}

	RefreshSelectedWeaponSlot();
	RefreshUpgradePreview();
	RefreshUpgradeButton();
}

void UPRWeaponUpgradeWidget::RefreshWeaponUpgradeView()
{
	RefreshWeaponList();
	RefreshSelectedWeaponSlot();
	RefreshUpgradePreview();
	RefreshUpgradeButton();
}

void UPRWeaponUpgradeWidget::CloseWeaponUpgrade()
{
	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}

	UPRUIControllerComponent* UIController = PlayerController->GetUIController();
	if (!IsValid(UIController))
	{
		return;
	}

	UIController->CloseWeaponUpgrade();
}

/*~ UUserWidget Interface ~*/

void UPRWeaponUpgradeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindChildWidgetEvents();
	ResolvePlayerSources();
	BindSourceEvents();
	RefreshWeaponUpgradeView();
}

void UPRWeaponUpgradeWidget::NativeDestruct()
{
	UnbindChildWidgetEvents();
	UnbindSourceEvents();
	ClearMaterialCostWidgets();

	Super::NativeDestruct();
}

void UPRWeaponUpgradeWidget::BindChildWidgetEvents()
{
	UnbindChildWidgetEvents();

	if (IsValid(WeaponListWidget))
	{
		WeaponListWidget->OnItemSelected.AddDynamic(this, &UPRWeaponUpgradeWidget::HandleWeaponListSelection);
	}

	if (IsValid(UpgradeButton))
	{
		UpgradeButton->OnClicked.AddDynamic(this, &UPRWeaponUpgradeWidget::HandleUpgradeButtonClicked);
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.AddDynamic(this, &UPRWeaponUpgradeWidget::HandleCloseButtonClicked);
	}
}

void UPRWeaponUpgradeWidget::UnbindChildWidgetEvents()
{
	if (IsValid(WeaponListWidget))
	{
		WeaponListWidget->OnItemSelected.RemoveDynamic(this, &UPRWeaponUpgradeWidget::HandleWeaponListSelection);
	}

	if (IsValid(UpgradeButton))
	{
		UpgradeButton->OnClicked.RemoveDynamic(this, &UPRWeaponUpgradeWidget::HandleUpgradeButtonClicked);
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UPRWeaponUpgradeWidget::HandleCloseButtonClicked);
	}
}

void UPRWeaponUpgradeWidget::BindSourceEvents()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRWeaponUpgradeWidget::HandleInventoryChanged);
		InventoryComponent->GetOnInventoryChanged().AddDynamic(this, &UPRWeaponUpgradeWidget::HandleInventoryChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRWeaponUpgradeWidget::HandleScrapChanged);
		CurrencyComponent->OnScrapChanged.AddDynamic(this, &UPRWeaponUpgradeWidget::HandleScrapChanged);
	}

	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (IsValid(PlayerController))
	{
		PlayerController->OnWeaponUpgradeResult.RemoveDynamic(this, &UPRWeaponUpgradeWidget::HandleWeaponUpgradeResult);
		PlayerController->OnWeaponUpgradeResult.AddDynamic(this, &UPRWeaponUpgradeWidget::HandleWeaponUpgradeResult);
	}
}

void UPRWeaponUpgradeWidget::UnbindSourceEvents()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->GetOnInventoryChanged().RemoveDynamic(this, &UPRWeaponUpgradeWidget::HandleInventoryChanged);
	}

	if (IsValid(CurrencyComponent))
	{
		CurrencyComponent->OnScrapChanged.RemoveDynamic(this, &UPRWeaponUpgradeWidget::HandleScrapChanged);
	}

	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (IsValid(PlayerController))
	{
		PlayerController->OnWeaponUpgradeResult.RemoveDynamic(this, &UPRWeaponUpgradeWidget::HandleWeaponUpgradeResult);
	}

	InventoryComponent = nullptr;
	CurrencyComponent = nullptr;
}

void UPRWeaponUpgradeWidget::ResolvePlayerSources()
{
	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	APRPlayerState* PlayerState = IsValid(PlayerController)
		? PlayerController->GetPlayerState<APRPlayerState>()
		: nullptr;

	InventoryComponent = IsValid(PlayerState) ? PlayerState->GetInventoryComponent() : nullptr;
	CurrencyComponent = IsValid(PlayerState) ? PlayerState->GetCurrencyComponent() : nullptr;
}

void UPRWeaponUpgradeWidget::RefreshWeaponList()
{
	if (!IsValid(WeaponListWidget))
	{
		return;
	}

	TArray<FPRInventoryItemSlotViewData> ListItems;
	if (IsValid(InventoryComponent))
	{
		for (UPRItemInstance_Weapon* WeaponItem : InventoryComponent->GetItemsByType<UPRItemInstance_Weapon>(EPRItemType::Weapon))
		{
			if (!IsValid(WeaponItem))
			{
				continue;
			}

			const bool bSelected = WeaponItem == SelectedWeaponItem.Get();
			ListItems.Add(BuildWeaponItemViewData(WeaponItem, bSelected));
		}
	}

	WeaponListWidget->SetItemList(EPRItemType::Weapon, ListItems);
	WeaponListWidget->SetVisibility(ESlateVisibility::Visible);
}

void UPRWeaponUpgradeWidget::RefreshSelectedWeaponSlot()
{
	if (!IsValid(SelectedWeaponSlotWidget))
	{
		return;
	}

	if (IsValid(SelectedWeaponItem))
	{
		SelectedWeaponSlotWidget->SetSlotViewData(BuildWeaponItemViewData(SelectedWeaponItem, true));
		return;
	}

	FPRInventoryItemSlotViewData EmptyViewData;
	EmptyViewData.ItemType = EPRItemType::Weapon;
	EmptyViewData.DisplayName = FText::FromString(TEXT("무기 선택"));
	SelectedWeaponSlotWidget->SetSlotViewData(EmptyViewData);
}

void UPRWeaponUpgradeWidget::RefreshUpgradePreview()
{
	APRPlayerController* PlayerController = GetOwningPlayer<APRPlayerController>();
	if (IsValid(UpgradeComponent) && IsValid(PlayerController) && IsValid(SelectedWeaponItem))
	{
		CurrentPreview = UpgradeComponent->BuildUpgradePreview(PlayerController, SelectedWeaponItem);
	}
	else
	{
		CurrentPreview = FPRWeaponUpgradePreview();
	}

	if (IsValid(UpgradeLevelText))
	{
		if (!CurrentPreview.bValidWeapon)
		{
			UpgradeLevelText->SetText(FText::FromString(TEXT("")));
		}
		else if (CurrentPreview.bMaxLevel)
		{
			UpgradeLevelText->SetText(FText::FromString(FString::Printf(TEXT("+%d 최대 강화"), CurrentPreview.CurrentLevel)));
		}
		else if (CurrentPreview.bHasUpgradeData)
		{
			UpgradeLevelText->SetText(FText::FromString(FString::Printf(TEXT("+%d -> +%d"), CurrentPreview.CurrentLevel, CurrentPreview.TargetLevel)));
		}
		else
		{
			UpgradeLevelText->SetText(FText::FromString(FString::Printf(TEXT("+%d 다음 강화 데이터 없음"), CurrentPreview.CurrentLevel)));
		}
	}

	if (IsValid(UpgradeDamageText))
	{
		if (CurrentPreview.bValidWeapon)
		{
			UpgradeDamageText->SetText(FText::FromString(FString::Printf(
				TEXT("%.1f -> %.1f"),
				CurrentPreview.CurrentBaseDamage,
				CurrentPreview.TargetBaseDamage)));
		}
		else
		{
			UpgradeDamageText->SetText(FText::GetEmpty());
		}
	}

	RefreshMaterialCostWidgets();
	OnUpgradePreviewChanged.Broadcast(CurrentPreview);
}

void UPRWeaponUpgradeWidget::RefreshUpgradeButton()
{
	if (IsValid(UpgradeButton))
	{
		UpgradeButton->SetIsEnabled(CurrentPreview.bCanUpgrade);
	}
}

void UPRWeaponUpgradeWidget::RefreshMaterialCostWidgets()
{
	ClearMaterialCostWidgets();

	if (!IsValid(MaterialCostPanel) || !IsValid(MaterialCostWidgetClass.Get()))
	{
		return;
	}

	const bool bHasScrapCost = CurrentPreview.ScrapCost > 0;
	const bool bHasMaterialCost = !CurrentPreview.MaterialCosts.IsEmpty();
	if (!CurrentPreview.bValidWeapon || CurrentPreview.bMaxLevel || !CurrentPreview.bHasUpgradeData || (!bHasScrapCost && !bHasMaterialCost))
	{
		MaterialCostPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	MaterialCostPanel->SetVisibility(ESlateVisibility::Visible);

	if (bHasScrapCost)
	{
		FPRWeaponUpgradeMaterialCostViewData ScrapCostViewData;
		ScrapCostViewData.DisplayName = FText::FromString(TEXT("고철"));
		ScrapCostViewData.Icon = ScrapIconTexture;
		ScrapCostViewData.RequiredQuantity = CurrentPreview.ScrapCost;
		ScrapCostViewData.OwnedQuantity = CurrentPreview.OwnedScrap;
		ScrapCostViewData.bEnough = CurrentPreview.bEnoughScrap;
		AddCostWidget(ScrapCostViewData);
	}

	for (const FPRWeaponUpgradeMaterialCostViewData& MaterialCost : CurrentPreview.MaterialCosts)
	{
		AddCostWidget(MaterialCost);
	}
}

void UPRWeaponUpgradeWidget::ClearMaterialCostWidgets()
{
	MaterialCostWidgets.Reset();

	if (IsValid(MaterialCostPanel))
	{
		MaterialCostPanel->ClearChildren();
	}
}

void UPRWeaponUpgradeWidget::AddCostWidget(const FPRWeaponUpgradeMaterialCostViewData& CostViewData)
{
	if (!IsValid(MaterialCostPanel) || !IsValid(MaterialCostWidgetClass.Get()))
	{
		return;
	}

	UPRWeaponUpgradeMaterialCostWidget* CostWidget = nullptr;
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (IsValid(OwningPlayer))
	{
		CostWidget = CreateWidget<UPRWeaponUpgradeMaterialCostWidget>(OwningPlayer, MaterialCostWidgetClass);
	}
	else if (IsValid(WidgetTree))
	{
		CostWidget = WidgetTree->ConstructWidget<UPRWeaponUpgradeMaterialCostWidget>(MaterialCostWidgetClass);
	}

	if (!IsValid(CostWidget))
	{
		return;
	}

	CostWidget->SetMaterialCostViewData(CostViewData);
	MaterialCostPanel->AddChild(CostWidget);
	MaterialCostWidgets.Add(CostWidget);
}

FPRInventoryItemSlotViewData UPRWeaponUpgradeWidget::BuildWeaponItemViewData(UPRItemInstance_Weapon* WeaponItem, bool bSelected) const
{
	FPRInventoryItemSlotViewData ViewData;
	if (!IsValid(WeaponItem))
	{
		return ViewData;
	}

	UPRWeaponDataAsset* WeaponData = WeaponItem->GetWeaponData();
	ViewData.ItemData = WeaponData;
	ViewData.ItemInstance = WeaponItem;
	ViewData.ItemType = EPRItemType::Weapon;
	ViewData.bSelected = bSelected;
	ViewData.bShowStackCount = false;

	if (IsValid(WeaponData))
	{
		ViewData.DisplayName = WeaponData->GetDisplayName();
		ViewData.Icon = WeaponData->GetIcon();
	}

	return ViewData;
}

void UPRWeaponUpgradeWidget::HandleWeaponListSelection(const FPRInventoryItemSlotViewData& ViewData)
{
	UPRItemInstance_Weapon* WeaponItem = Cast<UPRItemInstance_Weapon>(ViewData.ItemInstance.Get());
	if (!IsValid(WeaponItem))
	{
		return;
	}

	SelectWeapon(WeaponItem);
	RefreshWeaponList();
}

void UPRWeaponUpgradeWidget::HandleUpgradeButtonClicked()
{
	RequestUpgradeWeapon(SelectedWeaponItem);
}

void UPRWeaponUpgradeWidget::HandleCloseButtonClicked()
{
	CloseWeaponUpgrade();
}

void UPRWeaponUpgradeWidget::HandleInventoryChanged(UPRInventoryComponent* ChangedInventoryComponent, const FPRInventoryChangeEventData& EventData)
{
	if (ChangedInventoryComponent != InventoryComponent)
	{
		return;
	}

	if (IsValid(SelectedWeaponItem) && !InventoryComponent->OwnsItem(SelectedWeaponItem))
	{
		SelectedWeaponItem = nullptr;
	}

	static_cast<void>(EventData);
	RefreshWeaponUpgradeView();
}

void UPRWeaponUpgradeWidget::HandleScrapChanged(int32 NewScrap)
{
	static_cast<void>(NewScrap);

	RefreshUpgradePreview();
	RefreshUpgradeButton();
}

void UPRWeaponUpgradeWidget::HandleWeaponUpgradeResult(const FPRWeaponUpgradeResult& Result)
{
	if (Result.UpgradeComponent != UpgradeComponent)
	{
		return;
	}

	if (IsValid(UpgradeResultText))
	{
		const FText ResultText = Result.bSuccess
			? FText::FromString(TEXT("강화 완료"))
			: GetWeaponUpgradeFailReasonText(Result.FailReason);
		UpgradeResultText->SetText(ResultText);
	}

	if (IsValid(Result.WeaponItem))
	{
		SelectedWeaponItem = Result.WeaponItem;
	}

	RefreshWeaponUpgradeView();
}
