/*
* Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "GameTime.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "ulduar.h"
#include "Vehicle.h"

enum Yells
{
	SAY_AGGRO = 0,
	SAY_SUMMON = 1,
	SAY_SLAG_POT = 2,
	SAY_SCORCH = 3,
	SAY_SLAY = 4,
	SAY_BERSERK = 5,
	SAY_DEATH = 6,
	EMOTE_JETS = 7
};

enum Spells
{
	SPELL_FLAME_JETS = 62680,
	SPELL_SCORCH = 62546,
	SPELL_SLAG_POT = 62717,
	SPELL_SLAG_POT_DAMAGE = 65722,
	SPELL_SLAG_IMBUED = 62836,
	SPELL_ACTIVATE_CONSTRUCT = 62488,
	SPELL_STRENGHT = 64473,
	SPELL_GRAB = 62707,
	SPELL_BERSERK = 47008,

	// Iron Construct
	SPELL_HEAT = 65667,
	SPELL_MOLTEN = 62373,
	SPELL_BRITTLE = 67114,
	SPELL_SHATTER = 62383,
	SPELL_GROUND = 62548,
	SPELL_FREEZE_ANIM = 63354,
};

enum Events
{
	EVENT_JET = 1,
	EVENT_SCORCH = 2,
	EVENT_SLAG_POT = 3,
	EVENT_GRAB_POT = 4,
	EVENT_CHANGE_POT = 5,
	EVENT_END_POT = 6,
	EVENT_CONSTRUCT = 7,
	EVENT_BERSERK = 8,
};

enum Actions
{
	ACTION_REMOVE_BUFF = 20,
};

enum Creatures
{
	NPC_IRON_CONSTRUCT = 33121,
	NPC_GROUND_SCORCH = 33221,
};

enum AchievementData
{
	DATA_SHATTERED = 29252926,
	ACHIEVEMENT_IGNIS_START_EVENT = 20951,
};

#define CONSTRUCT_SPAWN_POINTS 20

Position const ConstructSpawnPosition[CONSTRUCT_SPAWN_POINTS] =
{
	{ 630.366f, 216.772f, 360.891f, 3.001970f },
	{ 630.594f, 231.846f, 360.891f, 3.124140f },
	{ 630.435f, 337.246f, 360.886f, 3.211410f },
	{ 630.493f, 313.349f, 360.886f, 3.054330f },
	{ 630.444f, 321.406f, 360.886f, 3.124140f },
	{ 630.366f, 247.307f, 360.888f, 3.211410f },
	{ 630.698f, 305.311f, 360.886f, 3.001970f },
	{ 630.500f, 224.559f, 360.891f, 3.054330f },
	{ 630.668f, 239.840f, 360.890f, 3.159050f },
	{ 630.384f, 329.585f, 360.886f, 3.159050f },
	{ 543.220f, 313.451f, 360.886f, 0.104720f },
	{ 543.356f, 329.408f, 360.886f, 6.248280f },
	{ 543.076f, 247.458f, 360.888f, 6.213370f },
	{ 543.117f, 232.082f, 360.891f, 0.069813f },
	{ 543.161f, 305.956f, 360.886f, 0.157080f },
	{ 543.277f, 321.482f, 360.886f, 0.052360f },
	{ 543.316f, 337.468f, 360.886f, 6.195920f },
	{ 543.280f, 239.674f, 360.890f, 6.265730f },
	{ 543.265f, 217.147f, 360.891f, 0.174533f },
	{ 543.256f, 224.831f, 360.891f, 0.122173f },
};

class boss_ignis : public CreatureScript
{
public:
	boss_ignis() : CreatureScript("boss_ignis") { }

	struct boss_ignis_AI : public BossAI
	{
		boss_ignis_AI(Creature* creature) : BossAI(creature, BOSS_IGNIS), _vehicle(me->GetVehicleKit())
		{
			ASSERT(_vehicle);

			for (int32 i = 0; i < CONSTRUCT_SPAWN_POINTS; ++i)
				DoSummon(NPC_IRON_CONSTRUCT, ConstructSpawnPosition[i]);
		}

		void Reset() override
		{
			//DespawnConstructors();

			_Reset();
			if (_vehicle)
				_vehicle->RemoveAllPassengers();

			instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEVEMENT_IGNIS_START_EVENT);

			std::list<Creature*> pCreatureList;

			Trinity::AllCreaturesOfEntryInRange checker(me, 33121, 200.0f);
			Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(me, pCreatureList, checker);

			me->VisitNearbyObject(400.0f, searcher);

			if (pCreatureList.empty())
				return;

			if (!pCreatureList.empty())
			{
				for (std::list<Creature*>::iterator i = pCreatureList.begin(); i != pCreatureList.end(); ++i)
					(*i)->DespawnOrUnsummon();
			}
			
			for (int32 i = 0; i < CONSTRUCT_SPAWN_POINTS; ++i)
			{				
				DoSummon(NPC_IRON_CONSTRUCT, ConstructSpawnPosition[i]);
			}
				

		}

			void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			Talk(SAY_AGGRO);
			events.ScheduleEvent(EVENT_JET, 30000);
			events.ScheduleEvent(EVENT_SCORCH, 25000);
			events.ScheduleEvent(EVENT_SLAG_POT, 35000);
			events.ScheduleEvent(EVENT_CONSTRUCT, urand(29000, 30000));
			events.ScheduleEvent(EVENT_END_POT, 40000);
			events.ScheduleEvent(EVENT_BERSERK, 480000);
			//_slagPotGUID = 0;
			_shattered = false;
			_firstConstructKill = 0;
			instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEVEMENT_IGNIS_START_EVENT);
		}

			void JustDied(Unit* /*killer*/) override
		{
			_JustDied();
			Talk(SAY_DEATH);
		}

			uint32 GetData(uint32 type) const override
		{
			if (type == DATA_SHATTERED)
			return _shattered ? 1 : 0;

			return 0;
		}

			void KilledUnit(Unit* who) override
		{
			if (who->GetTypeId() == TYPEID_PLAYER)
			Talk(SAY_SLAY);
		}


			void DoAction(int32 action) override
		{
			if (action != ACTION_REMOVE_BUFF)
			return;

			me->RemoveAuraFromStack(SPELL_STRENGHT);
			// Shattered Achievement
			time_t secondKill = GameTime::GetGameTime();
			if ((secondKill - _firstConstructKill) < 5)
				_shattered = true;
			_firstConstructKill = secondKill;
		}

			void UpdateAI(uint32 diff) override
		{
			if (!UpdateVictim())
			return;

			events.Update(diff);

			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_JET:
					me->StopMoving();
					Talk(EMOTE_JETS);
					DoCast(me, SPELL_FLAME_JETS);
					events.ScheduleEvent(EVENT_JET, urand(21000, 33000));
					break;
				case EVENT_SLAG_POT:
					if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me))) //NonTankTargetSelector(me)
					{
						Talk(SAY_SLAG_POT);
						_slagPotGUID = target->GetGUID();
						DoCast(target, SPELL_GRAB);
						events.DelayEvents(3000);
						events.ScheduleEvent(EVENT_GRAB_POT, 500);
					}
					events.ScheduleEvent(EVENT_SLAG_POT, RAID_MODE(30000, 15000));
					break;
				case EVENT_GRAB_POT:
					if (Unit* slagPotTarget = ObjectAccessor::GetUnit(*me, _slagPotGUID))
					{
						slagPotTarget->EnterVehicle(me, 0);
						events.CancelEvent(EVENT_GRAB_POT);
						events.ScheduleEvent(EVENT_CHANGE_POT, 1000);
					}
					break;
				case EVENT_CHANGE_POT:
					if (Unit* slagPotTarget = ObjectAccessor::GetUnit(*me, _slagPotGUID))
					{
						DoCast(slagPotTarget, SPELL_SLAG_POT);
						//slagPotTarget->AddAura(SPELL_SLAG_POT, slagPotTarget);
						slagPotTarget->EnterVehicle(me, 1);
						events.CancelEvent(EVENT_CHANGE_POT);
						events.ScheduleEvent(EVENT_END_POT, 10000);
					}
					break;
				case EVENT_END_POT:
					if (Unit* slagPotTarget = ObjectAccessor::GetUnit(*me, _slagPotGUID))
					{
						slagPotTarget->ExitVehicle();
						slagPotTarget = NULL;
						_slagPotGUID = ObjectGuid::Empty; // CUIDADO CON �STO.
						events.CancelEvent(EVENT_END_POT);
					}
					break;
				case EVENT_SCORCH:
					me->StopMoving();
					Talk(SAY_SCORCH);
					if (Unit* target = me->GetVictim())
						me->SummonCreature(NPC_GROUND_SCORCH, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 45000);
					DoCast(SPELL_SCORCH);
					events.ScheduleEvent(EVENT_SCORCH, 22000);
					break;
				case EVENT_CONSTRUCT:
					Talk(SAY_SUMMON);
					DoCast(SPELL_STRENGHT);
					DoCast(me, SPELL_ACTIVATE_CONSTRUCT);
					events.ScheduleEvent(EVENT_CONSTRUCT, 29000);
					break;
				case EVENT_BERSERK:
					DoCast(me, SPELL_BERSERK, true);
					Talk(SAY_BERSERK);
					break;
				}
			}

			DoMeleeAttackIfReady();

			EnterEvadeIfOutOfCombatArea(diff);
		}

	private:
		ObjectGuid _slagPotGUID;
		Vehicle* _vehicle;
		time_t _firstConstructKill;
		bool _shattered;


	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetUlduarAI<boss_ignis_AI>(creature);
	}
};

class npc_iron_construct : public CreatureScript
{
public:
	npc_iron_construct() : CreatureScript("npc_iron_construct") { }

	struct npc_iron_constructAI : public ScriptedAI
	{
		npc_iron_constructAI(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript())
		{
			creature->AddAura(SPELL_FREEZE_ANIM, creature);
			creature->SetReactState(REACT_PASSIVE);
		}

		void Reset() override
		{
			_brittled = false;
		}

			/*void JustReachedHome()
			{
			//
			me->AddAura(SPELL_FREEZE_ANIM, me);
			//me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED | UNIT_FLAG_DISABLE_MOVE);
			me->SetReactState(REACT_PASSIVE);
			}*/
			void JustDied(Unit* /*victim*/) override
		{
			//me->SummonCreature(NPC_IRON_CONSTRUCT, me->GetHomePosition(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);

		}

		void DamageTaken(Unit* /*attacker*/, uint32& damage) override
		{
			if (me->HasAura(SPELL_BRITTLE) && damage >= 3000)
			{
				DoCast(SPELL_SHATTER);
				if (Creature* ignis = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(BOSS_IGNIS)))
					if (ignis->AI())
						ignis->AI()->DoAction(ACTION_REMOVE_BUFF);

				me->DespawnOrUnsummon(1000);
			}
			else if (me->HasAura(62382) && damage >= 5000)
			{
				DoCast(SPELL_SHATTER);
				if (Creature* ignis = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(BOSS_IGNIS)))
					if (ignis->AI())
						ignis->AI()->DoAction(ACTION_REMOVE_BUFF);

				me->DespawnOrUnsummon(1000);
			}
		}

			void SpellHit(Unit* caster, SpellInfo const* spell) override
		{

			if (spell->Id != SPELL_ACTIVATE_CONSTRUCT)
			return;

			me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_PACIFIED | UNIT_FLAG_STUNNED | UNIT_FLAG_DISABLE_MOVE);
			me->RemoveAurasDueToSpell(SPELL_FREEZE_ANIM);
			me->setFaction(16);
			me->SetReactState(REACT_AGGRESSIVE);
			DoZoneInCombat();

			if (Creature* ignis = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(BOSS_IGNIS)))
			{
				me->AI()->AttackStart(ignis->GetVictim());
			}

		}

			void UpdateAI(uint32 /*uiDiff*/) override
		{
			if (!UpdateVictim())
			return;

			if (Aura* aur = me->GetAura(SPELL_HEAT))
			{
				if (aur->GetStackAmount() >= 14)
				{
					me->RemoveAura(SPELL_HEAT);
					DoCast(SPELL_MOLTEN);
					me->getThreatManager().resetAllAggro(); // Reset the aggro.
					_brittled = false;
				}
			}
			// COMPROBAR QUE SE QUITA EL STUN BRITTLE A LOS 20 SEGUNDOS, Y SI SE REVIENTA QUE HAGA DA�O.

			/*



			PARA SPELLS COMO LOS SCORCH Y DEM�S HACER un custom spell con el da�o modificado en funci�n a los stacks.

			Revisar unos que hiciste con damage, el da�o modificado.



			*/
			// Water pools
			if (me->IsInWater() && !_brittled && me->HasAura(SPELL_MOLTEN)) //62382 en 10 67114 en 25
			{

				DoCast(SPELL_BRITTLE);
				me->RemoveAura(SPELL_MOLTEN);
				_brittled = true;
			}

			DoMeleeAttackIfReady();
		}

	private:
		InstanceScript* _instance;
		bool _brittled;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetUlduarAI<npc_iron_constructAI>(creature);
	}
};

class npc_scorch_ground : public CreatureScript
{
public:
	npc_scorch_ground() : CreatureScript("npc_scorch_ground") { }

	struct npc_scorch_groundAI : public ScriptedAI
	{
		npc_scorch_groundAI(Creature* creature) : ScriptedAI(creature)
		{
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_PACIFIED);
			creature->SetDisplayId(16925); //model 2 in db cannot overwrite wdb fields
		}


		/*			case :
		case 63477*/
		void MoveInLineOfSight(Unit* who) override

		{
			if (!_heat)
			{
				if (who->GetEntry() == NPC_IRON_CONSTRUCT)
				{
					if (!who->HasAura(SPELL_HEAT) || !who->HasAura(SPELL_MOLTEN))
					{
						_constructGUID = who->GetGUID();
						_heat = true;
					}
				}
			}
		}

			//If the target who is hit with the scorch spell has slag pot aura, do not perform damage to him.


			void Reset() override
		{
			_heat = false;

			DoCast(me, RAID_MODE(62548, 63476));
			//_constructGUID = 0;
			_heatTimer = 0;
		}

			void UpdateAI(uint32 uiDiff) override
		{
			if (_heat)
			{
				if (_heatTimer <= uiDiff)
				{
					Creature* construct = ObjectAccessor::GetCreature(*me, _constructGUID);
					if (construct && construct->GetReactState() == REACT_AGGRESSIVE)
					{
						if (!construct->HasAura(SPELL_MOLTEN)) // SI TIENEN EL MOLTEN NO PUEDEN TENER EL HEAT, ESO ES POR LOS SPELLS ID PROBABLMENTE DE 25 Y 10
						{
							me->AddAura(SPELL_HEAT, construct);
							_heatTimer = 1000;
						}
					}

				}
				else
					_heatTimer -= uiDiff;
			}
		}

	private:
		ObjectGuid _constructGUID;
		uint32 _heatTimer;
		bool _heat;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetUlduarAI<npc_scorch_groundAI>(creature);
	}
};

class spell_ignis_slag_pot : public SpellScriptLoader
{
public:
	spell_ignis_slag_pot() : SpellScriptLoader("spell_ignis_slag_pot")
	{
		//instance = me->GetInstanceScript();

	}

	class spell_ignis_slag_pot_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_ignis_slag_pot_AuraScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_SLAG_POT_DAMAGE)
			|| !sSpellMgr->GetSpellInfo(SPELL_SLAG_IMBUED))
			return false;
			return true;
		}

			void HandleEffectPeriodic(AuraEffect const* aurEff)
		{
			//GetCaster()->Kill(aurEff->GetCaster());


			Unit* aurEffCaster = aurEff->GetCaster();
			if (!aurEffCaster)
				return;

			Unit* target = GetTarget();
			if (InstanceScript* instance = GetCaster()->GetInstanceScript())
			{
				if (instance->instance->Is25ManRaid())
					aurEffCaster->CastSpell(target, 65723, true);
				else
					aurEffCaster->CastSpell(target, SPELL_SLAG_POT_DAMAGE, true);
			}
		}

		void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
		{
			if (GetTarget()->IsAlive())
			{
				GetTarget()->CastSpell(GetTarget(), SPELL_SLAG_IMBUED, true);

				if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
				{
					if (InstanceScript* instance = GetCaster()->GetInstanceScript())
					{
						if (instance->instance->Is25ManRaid())
						{
							if (AchievementEntry const* HotPocket = sAchievementMgr->GetAchievement(2928)) /* Write this in a more elegant way through DEFINES, achievement IDs. */
								GetTarget()->ToPlayer()->CompletedAchievement(HotPocket);
						}
						else
						{
							if (AchievementEntry const* HotPocket = sAchievementMgr->GetAchievement(2927)) /* Write this in a more elegant way through DEFINES, achievement IDs. */
								GetTarget()->ToPlayer()->CompletedAchievement(HotPocket);
						}
					}
				}
			}
		}

		void Register() override
		{
			OnEffectPeriodic += AuraEffectPeriodicFn(spell_ignis_slag_pot_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
			AfterEffectRemove += AuraEffectRemoveFn(spell_ignis_slag_pot_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
		}
	private:
		InstanceScript* instance;
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_ignis_slag_pot_AuraScript();
	}
};

class achievement_ignis_shattered : public AchievementCriteriaScript
{
public:
	achievement_ignis_shattered() : AchievementCriteriaScript("achievement_ignis_shattered") { }

	bool OnCheck(Player* /*source*/, Unit* target) override
	{
		if (target && target->IsAIEnabled)
		return target->GetAI()->GetData(DATA_SHATTERED);

		return false;
	}
};

//GC

// Selector removes targets that are neither player not pets
class spell_ignis_flame_jets : public SpellScriptLoader
{
public:
	spell_ignis_flame_jets() : SpellScriptLoader("spell_ignis_flame_jets") { }

	class spell_ignis_flame_jets_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_ignis_flame_jets_SpellScript);

		void FilterTargets(std::list<WorldObject*>& targets)
		{
			targets.remove_if(PlayerOrPetCheck());
		}

		void Register()
		{
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ignis_flame_jets_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ignis_flame_jets_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ignis_flame_jets_SpellScript::FilterTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
		}
	};

	SpellScript* GetSpellScript() const
	{
		return new spell_ignis_flame_jets_SpellScript();
	}
};

class ActiveConstructFilter
{
public:
	bool operator()(WorldObject* target) const
	{
		return target->ToCreature()->GetReactState() == REACT_AGGRESSIVE;
	}
};


class spell_ignis_activate_construct : public SpellScriptLoader
{
public:
	spell_ignis_activate_construct() : SpellScriptLoader("spell_ignis_activate_construct") {}

	class spell_ignis_activate_construct_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_ignis_activate_construct_SpellScript);

		void FilterTargets(std::list<WorldObject*>& targets)
		{
			//
			targets.remove_if(ActiveConstructFilter());
			if (targets.size() > 1)
			{
				if (WorldObject* _target = Trinity::Containers::SelectRandomContainerElement(targets))
				{
					targets.clear();
					targets.push_back(_target);

					/*if (Creature* construct = GetCaster()->SummonCreature(NPC_IRON_CONSTRUCT, _target->GetPositionX(), _target->GetPositionY(), _target->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000)) //ConstructSpawnPosition[urand(0, CONSTRUCT_SPAWN_POINTS - 1)]
					construct->RemoveAura(59123);*/
				}
			}

		}

		void Register()
		{
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ignis_activate_construct_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
		}
	};

	SpellScript* GetSpellScript() const
	{
		return new spell_ignis_activate_construct_SpellScript();
	}
};

class GroundScorchFilter
{
public:
	bool operator()(WorldObject* target) const
	{
		if (target->ToPlayer())
			if (target->ToPlayer()->HasAura(63477) || target->ToPlayer()->HasAura(62717))//if (target->ToPlayer()->HasAura(63477) || target->ToPlayer()->HasAura(62717))
				return true;

		return false;
	}
};

class spell_ignis_ground_scorch : public SpellScriptLoader
{
public:
	spell_ignis_ground_scorch() : SpellScriptLoader("spell_ignis_ground_scorch") {}

	class spell_ignis_ground_scorch_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_ignis_ground_scorch_SpellScript);

		void FilterTargets(std::list<WorldObject*>& targets)
		{

			targets.remove_if(GroundScorchFilter());

		}

		void Register()
		{
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ignis_ground_scorch_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
		}
	};

	SpellScript* GetSpellScript() const
	{
		return new spell_ignis_ground_scorch_SpellScript();
	}
};

void AddSC_boss_ignis()
{
	new boss_ignis();
	new npc_iron_construct();
	new npc_scorch_ground();
	new spell_ignis_slag_pot();
	new spell_ignis_ground_scorch(); // 62548 63475
	new achievement_ignis_shattered();

	new spell_ignis_activate_construct();
	new spell_ignis_flame_jets();
}
