// Copyright ProjectR. All Rights Reserved.

#include "PREncounterDialogueData.h"

// ===== 대화 그래프 조회 =====

bool UPRFaerinEncounterDialogueData::FindDialogueNode(FName NodeId, FPRFaerinDialogueNode& OutNode) const
{
	const FPRFaerinDialogueNode* FoundNode = FindDialogueNodePtr(NodeId);
	if (FoundNode == nullptr)
	{
		return false;
	}

	OutNode = *FoundNode;
	return true;
}

bool UPRFaerinEncounterDialogueData::HasDialogueGraph() const
{
	return !RootDialogueNodeId.IsNone() && DialogueNodes.Num() > 0;
}

const FPRFaerinDialogueNode* UPRFaerinEncounterDialogueData::FindDialogueNodePtr(FName NodeId) const
{
	if (NodeId.IsNone())
	{
		return nullptr;
	}

	for (const FPRFaerinDialogueNode& Node : DialogueNodes)
	{
		if (Node.NodeId == NodeId)
		{
			return &Node;
		}
	}

	return nullptr;
}
