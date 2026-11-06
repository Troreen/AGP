#include "BehaviourTreeFactory.h"

#include <memory>
#include <CommonUtilities/Utility.hpp>

#include "GenericBehaviourTree/BehaviourTree.h"
#include "GenericBehaviourTree/BlackBoard.h"

#include "GenericBehaviourTree/BT_Node.h"
#include "GenericBehaviourTree/BT_CompositeNode.h"
#include "GenericBehaviourTree/BT_LeafNodeAction.h"
#include "GenericBehaviourTree/BT_LeafNodeCondition.h"
#include "GenericBehaviourTree/BT_SelectorNode.h"
#include "GenericBehaviourTree/BT_SequenceNode.h"

#include <Application/Game/BehaviourTree/BehaviourTreeComponent.h>
#include <Application/Game/Controllers/ControllerComponent.h>
#include <GameFramework/World/Actor.h>
#include <GameFramework/Components/AnimatorComponent.h>
#include <GameFramework/Animation/AnimationTree.h>

#include <GameFramework/ServiceLocator.h>

#define BT_Act BT_LeafNodeAction
#define BT_Con BT_LeafNodeCondition
#define BT_Sel BT_SelectorNode
#define BT_Seq BT_SequenceNode

#define BT_NewNode(name, node) std::shared_ptr<node> name = std::make_shared<node>(&outTree)
#define BB_Get(name, type) outTree.GetBlackBoard().Get<type>(name)
#define BB_Set(name, type, value) outTree.GetBlackBoard().Set<type>(name, value)
#define BB_SetGet(name, type, value) outTree.GetBlackBoard().SetGet<type>(name, value)

BehaviourTreeFactory::BehaviourTreeFactory()
{
	myBehaviourTrees["Enemy01"] = [](BehaviourTree& outTree)
		{
			BT_NewNode(idle, BT_Act);
			BT_NewNode(detectPlayer, BT_Act);
			BT_NewNode(moveToPlayer, BT_Act);
			BT_NewNode(attackPlayer, BT_Act);

			BT_NewNode(foundPlayer, BT_Con);
			BT_NewNode(playerInRange, BT_Con);

			BT_NewNode(root, BT_Seq);
			BT_NewNode(base, BT_Seq);
			BT_NewNode(detect, BT_Seq);

			BT_NewNode(look, BT_Sel);
			BT_NewNode(chase, BT_Sel);

			auto* owner = BB_Get("Owner", Actor*);
			auto* target = BB_SetGet("Target", const Actor*, ServiceLocator::GetInstance().GetPlayer());

			auto* running = &BB_SetGet("Running", BT_Act*, nullptr);

			auto& aggroRange = BB_Get("Aggro Range", float);
			auto& attackRange = BB_Get("Attack Range", float);
			auto& moveSpeed = BB_Get("Move Speed", float);

			auto& detected = BB_SetGet("Detected", bool, false);
			auto& found = BB_SetGet("Found Player", bool, false);

			BB_Set("Shoot Position", CommonUtilities::Vector3f, {});

			{
				// Add this node first if your tree does not execute from root every frame.
				// Make return either Success or Failure dependet on if root is a Sequence or Selector node.
				BT_NewNode(exit, BT_Act);
				exit->SetCondition([running]() -> BT_NodeStatus {

					if (*running)
					{
						(*running)->Exit();
						*running = nullptr;
					}

					return BT_NodeStatus::Success;
					});
				root->AddChild(std::move(exit));
			}

			/*idle*/ {

				idle->SetCondition([&outTree]() -> BT_NodeStatus
					{
						return BT_NodeStatus::Running;
					});
				idle->SetStart([&outTree, idle, owner, running]()
					{
						*running = idle.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsIdle", true);
					});

				idle->SetAction([&outTree]()
					{

					});

				idle->SetEnd([&outTree, owner]()
					{
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsIdle", false);
					});
			}

			/*detect*/ {

				detectPlayer->SetCondition([&outTree, &found]() -> BT_NodeStatus
					{
						if (found)
						{
							return BT_NodeStatus::Success;
						}

						return BT_NodeStatus::Running;
					});
				detectPlayer->SetStart([&outTree, detectPlayer, owner, running]()
					{
						*running = detectPlayer.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetTrigger("IsPlayerDetected");
					});

				detectPlayer->SetAction([&outTree, owner, &found]()
					{
						if (!owner->GetComponent<AnimatorComponent>()->GetCurrentPlayBackState(true).Active)
						{
							found = true;
						}
					});
			}

			/*moveToPlayer*/ {
				moveToPlayer->SetCondition([&outTree, owner, target]() -> BT_NodeStatus
					{
						if (target == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						if (owner == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						return BT_NodeStatus::Running;
					});

				moveToPlayer->SetStart([&outTree, moveToPlayer, owner, running]()
					{
						*running = moveToPlayer.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsRunning", true);
					});

				moveToPlayer->SetAction([&outTree, owner, target, &moveSpeed]()
					{
						CommonUtilities::Vector3f direction = target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition();

						direction.y = 0;

						direction.Normalize();
						CommonUtilities::Vector3f newPos = direction * moveSpeed * ServiceLocator::GetInstance().GetDeltaTime() + owner->GetTransform().GetWorldPosition();

						float rotation = atan2f(direction.x, direction.z);
						owner->GetTransform().SetLocalPosition(newPos);
						owner->GetTransform().SetLocalRotation({ 0.0f, rotation, 0.0f });
					});

				moveToPlayer->SetEnd([&outTree, owner]()
					{
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsRunning", false);
					});
			}

			/*attack*/ {
				attackPlayer->SetCondition([&outTree, owner, target]() -> BT_NodeStatus
					{
						if (target == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						if (owner == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						return BT_NodeStatus::Running;
					});
				attackPlayer->SetStart([&outTree, attackPlayer, owner, running]()
					{
						*running = attackPlayer.get();
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsAttacking", true);
					});

				attackPlayer->SetAction([&outTree, target, owner]()
					{
						CommonUtilities::Vector3f direction = target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition();
						direction.y = 0;
						direction.Normalize();

						float rotation = atan2f(direction.x, direction.z);
						owner->GetTransform().SetLocalPosition(owner->GetTransform().GetLocalPosition());
						owner->GetTransform().SetLocalRotation({ 0.0f, rotation, 0.0f });
					});

				attackPlayer->SetEnd([&outTree, owner, running]()
					{
						*running = nullptr;

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsAttacking", false);
					});
			}

			foundPlayer->SetCondition([&outTree, owner, target, &aggroRange, &detected]() -> bool
				{
					if (detected || (target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition()).Length() < aggroRange)
					{
						detected = true;
						return true;
					}
					else
					{
						return false;
					}
				});
			playerInRange->SetCondition([&outTree, owner, target, &attackRange]() -> bool
				{
					if ((target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition()).Length() < attackRange)
					{
						return true;
					}
					else
					{
						return false;
					}
				});

			{
				detect->AddChild(foundPlayer);
				detect->AddChild(detectPlayer);

				look->AddChild(detect);
				look->AddChild(idle);

				chase->AddChild(playerInRange);
				chase->AddChild(moveToPlayer);

				base->AddChild(look);
				base->AddChild(chase);
				base->AddChild(attackPlayer);

				root->AddChild(base);
			}

			outTree.SetRoot(std::move(root));
		};
	myBehaviourTrees["Enemy02"] = [](BehaviourTree& outTree) {

			BT_NewNode(idle, BT_Act);
			BT_NewNode(detectPlayer, BT_Act);
			BT_NewNode(moveToPlayer, BT_Act);
			BT_NewNode(moveToShootPosition, BT_Act);
			BT_NewNode(shootPlayer, BT_Act);

			BT_NewNode(foundPlayer, BT_Con);
			BT_NewNode(playerInLOS, BT_Con);

			BT_NewNode(root, BT_Seq);
			BT_NewNode(base, BT_Seq);
			BT_NewNode(detect, BT_Seq);
			BT_NewNode(attack, BT_Seq);

			BT_NewNode(look, BT_Sel);
			BT_NewNode(chase, BT_Sel);

			auto* owner = BB_Get("Owner", Actor*);
			auto* target = BB_SetGet("Target", const Actor*, ServiceLocator::GetInstance().GetPlayer());

			auto* running = &BB_SetGet("Running", BT_Act*, nullptr);

			auto& aggroRange = BB_Get("Aggro Range", float);
			auto& attackRange = BB_Get("Attack Range", float);
			auto& attackTime = BB_Get("Attack Time", float);
			auto& attackTimer = BB_SetGet("Attack Timer", float, static_cast<float>(attackTime));
			auto& moveSpeed = BB_Get("Move Speed", float);

			auto& shootPos = BB_SetGet("Shoot Pos", CommonUtilities::Vector3f, {});

			auto& detected = BB_SetGet("Detected", bool, false);
			auto& found = BB_SetGet("Found Player", bool, false);
			auto& ready = BB_SetGet("Ready To Fire", bool, false);

			BB_Set("Shoot Position", CommonUtilities::Vector3f, {});

			{
				// Add this node first if your tree does not execute from root every frame.
				// Make return either Success or Failure dependet on if root is a Sequence or Selector node.
				BT_NewNode(exit, BT_Act);
				exit->SetCondition([running]() -> BT_NodeStatus {

					if (*running)
					{
						(*running)->Exit();
						*running = nullptr;
					}

					return BT_NodeStatus::Success;
					});
				root->AddChild(std::move(exit));
			}

			/*idle*/ {

				idle->SetCondition([&outTree]() -> BT_NodeStatus
					{
						return BT_NodeStatus::Running;
					});
				idle->SetStart([&outTree, idle, owner, running]()
					{
						*running = idle.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsIdle", true);
					});

				idle->SetAction([&outTree]()
					{

					});

				idle->SetEnd([&outTree, owner]()
					{
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsIdle", false);
					});
			}

			/*detect*/ {

				detectPlayer->SetCondition([&outTree, &found]() -> BT_NodeStatus
					{
						if (found)
						{
							return BT_NodeStatus::Success;
						}

						return BT_NodeStatus::Running;
					});
				detectPlayer->SetStart([&outTree, detectPlayer, owner, running]()
					{
						*running = detectPlayer.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetTrigger("IsPlayerDetected");
					});

				detectPlayer->SetAction([&outTree, owner, &found]()
					{
						if (!owner->GetComponent<AnimatorComponent>()->GetCurrentPlayBackState(true).Active)
						{
							found = true;
						}
					});
			}

			/*moveToPlayer*/ {
				moveToPlayer->SetCondition([&outTree, owner, target]() -> BT_NodeStatus
					{
						if (target == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						if (owner == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						return BT_NodeStatus::Running;
					});

				moveToPlayer->SetStart([&outTree, moveToPlayer, owner, running]()
					{
						*running = moveToPlayer.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsRunning", true);
					});

				moveToPlayer->SetEnd([&outTree, owner]()
					{
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsRunning", false);
						//owner->GetComponent<ControllerComponent>()->SetWeight("Follow Path", 0);
					});
			}

			/*moveToShoot*/ {
				moveToShootPosition->SetCondition([&outTree, owner, &shootPos, &ready]() -> BT_NodeStatus
					{
						if (ready)
						{
							return BT_NodeStatus::Success;
						}

						return BT_NodeStatus::Running;
					});
				moveToShootPosition->SetStart([&outTree, running, moveToShootPosition, owner, target, &attackRange, &shootPos]()
					{
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsRunning", true);
						*running = moveToShootPosition.get();

						float DTR = CommonUtilities::Math::DEGREES_TO_RADIANS<float>;
						std::uniform_real_distribution<float> rnd(DTR * -90, DTR * 90);
						float angle = rnd(ServiceLocator::GetInstance().GetRngEngine());

						CommonUtilities::Vector3f direction = (owner->GetTransform().GetWorldPosition() - target->GetTransform().GetWorldPosition());
						direction.y = 0;
						direction = direction.GetNormalized() * attackRange;

						shootPos.x = direction.x * cosf(angle) - direction.z * sinf(angle);
						shootPos.z = direction.x * sinf(angle) + direction.z * cosf(angle);

						shootPos.y = 0;
						shootPos = shootPos + target->GetTransform().GetWorldPosition();

						BB_Set("Shoot Position", CommonUtilities::Vector3f, std::move(shootPos));
					});
				moveToShootPosition->SetAction([&outTree, owner, &shootPos, &moveSpeed, &ready]()
					{
						CommonUtilities::Vector3f direction = shootPos - owner->GetTransform().GetWorldPosition();

						direction.Normalize();
						CommonUtilities::Vector3f newPos = direction * moveSpeed * ServiceLocator::GetInstance().GetDeltaTime() + owner->GetTransform().GetWorldPosition();

						float rotation = atan2f(direction.x, direction.z);
						owner->GetTransform().SetLocalPosition(newPos);
						owner->GetTransform().SetLocalRotation({ 0.0f, rotation, 0.0f });

						if ((owner->GetTransform().GetWorldPosition() - shootPos).Length() < 10.f)
						{
							ready = true;
						}

					});
				moveToShootPosition->SetEnd([&outTree, owner]()
					{
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsRunning", false);
					});
			}

			/*shoot*/ {
				shootPlayer->SetCondition([&outTree, owner, target, &attackTimer, &ready]() -> BT_NodeStatus
					{
						if (target == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						if (owner == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						if (!ready)
						{
							return BT_NodeStatus::Success;
						}

						return BT_NodeStatus::Running;
					});
				shootPlayer->SetStart([&outTree, shootPlayer, owner, running]()
					{
						*running = shootPlayer.get();
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsAttacking", true);
					});

				shootPlayer->SetAction([&outTree, target, owner, &attackTimer, &ready]()
					{
						CommonUtilities::Vector3f direction = target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition();
						direction.y = 0;
						direction.Normalize();

						float rotation = atan2f(direction.x, direction.z);
						owner->GetTransform().SetLocalPosition(owner->GetTransform().GetWorldPosition());
						owner->GetTransform().SetLocalRotation({ 0.0f, rotation, 0.0f });

						attackTimer -= ServiceLocator::GetInstance().GetDeltaTime();

						if (attackTimer < 0)
						{
							ready = false;
						}
					});

				shootPlayer->SetEnd([&outTree, owner, running, &attackTimer, &attackTime, &ready]()
					{
						*running = nullptr;

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsAttacking", false);

						attackTimer = attackTime;

						ready = false;
					});
			}

			foundPlayer->SetCondition([&outTree, owner, target, &attackRange, &detected]() -> bool
				{
					if (detected || (target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition()).Length() < attackRange)
					{
						detected = true;
						return true;
					}
					else
					{
						return false;
					}
				});
			playerInLOS->SetCondition([&outTree, owner, target, &aggroRange]() -> bool
				{
					if ((target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition()).Length() < aggroRange)
					{
						return true;
					}
					else
					{
						return false;
					}
				});

			{
				detect->AddChild(foundPlayer);
				detect->AddChild(detectPlayer);

				look->AddChild(detect);
				look->AddChild(idle);

				chase->AddChild(playerInLOS);
				chase->AddChild(moveToPlayer);

				attack->AddChild(moveToShootPosition);
				attack->AddChild(shootPlayer);

				base->AddChild(look);
				base->AddChild(chase);
				base->AddChild(attack);

				root->AddChild(base);
			}

			outTree.SetRoot(std::move(root));
			};
	myBehaviourTrees["Enemy03"] = [](BehaviourTree& outTree)
		{
			BT_NewNode(idle, BT_Act);
			BT_NewNode(detectPlayer, BT_Act);
			BT_NewNode(moveToPlayer, BT_Act);
			BT_NewNode(attackPlayer, BT_Act);

			BT_NewNode(foundPlayer, BT_Con);
			BT_NewNode(playerInRange, BT_Con);

			BT_NewNode(root, BT_Seq);
			BT_NewNode(base, BT_Seq);
			BT_NewNode(detect, BT_Seq);

			BT_NewNode(look, BT_Sel);
			BT_NewNode(chase, BT_Sel);

			auto* owner = BB_Get("Owner", Actor*);
			auto* target = BB_SetGet("Target", const Actor*, ServiceLocator::GetInstance().GetPlayer());

			auto* running = &BB_SetGet("Running", BT_Act*, nullptr);

			auto& aggroRange = BB_Get("Aggro Range", float);
			auto& attackRange = BB_Get("Attack Range", float);
			auto& moveSpeed = BB_Get("Move Speed", float);

			auto& detected = BB_SetGet("Detected", bool, false);
			auto& found = BB_SetGet("Found Player", bool, false);

			BB_Set("Shoot Position", CommonUtilities::Vector3f, {});

			{
				// Add this node first if your tree does not execute from root every frame.
				// Make return either Success or Failure dependet on if root is a Sequence or Selector node.
				BT_NewNode(exit, BT_Act);
				exit->SetCondition([running]() -> BT_NodeStatus {

					if (*running)
					{
						(*running)->Exit();
						*running = nullptr;
					}

					return BT_NodeStatus::Success;
					});
				root->AddChild(std::move(exit));
			}

			/*idle*/ {

				idle->SetCondition([&outTree]() -> BT_NodeStatus
					{
						return BT_NodeStatus::Running;
					});
				idle->SetStart([&outTree, idle, owner, running]()
					{
						*running = idle.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsIdle", true);
					});

				idle->SetAction([&outTree]()
					{

					});

				idle->SetEnd([&outTree, owner]()
					{
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsIdle", false);
					});
			}

			/*detect*/ {

				detectPlayer->SetCondition([&outTree, &found]() -> BT_NodeStatus
					{
						if (found)
						{
							return BT_NodeStatus::Success;
						}

						return BT_NodeStatus::Running;
					});
				detectPlayer->SetStart([&outTree, detectPlayer, owner, running]()
					{
						*running = detectPlayer.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetTrigger("IsPlayerDetected");
					});

				detectPlayer->SetAction([&outTree, owner, &found]()
					{
						if (!owner->GetComponent<AnimatorComponent>()->GetCurrentPlayBackState(true).Active)
						{
							found = true;
						}
					});
			}

			/*moveToPlayer*/ {
				moveToPlayer->SetCondition([&outTree, owner, target]() -> BT_NodeStatus
					{
						if (target == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						if (owner == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						return BT_NodeStatus::Running;
					});

				moveToPlayer->SetStart([&outTree, moveToPlayer, owner, running]()
					{
						*running = moveToPlayer.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsRunning", true);
					});

				moveToPlayer->SetAction([&outTree, owner, target, &moveSpeed]()
					{
						CommonUtilities::Vector3f direction = target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition();

						direction.y = 0;

						direction.Normalize();
						CommonUtilities::Vector3f newPos = direction * moveSpeed * ServiceLocator::GetInstance().GetDeltaTime() + owner->GetTransform().GetWorldPosition();

						float rotation = atan2f(direction.x, direction.z);
						owner->GetTransform().SetLocalPosition(newPos);
						owner->GetTransform().SetLocalRotation({ 0.0f, rotation, 0.0f });
					});

				moveToPlayer->SetEnd([&outTree, owner]()
					{
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsRunning", false);
					});
			}

			/*attack*/ {
				attackPlayer->SetCondition([&outTree, owner, target]() -> BT_NodeStatus
					{
						if (target == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						if (owner == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						return BT_NodeStatus::Running;
					});
				attackPlayer->SetStart([&outTree, attackPlayer, owner, running]()
					{
						*running = attackPlayer.get();
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsAttacking", true);
					});

				attackPlayer->SetAction([&outTree, target, owner]()
					{
						CommonUtilities::Vector3f direction = target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition();
						direction.y = 0;
						direction.Normalize();

						float rotation = atan2f(direction.x, direction.z);
						owner->GetTransform().SetLocalPosition(owner->GetTransform().GetWorldPosition());
						owner->GetTransform().SetLocalRotation({ 0.0f, rotation, 0.0f });
					});

				attackPlayer->SetEnd([&outTree, owner, running]()
					{
						*running = nullptr;

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsAttacking", false);
					});
			}

			foundPlayer->SetCondition([&outTree, owner, target, &aggroRange, &detected]() -> bool
				{
					if (detected || (target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition()).Length() < aggroRange)
					{
						detected = true;
						return true;
					}
					else
					{
						return false;
					}
				});
			playerInRange->SetCondition([&outTree, owner, target, &attackRange]() -> bool
				{
					if ((target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition()).Length() < attackRange)
					{
						return true;
					}
					else
					{
						return false;
					}
				});

			{
				detect->AddChild(foundPlayer);
				detect->AddChild(detectPlayer);

				look->AddChild(detect);
				look->AddChild(idle);

				chase->AddChild(playerInRange);
				chase->AddChild(moveToPlayer);

				base->AddChild(look);
				base->AddChild(chase);
				base->AddChild(attackPlayer);

				root->AddChild(base);
			}

			outTree.SetRoot(std::move(root));

		};
	myBehaviourTrees["Enemy04"] = [](BehaviourTree& outTree)
		{
			BT_NewNode(idle, BT_Act);
			BT_NewNode(detectPlayer, BT_Act);
			BT_NewNode(moveToPlayer, BT_Act);
			BT_NewNode(attackPlayer, BT_Act);

			BT_NewNode(foundPlayer, BT_Con);
			BT_NewNode(playerInRange, BT_Con);

			BT_NewNode(root, BT_Seq);
			BT_NewNode(base, BT_Seq);
			BT_NewNode(detect, BT_Seq);

			BT_NewNode(look, BT_Sel);
			BT_NewNode(chase, BT_Sel);

			auto* owner = BB_Get("Owner", Actor*);
			auto* target = BB_SetGet("Target", const Actor*, ServiceLocator::GetInstance().GetPlayer());

			auto* running = &BB_SetGet("Running", BT_Act*, nullptr);

			auto& aggroRange = BB_Get("Aggro Range", float);
			auto& attackRange = BB_Get("Attack Range", float);
			auto& moveSpeed = BB_Get("Move Speed", float);

			auto& detected = BB_SetGet("Detected", bool, false);
			auto& found = BB_SetGet("Found Player", bool, false);

			BB_Set("Shoot Position", CommonUtilities::Vector3f, {});

			{
				// Add this node first if your tree does not execute from root every frame.
				// Make return either Success or Failure dependet on if root is a Sequence or Selector node.
				BT_NewNode(exit, BT_Act);
				exit->SetCondition([running]() -> BT_NodeStatus {

					if (*running)
					{
						(*running)->Exit();
						*running = nullptr;
					}

					return BT_NodeStatus::Success;
					});
				root->AddChild(std::move(exit));
			}

			/*idle*/ {

				idle->SetCondition([&outTree]() -> BT_NodeStatus
					{
						return BT_NodeStatus::Running;
					});
				idle->SetStart([&outTree, idle, owner, running]()
					{
						*running = idle.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsIdle", true);
					});

				idle->SetAction([&outTree]()
					{

					});

				idle->SetEnd([&outTree, owner]()
					{
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsIdle", false);
					});
			}

			/*detect*/ {

				detectPlayer->SetCondition([&outTree, &found]() -> BT_NodeStatus
					{
						if (found)
						{
							return BT_NodeStatus::Success;
						}

						return BT_NodeStatus::Running;
					});
				detectPlayer->SetStart([&outTree, detectPlayer, owner, running]()
					{
						*running = detectPlayer.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetTrigger("IsPlayerDetected");
					});

				detectPlayer->SetAction([&outTree, owner, &found]()
					{
						if (!owner->GetComponent<AnimatorComponent>()->GetCurrentPlayBackState(true).Active)
						{
							found = true;
						}
					});
			}

			/*moveToPlayer*/ {
				moveToPlayer->SetCondition([&outTree, owner, target]() -> BT_NodeStatus
					{
						if (target == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						if (owner == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						return BT_NodeStatus::Running;
					});

				moveToPlayer->SetStart([&outTree, moveToPlayer, owner, running]()
					{
						*running = moveToPlayer.get();

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsRunning", true);
					});

				moveToPlayer->SetAction([&outTree, owner, target, &moveSpeed]()
					{
						CommonUtilities::Vector3f direction = target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition();

						direction.y = 0;

						direction.Normalize();
						CommonUtilities::Vector3f newPos = direction * moveSpeed * ServiceLocator::GetInstance().GetDeltaTime() + owner->GetTransform().GetWorldPosition();

						float rotation = atan2f(direction.x, direction.z);
						owner->GetTransform().SetLocalPosition(newPos);
						owner->GetTransform().SetLocalRotation({ 0.0f, rotation, 0.0f });
					});

				moveToPlayer->SetEnd([&outTree, owner]()
					{
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsRunning", false);
					});
			}

			/*attack*/ {
				attackPlayer->SetCondition([&outTree, owner, target]() -> BT_NodeStatus
					{
						if (target == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						if (owner == nullptr)
						{
							return BT_NodeStatus::Failure;
						}

						return BT_NodeStatus::Running;
					});
				attackPlayer->SetStart([&outTree, attackPlayer, owner, running]()
					{
						*running = attackPlayer.get();
						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsAttacking", true);
					});

				attackPlayer->SetAction([&outTree, target, owner]()
					{
						CommonUtilities::Vector3f direction = target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition();
						direction.y = 0;
						direction.Normalize();

						float rotation = atan2f(direction.x, direction.z);
						owner->GetTransform().SetLocalPosition(owner->GetTransform().GetWorldPosition());
						owner->GetTransform().SetLocalRotation({ 0.0f, rotation, 0.0f });
					});

				attackPlayer->SetEnd([&outTree, owner, running]()
					{
						*running = nullptr;

						owner->GetComponent<AnimatorComponent>()->GetTree().SetBool("IsAttacking", false);
					});
			}

			foundPlayer->SetCondition([&outTree, owner, target, &aggroRange, &detected]() -> bool
				{
					if (detected || (target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition()).Length() < aggroRange)
					{
						detected = true;
						return true;
					}
					else
					{
						return false;
					}
				});
			playerInRange->SetCondition([&outTree, owner, target, &attackRange]() -> bool
				{
					if ((target->GetTransform().GetWorldPosition() - owner->GetTransform().GetWorldPosition()).Length() < attackRange)
					{
						return true;
					}
					else
					{
						return false;
					}
				});

			{
				detect->AddChild(foundPlayer);
				detect->AddChild(detectPlayer);

				look->AddChild(detect);
				look->AddChild(idle);

				chase->AddChild(playerInRange);
				chase->AddChild(moveToPlayer);

				base->AddChild(look);
				base->AddChild(chase);
				base->AddChild(attackPlayer);

				root->AddChild(base);
			}

			outTree.SetRoot(std::move(root));

		};
}

void BehaviourTreeFactory::SetBehaviourTree(const std::string& aTree, BehaviourTree& outTree)
{
	if (aTree.empty())
	{
		return;
	}

	myBehaviourTrees[aTree](outTree);
}

