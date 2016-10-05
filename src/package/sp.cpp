#include "sp.h"
#include "client.h"
#include "general.h"
#include "skill.h"
#include "standard-skillcards.h"
#include "engine.h"
#include "maneuvering.h"

#include "settings.h"
#include "json.h"

class dummyVS : public ZeroCardViewAsSkill
{
public:
    dummyVS() : ZeroCardViewAsSkill("dummy")
    {
    }

    const Card *viewAs() const
    {
        return NULL;
    }
};

class Jilei : public TriggerSkill
{
public:
    Jilei() : TriggerSkill("jilei")
    {
        events << Damaged;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *yangxiu, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *current = room->getCurrent();
        if (!current || current->getPhase() == Player::NotActive || current->isDead() || !damage.from)
            return false;

        if (room->askForSkillInvoke(yangxiu, objectName(), QVariant::fromValue(damage.from))) {
            yangxiu->broadcastSkillInvoke(objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, yangxiu->objectName(), damage.from->objectName());
            QString choice = room->askForChoice(yangxiu, objectName(), "BasicCard+EquipCard+TrickCard");
            

            LogMessage log;
            log.type = "#Jilei";
            log.from = damage.from;
            log.arg = choice;
            room->sendLog(log);

            QStringList jilei_list = damage.from->tag[objectName()].toStringList();
            if (jilei_list.contains(choice)) return false;
            jilei_list.append(choice);
            damage.from->tag[objectName()] = QVariant::fromValue(jilei_list);
            QString _type = choice + "|.|.|hand"; // Handcards only
            room->setPlayerCardLimitation(damage.from, "use,response,discard", _type, true);

            QString type_name = choice.replace("Card", "").toLower();
            room->addPlayerTip(damage.from, "#jilei_" + type_name);
        }

        return false;
    }
};

class JileiClear : public TriggerSkill
{
public:
    JileiClear() : TriggerSkill("#jilei-clear")
    {
        events << EventPhaseChanging << Death;
    }

    int getPriority(TriggerEvent) const
    {
        return 5;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != target || target != room->getCurrent())
                return false;
        }
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach (ServerPlayer *player, players) {
            QStringList jilei_list = player->tag["jilei"].toStringList();
            if (!jilei_list.isEmpty()) {
                LogMessage log;
                log.type = "#JileiClear";
                log.from = player;
                room->sendLog(log);

                foreach (QString jilei_type, jilei_list) {
                    room->removePlayerCardLimitation(player, "use,response,discard", jilei_type + "|.|.|hand$1");
                    QString type_name = jilei_type.replace("Card", "").toLower();
                    room->removePlayerTip(player, "#jilei_" + type_name);
                }
                player->tag.remove("jilei");
            }
        }

        return false;
    }
};

class Danlao : public TriggerSkill
{
public:
    Danlao() : TriggerSkill("danlao")
    {
        events << TargetConfirmed;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length() <= 1 || !use.to.contains(player)
            || !use.card->isKindOf("TrickCard")
            || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        player->broadcastSkillInvoke(objectName());
        player->setFlags("-DanlaoTarget");
        player->setFlags("DanlaoTarget");
        player->drawCards(1, objectName());
        if (player->isAlive() && player->hasFlag("DanlaoTarget")) {
            player->setFlags("-DanlaoTarget");
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class Yongsi : public TriggerSkill
{
public:
	Yongsi() : TriggerSkill("yongsi")
	{
		events << DrawNCards << EventPhaseStart;
		frequency = Compulsory;
	}

	bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *yuanshu, QVariant &data) const
	{
		if (triggerEvent == DrawNCards) {
			QSet<QString> kingdom_set;
			foreach(ServerPlayer *p, room->getAlivePlayers())
				kingdom_set << p->getKingdom();

			data = kingdom_set.size();

			room->sendCompulsoryTriggerLog(yuanshu, objectName());

            yuanshu->broadcastSkillInvoke(objectName());
		} else if (triggerEvent == EventPhaseStart && yuanshu->getPhase() == Player::Discard) {
			room->sendCompulsoryTriggerLog(yuanshu, objectName());
            yuanshu->broadcastSkillInvoke(objectName());
			if (!room->askForDiscard(yuanshu, "yongsi", 1, 1, true, true, "@yongsi-discard"))
				room->loseHp(yuanshu);
		}

		return false;
	}
};

class SpJixi : public TriggerSkill
{
public:
    SpJixi() : TriggerSkill("spjixi")
    {
        events << EventPhaseChanging;
		frequency = Wake;
	}

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill(objectName())
            && target->isAlive() && target->getMark("jixi_turns") > 1 && !target->hasFlag("JixiHasLoseHp")
            && target->getMark(objectName()) == 0;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *yuanshu, QVariant &data) const
    {
		if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
            room->sendCompulsoryTriggerLog(yuanshu, objectName());
            yuanshu->broadcastSkillInvoke(objectName());
			room->setPlayerMark(yuanshu, objectName(), 1);
            if (room->changeMaxHpForAwakenSkill(yuanshu, 1)) {
                room->recover(yuanshu, RecoverStruct(yuanshu));
                if (room->askForChoice(yuanshu, objectName(), "wangzun+draw", data) == "wangzun")
					room->acquireSkill(yuanshu, "wangzun");
				else {
					yuanshu->drawCards(2);
					if (!isNormalGameMode(room->getMode())) return false;
                    ServerPlayer *the_lord = room->getLord();
					if (the_lord == NULL) return false;
					QStringList acquireList;
					foreach (const Skill *skill, the_lord->getVisibleSkillList()) {
						if (!skill->isLordSkill()) continue;
						QString skill_name = skill->objectName();
						if (skill->getFrequency() == Skill::Wake && the_lord->getMark(skill_name) > 0) continue;
						if (skill->getFrequency() == Skill::Limited && the_lord->getMark(skill->getLimitMark()) < 1) continue;
						acquireList.append(skill_name);
					}
					if (!acquireList.isEmpty())
						room->handleAcquireDetachSkills(yuanshu, acquireList);
				}
            }
        }
        return false;
    }
};

class SpJixiRecord : public TriggerSkill
{
public:
    SpJixiRecord() : TriggerSkill("#spjixi-record")
    {
        events << EventPhaseChanging << HpLost;
        global = true;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == EventPhaseChanging)
            return 1;

        return TriggerSkill::getPriority(triggerEvent);
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *yuanshu, QVariant &data) const
    {
		if (triggerEvent == HpLost && yuanshu->getPhase() != Player::NotActive) {
            room->setPlayerFlag(yuanshu, "JixiHasLoseHp");
			room->setPlayerMark(yuanshu, "jixi_turns", 0);
		} else if (triggerEvent == EventPhaseChanging && !yuanshu->hasFlag("JixiHasLoseHp")) {
			if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
				room->addPlayerMark(yuanshu, "jixi_turns");
			}
		}
        return false;
    }
};

class Liangzhu : public TriggerSkill
{
public:
    Liangzhu() : TriggerSkill("liangzhu")
    {
        events << HpRecover;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->getPhase() == Player::Play;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        foreach (ServerPlayer *sun, room->getAlivePlayers()) {
            if (!TriggerSkill::triggerable(sun)) continue;
            QString choice = room->askForChoice(sun, objectName(), "draw+letdraw+cancel", QVariant::fromValue(player), "@liangzhu-choose::"+player->objectName());
            if (choice != "cancel") {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = sun;
                log.arg = objectName();
                room->sendLog(log);
                sun->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(sun, objectName());
                if (choice == "draw") {
                    sun->drawCards(1, objectName());
                } else if (choice == "letdraw") {
                    player->drawCards(2, objectName());
                    room->setPlayerMark(player, "yuan", 1);
                }
            }
        }
        return false;
    }
};

class Fanxiang : public TriggerSkill
{
public:
    Fanxiang() : TriggerSkill("fanxiang")
    {
        events << EventPhaseStart;
        frequency = Skill::Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return (target != NULL && target->hasSkill(this) && target->getPhase() == Player::Start && target->getMark("@fanxiang") == 0);
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        bool flag = false;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getMark("yuan") > 0 && p->isWounded()) {
                flag = true;
                break;
            }
        }
        if (flag) {
			room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());

            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getMark("yuan") > 0)
                    room->setPlayerMark(p, "yuan", 0);
            }

            //room->doLightbox("$fanxiangAnimate", 5000);
            room->notifySkillInvoked(player, objectName());
            room->setPlayerMark(player, "fanxiang", 1);
            if (room->changeMaxHpForAwakenSkill(player, 1) && player->getMark("fanxiang") > 0) {
                room->recover(player, RecoverStruct(player));
                room->handleAcquireDetachSkills(player, "-liangzhu|xiaoji");
            }
        }
        return false;
    }
};

JuesiCard::JuesiCard()
{
    will_throw = true;
}

bool JuesiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->inMyAttackRange(to_select) && !to_select->isNude();
}

void JuesiCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    ServerPlayer *target = effect.to;
	const Card *card = room->askForCard(target, "..!", "@juesi-discard:" + source->objectName(), QVariant());
	if (!card->isKindOf("Slash") && source->getHp() <= target->getHp()) {
		Duel *duel = new Duel(Card::NoSuit, 0);
		duel->setSkillName("_juesi");
		if (!source->isCardLimited(duel, Card::MethodUse) && !source->isProhibited(target, duel))
			room->useCard(CardUseStruct(duel, source, target));
		else
			delete duel;
	}
}

class Juesi : public OneCardViewAsSkill
{
public:
    Juesi() : OneCardViewAsSkill("juesi")
    {
        filter_pattern = "Slash";
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        JuesiCard *skillcard = new JuesiCard;
        skillcard->addSubcard(originalCard);
        return skillcard;
    }
};

class Nuzhan : public TriggerSkill
{
public:
    Nuzhan() : TriggerSkill("nuzhan")
    {
        events << PreCardUsed << CardUsed << ConfirmDamage;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (TriggerSkill::triggerable(use.from)) {
                if (use.card != NULL && use.card->isKindOf("Slash") && use.card->isVirtualCard() && use.card->subcardsLength() == 1 && Sanguosha->getCard(use.card->getSubcards().first())->isKindOf("TrickCard") && use.m_addHistory) {
                    room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                    use.m_addHistory = false;
                    data = QVariant::fromValue(use);
                }
            }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (TriggerSkill::triggerable(use.from)) {
                if (use.card != NULL && use.card->isKindOf("Slash") && use.card->isVirtualCard() && use.card->subcardsLength() == 1 && Sanguosha->getCard(use.card->getSubcards().first())->isKindOf("EquipCard"))
                    use.card->setFlags("nuzhan_slash");
            }
        } else if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL && damage.card->hasFlag("nuzhan_slash")) {
                ++damage.damage;
                data = QVariant::fromValue(damage);
            }
        }
        return false;
    }
};

class Nuzhan_tms : public TargetModSkill 
{
public:
    Nuzhan_tms() : TargetModSkill("#nuzhan") {

    }

    virtual int getResidueNum(const Player *from, const Card *card) const {
        if (from->hasSkill("nuzhan")) {
            if ((card->isVirtualCard() && card->subcardsLength() == 1 && Sanguosha->getCard(card->getSubcards().first())->isKindOf("TrickCard")) || card->hasFlag("Global_SlashAvailabilityChecker"))
                return 1000;
        }

        return 0;
    }
};

class Danji : public PhaseChangeSkill
{
public:
    Danji() : PhaseChangeSkill("danji")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *guanyu, Room *room) const
    {
        return PhaseChangeSkill::triggerable(guanyu) && guanyu->getMark(objectName()) == 0 && guanyu->getPhase() == Player::Start && guanyu->getHandcardNum() > guanyu->getHp() && !lordIsLiubei(room);
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
		room->sendCompulsoryTriggerLog(target, objectName());
        target->broadcastSkillInvoke(objectName());
        room->setPlayerMark(target, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(target) && target->getMark(objectName()) > 0)
            room->handleAcquireDetachSkills(target, "mashu|nuzhan");

        return false;
    }

private:
    static bool lordIsLiubei(const Room *room)
    {
        if (room->getLord() != NULL) {
            const ServerPlayer *const lord = room->getLord();
            if (lord->getGeneral() && lord->getGeneralName().contains("liubei"))
                return true;

            if (lord->getGeneral2() && lord->getGeneral2Name().contains("liubei"))
                return true;
        }

        return false;
    }
};

ShichouCard::ShichouCard()
{
}

bool ShichouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QStringList available_targets = Self->property("shichou_available_targets").toString().split("+");
    return targets.length() < Self->getLostHp() && available_targets.contains(to_select->objectName());
}

void ShichouCard::use(Room *, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
	foreach (ServerPlayer *p, targets)
	    p->setFlags("ShichouExtraTarget");
}

class ShichouViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ShichouViewAsSkill() : ZeroCardViewAsSkill("shichou")
    {
		response_pattern = "@@shichou";
    }

    virtual const Card *viewAs() const
    {
        return new ShichouCard;
    }
};

class Shichou : public TriggerSkill
{
public:
    Shichou() : TriggerSkill("shichou")
    {
        events << TargetChosen;
		view_as_skill = new ShichouViewAsSkill;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
		CardUseStruct use = data.value<CardUseStruct>();
		if (use.card->isKindOf("Slash") && !use.card->hasFlag("slashDisableExtraTarget") && player->isWounded()) {
			QStringList available_targets;
			bool no_distance_limit = false;
			if (use.card->hasFlag("slashNoDistanceLimit")){
				no_distance_limit = true;
				room->setPlayerFlag(player, "slashNoDistanceLimit");
			}
			foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (use.to.contains(p) || room->isProhibited(player, p, use.card)) continue;
                if (use.card->targetFilter(QList<const Player *>(), p, player))
                    available_targets << p->objectName();
            }
			if (no_distance_limit)
				room->setPlayerFlag(player, "-slashNoDistanceLimit");

			if (available_targets.isEmpty())
				return false;
			room->setPlayerProperty(player, "shichou_available_targets", available_targets.join("+"));
			player->tag["shichou-use"] = data;
			room->askForUseCard(player, "@@shichou", "@shichou-add:::" + QString::number(player->getLostHp()));
			player->tag.remove("shichou-use");
			room->setPlayerProperty(player, "shichou_available_targets", QString());
			foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasFlag("ShichouExtraTarget")) {
                    p->setFlags("-ShichouExtraTarget");
					use.to.append(p);
                }
            }
			room->sortByActionOrder(use.to);
		    data = QVariant::fromValue(use); 
		}
        return false;
    }
};

class Zhenlve : public ProhibitSkill
{
public:
    Zhenlve() : ProhibitSkill("zhenlve")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(this) && card->isKindOf("DelayedTrick");
    }
};

class ZhenlveTrick : public TriggerSkill
{
public:
    ZhenlveTrick() : TriggerSkill("#zhenlve-trick")
    {
        events << PreCardUsed << TrickCardCanceling;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *, QVariant &data) const
    {
		if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->isAlive() && use.from->hasSkill("zhenlve")) {
                if (use.card != NULL && use.card->isNDTrick())
                    use.card->setFlags("ZhenlveEffect");
            }
        } else if (triggerEvent == TrickCardCanceling) {
			if (data.value<CardEffectStruct>().card->hasFlag("ZhenlveEffect"))
				return true;
        }
        return false;
    }
};

JianshuCard::JianshuCard()
{
    will_throw = false;
	will_sort = false;
    handling_method = Card::MethodNone;
}

bool JianshuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return to_select != Self && (targets.isEmpty() || (targets.length() == 1 && to_select->inMyAttackRange(targets.first()) && !to_select->isKongcheng()));
}

bool JianshuCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void JianshuCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
	room->removePlayerMark(card_use.from, "@alienation");
	CardMoveReason reason(CardMoveReason::S_REASON_GIVE, card_use.from->objectName(), card_use.to.first()->objectName(), "jianshu", QString());
    room->obtainCard(card_use.to.first(), this, reason);
}

void JianshuCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
	ServerPlayer *from = targets.at(0);
    ServerPlayer *to = targets.at(1);
	if (!from->isKongcheng() && !to->isKongcheng()) {
        to->setFlags("JianshuPindianTarget");
		bool success = from->pindian(to, "jianshu", NULL);
        to->setFlags("-JianshuPindianTarget");
		if (from->hasFlag("JianshuSamePoint")){
			from->setFlags("-JianshuSamePoint");
			room->sortByActionOrder(targets);
			foreach (ServerPlayer *p, targets) {
				room->loseHp(p);
			}
		}else{
		    ServerPlayer *winner = NULL;
		    ServerPlayer *loser = NULL;
		    if (success) {
			    winner = from;
			    loser = to;
		    }else{
			    winner = to;
			    loser = from;
		    }
		    room->askForDiscard(winner, "jianshu", 2, 2, false, true);
			room->loseHp(loser);
		}
	}
}

class JianshuViewAsSkill : public OneCardViewAsSkill
{
public:
    JianshuViewAsSkill() : OneCardViewAsSkill("jianshu")
    {
        filter_pattern = ".|black|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@alienation") > 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        JianshuCard *skillcard = new JianshuCard;
        skillcard->addSubcard(originalCard);
        return skillcard;
    }
};

class Jianshu : public TriggerSkill
{
public:
    Jianshu() : TriggerSkill("jianshu")
    {
        events << Pindian;
        view_as_skill = new JianshuViewAsSkill;
		frequency = Limited;
        limit_mark = "@alienation";
    }

    int getPriority(TriggerEvent) const
    {
        return -1;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason != objectName() || pindian->from_number != pindian->to_number)
            return false;

        pindian->from->setFlags("JianshuSamePoint");

        return false;
    }
};

class Yongdi : public TriggerSkill
{
public:
    Yongdi() : TriggerSkill("yongdi")
    {
        events << Damaged;
		frequency = Limited;
        limit_mark = "@advocacy";
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
		if (player->getMark(limit_mark) <= 0) return false;
		QList<ServerPlayer *> males;
        foreach (ServerPlayer *player, room->getOtherPlayers(player)) {
            if (player->isMale())
                males << player;
        }
        if (males.isEmpty()) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, males, objectName(), "@yongdi", true, true);
		if (target) {
			room->removePlayerMark(player, limit_mark);
            player->broadcastSkillInvoke(objectName());

            LogMessage log;
            log.type = "#GainMaxHp";
            log.from = target;
            log.arg = "1";
            log.arg2 = QString::number(target->getMaxHp() + 1);
            room->sendLog(log);
            room->setPlayerProperty(target, "maxhp", target->getMaxHp() + 1);

			if (!target->isLord()) {
				QStringList skill_names;
                const General *general = target->getGeneral();
                foreach (const Skill *skill, general->getVisibleSkillList()) {
                    if (skill->isLordSkill())
                        skill_names << skill->objectName();
                }
				const General *general2 = target->getGeneral2();
				if (general2) {
					foreach (const Skill *skill, general2->getVisibleSkillList()) {
						if (skill->isLordSkill())
							skill_names << skill->objectName();
					}
				}
				if (!skill_names.isEmpty())
					room->handleAcquireDetachSkills(target, skill_names, true);
			}
		}
        return false;
    }
};

YuanhuCard::YuanhuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool YuanhuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (!targets.isEmpty())
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void YuanhuCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *caohong = effect.from;
    Room *room = caohong->getRoom();
    room->moveCardTo(this, caohong, effect.to, Player::PlaceEquip,
        CardMoveReason(CardMoveReason::S_REASON_PUT, caohong->objectName(), "yuanhu", QString()));

    const Card *card = Sanguosha->getCard(subcards.first());

    LogMessage log;
    log.type = "$PutEquip";
    log.from = effect.to;
    log.card_str = QString::number(card->getEffectiveId());
    room->sendLog(log);

    if (card->isKindOf("Weapon")) {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (effect.to->distanceTo(p) == 1 && caohong->canDiscard(p, "hej"))
                targets << p;
        }
        if (!targets.isEmpty()) {
            ServerPlayer *to_dismantle = room->askForPlayerChosen(caohong, targets, "yuanhu", "@yuanhu-discard:" + effect.to->objectName());
            int card_id = room->askForCardChosen(caohong, to_dismantle, "hej", "yuanhu", false, Card::MethodDiscard);
            room->throwCard(Sanguosha->getCard(card_id), to_dismantle, caohong);
        }
    } else if (card->isKindOf("Armor")) {
        effect.to->drawCards(1, "yuanhu");
    } else if (card->isKindOf("Horse")) {
        room->recover(effect.to, RecoverStruct(effect.from));
    }
}

class YuanhuViewAsSkill : public OneCardViewAsSkill
{
public:
    YuanhuViewAsSkill() : OneCardViewAsSkill("yuanhu")
    {
        filter_pattern = "EquipCard";
        response_pattern = "@@yuanhu";
    }

    const Card *viewAs(const Card *originalcard) const
    {
        YuanhuCard *first = new YuanhuCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Yuanhu : public PhaseChangeSkill
{
public:
    Yuanhu() : PhaseChangeSkill("yuanhu")
    {
        view_as_skill = new YuanhuViewAsSkill;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (target->getPhase() == Player::Finish && !target->isNude())
            room->askForUseCard(target, "@@yuanhu", "@yuanhu-equip", -1, Card::MethodNone);
        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        int index = -1;
		if (card->isKindOf("YuanhuCard")) {
			const Card *subcard = Sanguosha->getCard(card->getSubcards().first());
            if (subcard->isKindOf("Weapon"))
                index = 1;
            else if (subcard->isKindOf("Armor"))
                index = 2;
            else if (subcard->isKindOf("Horse"))
                index = 3;
		}
        return index;
    }
};

XuejiCard::XuejiCard()
{
}

bool XuejiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.length() >= Self->getLostHp())
        return false;

    if (to_select == Self)
        return false;

    int range_fix = 0;
    if (Self->getWeapon() && Self->getWeapon()->getEffectiveId() == getEffectiveId()) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        range_fix += weapon->getRange() - Self->getAttackRange(false);
    } else if (Self->getOffensiveHorse() && Self->getOffensiveHorse()->getEffectiveId() == getEffectiveId())
        range_fix += 1;

    return Self->inMyAttackRange(to_select, range_fix);
}

void XuejiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    DamageStruct damage;
    damage.from = source;
    damage.reason = "xueji";

    foreach (ServerPlayer *p, targets) {
        damage.to = p;
        room->damage(damage);
    }
    foreach (ServerPlayer *p, targets) {
        if (p->isAlive())
            p->drawCards(1, "xueji");
    }
}

class Xueji : public OneCardViewAsSkill
{
public:
    Xueji() : OneCardViewAsSkill("xueji")
    {
        filter_pattern = ".|red!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getLostHp() > 0 && !player->hasUsed("XuejiCard");
    }

    const Card *viewAs(const Card *originalcard) const
    {
        XuejiCard *first = new XuejiCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Huxiao : public TriggerSkill
{
public:
    Huxiao() : TriggerSkill("huxiao")
    {
        events << SlashMissed << EventPhaseChanging;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == SlashMissed && TriggerSkill::triggerable(player)) {
			SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (player->getPhase() == Player::Play){
				room->sendCompulsoryTriggerLog(player, "huxiao");
                player->broadcastSkillInvoke("huxiao");
			    if (effect.to->isAlive())
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), effect.to->objectName());
                room->addPlayerMark(player, "huxiao");
			}
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                if (player->getMark("huxiao") > 0)
                    room->setPlayerMark(player, "huxiao", 0);
        }

        return false;
    }
};

class HuxiaoTargetMod : public TargetModSkill
{
public:
    HuxiaoTargetMod() : TargetModSkill("#huxiao-target")
    {
    }

    int getResidueNum(const Player *from, const Card *) const
    {
        return from->getMark(objectName());
    }
};

class WujiCount : public TriggerSkill
{
public:
    WujiCount() : TriggerSkill("#wuji-count")
    {
        events << PreDamageDone << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.from == room->getCurrent() && damage.from->getMark("wuji") == 0)
                room->addPlayerMark(damage.from, "wuji_damage", damage.damage);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                if (player->getMark("wuji_damage") > 0)
                    room->setPlayerMark(player, "wuji_damage", 0);
        }

        return false;
    }
};

class Wuji : public PhaseChangeSkill
{
public:
    Wuji() : PhaseChangeSkill("wuji")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Finish
            && target->getMark("wuji") == 0
            && target->getMark("wuji_damage") >= 3;
    }

    bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->sendCompulsoryTriggerLog(player, objectName());

        player->broadcastSkillInvoke(objectName());
        //room->doLightbox("$WujiAnimate", 4000);

        room->setPlayerMark(player, "wuji", 1);
        if (room->changeMaxHpForAwakenSkill(player, 1)) {
            room->recover(player, RecoverStruct(player));
            if (player->getMark("wuji") == 1)
                room->detachSkillFromPlayer(player, "huxiao");
        }

        return false;
    }
};

class Moukui : public TriggerSkill
{
public:
    Moukui() : TriggerSkill("moukui")
    {
        events << TargetSpecified << SlashMissed << CardFinished;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            foreach (ServerPlayer *p, use.to) {
                if (player->askForSkillInvoke(this, QVariant::fromValue(p))) {
                    player->broadcastSkillInvoke(objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
                    QString choice;
                    if (!player->canDiscard(p, "he"))
                        choice = "draw";
                    else
                        choice = room->askForChoice(player, objectName(), "draw+discard", QVariant::fromValue(p));
                    if (choice == "draw") {
                        player->drawCards(1, objectName());
                    } else {
                        room->setTag("MoukuiDiscard", data);
                        int disc = room->askForCardChosen(player, p, "he", objectName(), false, Card::MethodDiscard);
                        room->removeTag("MoukuiDiscard");
                        room->throwCard(disc, p, player);
                    }
                    room->addPlayerMark(p, objectName() + use.card->toString());
                }
            }
        } else if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.to->isDead() || effect.to->getMark(objectName() + effect.slash->toString()) <= 0)
                return false;
            if (!effect.from->isAlive() || !effect.to->isAlive() || !effect.to->canDiscard(effect.from, "he"))
                return false;
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, effect.to->objectName(), effect.from->objectName());
            int disc = room->askForCardChosen(effect.to, effect.from, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(disc, effect.from, effect.to);
            room->removePlayerMark(effect.to, objectName() + effect.slash->toString());
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            foreach(ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, objectName() + use.card->toString(), 0);
        }

        return false;
    }
};


class TianmingViewAsSkill : public ViewAsSkill
{
public:
    TianmingViewAsSkill() : ViewAsSkill("tianming")
    {
        response_pattern = "@@tianming";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return !Self->isJilei(to_select) && selected.length() < Self->getMark("tianming_count");
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != Self->getMark("tianming_count"))
            return NULL;

        DummyCard *xt = new DummyCard;
        xt->addSubcards(cards);
        return xt;
    }
};

class Tianming : public TriggerSkill
{
public:
    Tianming() : TriggerSkill("tianming")
    {
        events << TargetConfirmed;
		view_as_skill = new TianmingViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")){
			foreach (ServerPlayer *to, use.to) {
				if (to == player) {
			        int i = qMin(2, player->getCardCount());
                    bool invoke = false;
					if (i == 0) {
						if (room->askForSkillInvoke(player, objectName(), "prompt")) {
							invoke = true;
                            player->broadcastSkillInvoke(objectName());
						}
					} else {
						room->setPlayerMark(player, "tianming_count", i);
						invoke = room->askForCard(player, "@@tianming", "@tianming-discard:::" + QString::number(i), data, objectName());
						room->setPlayerMark(player, "tianming_count", 0);
					}
			        if (invoke){
						player->drawCards(2);
                        int max = -1000;
                        foreach (ServerPlayer *p, room->getAllPlayers()) {
                            if (p->getHp() > max)
                                max = p->getHp();
                        }
                        if (player->getHp() == max)
                            return false;

                        QList<ServerPlayer *> maxs;
                        foreach (ServerPlayer *p, room->getAllPlayers()) {
                            if (p->getHp() == max)
                                maxs << p;
                            if (maxs.size() > 1)
                                return false;
                        }
                        ServerPlayer *mosthp = maxs.first();
		        		int j = qMin(2, mosthp->getCardCount());
		        		if ((j == 0 && room->askForSkillInvoke(mosthp, "tianming_draw", "prompt")) || (j > 0 && 
		        		    room->askForDiscard(mosthp, objectName(), j, j, true, true, "@tianming-discard:::" + QString::number(j))))
			        		mosthp->drawCards(2, objectName());
                    }
				}
			}
        }
        return false;
    }
};

MizhaoCard::MizhaoCard()
{
}

bool MizhaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void MizhaoCard::onEffect(const CardEffectStruct &effect) const
{
    DummyCard *handcards = effect.from->wholeHandCards();
    handcards->deleteLater();
    CardMoveReason r(CardMoveReason::S_REASON_GIVE, effect.from->objectName());
    Room *room = effect.from->getRoom();
    room->obtainCard(effect.to, handcards, r, false);
    if (effect.to->isKongcheng()) return;

    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
        if (!p->isKongcheng() && p != effect.from)
            targets << p;
    }

    if (!targets.isEmpty()) {
        ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "mizhao", "@mizhao-pindian:" + effect.to->objectName());
        target->setFlags("MizhaoPindianTarget");
        bool success = effect.to->pindian(target, "mizhao", NULL);
		if (effect.to->hasFlag("MizhaoSamePoint")){
			effect.to->setFlags("-MizhaoSamePoint");
		}else{
		    ServerPlayer *winner = NULL;
		    ServerPlayer *loser = NULL;
		    if (success) {
			    winner = effect.to;
			    loser = target;
		    }else{
			    winner = target;
			    loser = effect.to;
		    }
		    if (winner->canSlash(loser, NULL, false)) {
                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName("_mizhao");
                room->useCard(CardUseStruct(slash, winner, loser));
            }
		}
        target->setFlags("-MizhaoPindianTarget");
    }
}

class MizhaoViewAsSkill : public ZeroCardViewAsSkill
{
public:
    MizhaoViewAsSkill() : ZeroCardViewAsSkill("mizhao")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("MizhaoCard");
    }

    const Card *viewAs() const
    {
        return new MizhaoCard;
    }
};

class Mizhao : public TriggerSkill
{
public:
    Mizhao() : TriggerSkill("mizhao")
    {
        events << Pindian;
        view_as_skill = new MizhaoViewAsSkill;
    }

    int getPriority(TriggerEvent) const
    {
        return -1;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason != objectName() || pindian->from_number != pindian->to_number)
            return false;

        pindian->from->setFlags("MizhaoSamePoint");

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return 0;
        else
            return -1;
    }
};

class Jieyuan : public TriggerSkill
{
public:
    Jieyuan() : TriggerSkill("jieyuan")
    {
        events << DamageCaused << DamageInflicted;
		view_as_skill = new dummyVS;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageCaused) {
            if (damage.to && damage.to->isAlive()
                && damage.to->getHp() >= player->getHp() && damage.to != player && player->canDiscard(player, "h")
                && room->askForCard(player, ".black", "@jieyuan-increase:" + damage.to->objectName(), data, objectName())) {

                LogMessage log;
                log.type = "#JieyuanIncrease";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(++damage.damage);
                room->sendLog(log);

                data = QVariant::fromValue(damage);
            }
        } else if (triggerEvent == DamageInflicted) {
            if (damage.from && damage.from->isAlive()
                && damage.from->getHp() >= player->getHp() && damage.from != player && player->canDiscard(player, "h")
                && room->askForCard(player, ".red", "@jieyuan-decrease:" + damage.from->objectName(), data, objectName())) {

                LogMessage log;
                log.type = "#JieyuanDecrease";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(--damage.damage);
                room->sendLog(log);

                data = QVariant::fromValue(damage);
                if (damage.damage < 1)
                    return true;
            }
        }

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const QString &prompt) const
    {
        if (prompt.startsWith("@jieyuan-increase"))
			return 1;
		else if (prompt.startsWith("@jieyuan-decrease"))
			return 2;
		return -1;
    }
};

class Fenxin : public TriggerSkill
{
public:
    Fenxin() : TriggerSkill("fenxin")
    {
        events << BeforeGameOverJudge;
        frequency = Limited;
        limit_mark = "@burnheart";
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (!isNormalGameMode(room->getMode()))
            return false;
        DeathStruct death = data.value<DeathStruct>();
        if (death.damage == NULL)
            return false;
        ServerPlayer *killer = death.damage->from;
        if (killer == NULL || killer->isLord() || player->isLord() || player->getHp() > 0)
            return false;
        if (!TriggerSkill::triggerable(killer) || killer->getMark("@burnheart") == 0)
            return false;
        player->setFlags("FenxinTarget");
        bool invoke = room->askForSkillInvoke(killer, objectName(), QVariant::fromValue(player));
        player->setFlags("-FenxinTarget");
        if (invoke) {
            killer->broadcastSkillInvoke(objectName());
            //room->doLightbox("$FenxinAnimate");
            room->removePlayerMark(killer, "@burnheart");
            QString role1 = killer->getRole();
            killer->setRole(player->getRole());
            room->notifyProperty(killer, killer, "role", player->getRole());
            room->setPlayerProperty(player, "role", role1);
        }
        return false;
    }
};

class Baobian : public TriggerSkill
{
public:
    Baobian() : TriggerSkill("baobian")
    {
        events << GameStart << HpChanged << MaxHpChanged << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == objectName()) {
                QStringList baobian_skills = player->tag["BaobianSkills"].toStringList();
                QStringList detachList;
                foreach(QString skill_name, baobian_skills)
                    detachList.append("-" + skill_name);
                room->handleAcquireDetachSkills(player, detachList);
                player->tag["BaobianSkills"] = QVariant();
            }
            return false;
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName()) return false;
        }

        if (!player->isAlive() || !player->hasSkill(this, true)) return false;

        acquired_skills.clear();
        detached_skills.clear();
        BaobianChange(room, player, 1, "shensu");
        BaobianChange(room, player, 2, "paoxiao");
        BaobianChange(room, player, 3, "tiaoxin");
        if (!acquired_skills.isEmpty() || !detached_skills.isEmpty())
            room->handleAcquireDetachSkills(player, acquired_skills + detached_skills);
        return false;
    }

private:
    void BaobianChange(Room *room, ServerPlayer *player, int hp, const QString &skill_name) const
    {
        QStringList baobian_skills = player->tag["BaobianSkills"].toStringList();
        if (player->getHp() <= hp) {
            if (!baobian_skills.contains(skill_name)) {
                room->notifySkillInvoked(player, "baobian");
                acquired_skills.append(skill_name);
                baobian_skills << skill_name;
            }
        } else {
            if (baobian_skills.contains(skill_name)) {
                detached_skills.append("-" + skill_name);
                baobian_skills.removeOne(skill_name);
            }
        }
        player->tag["BaobianSkills"] = QVariant::fromValue(baobian_skills);
    }

    mutable QStringList acquired_skills, detached_skills;
};

class Xiuluo : public PhaseChangeSkill
{
public:
    Xiuluo() : PhaseChangeSkill("xiuluo")
    {
		view_as_skill = new dummyVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->canDiscard(target, "h")
            && hasDelayedTrick(target);
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        while (hasDelayedTrick(target) && target->canDiscard(target, "h")) {
            QStringList suits;
            foreach (const Card *jcard, target->getJudgingArea()) {
                if (!suits.contains(jcard->getSuitString()))
                    suits << jcard->getSuitString();
            }

            const Card *card = room->askForCard(target, QString(".|%1|.|hand").arg(suits.join(",")),
                "@xiuluo", QVariant(), objectName());
            if (!card || !hasDelayedTrick(target)) break;
            target->broadcastSkillInvoke(objectName());

            QList<int> avail_list, other_list;
            foreach (const Card *jcard, target->getJudgingArea()) {
                if (jcard->isKindOf("SkillCard")) continue;
                if (jcard->getSuit() == card->getSuit())
                    avail_list << jcard->getEffectiveId();
                else
                    other_list << jcard->getEffectiveId();
            }
            room->fillAG(avail_list + other_list, NULL, other_list);
            int id = room->askForAG(target, avail_list, false, objectName());
            room->clearAG();
            room->throwCard(id, NULL);
        }

        return false;
    }

private:
    static bool hasDelayedTrick(const ServerPlayer *target)
    {
        foreach(const Card *card, target->getJudgingArea())
            if (!card->isKindOf("SkillCard")) return true;
        return false;
    }
};

class Shenwei : public DrawCardsSkill
{
public:
    Shenwei() : DrawCardsSkill("#shenwei-draw")
    {
        frequency = Compulsory;
    }

    int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();

        player->broadcastSkillInvoke("shenwei");
        room->sendCompulsoryTriggerLog(player, "shenwei");

        return n + 2;
    }
};

class ShenweiKeep : public MaxCardsSkill
{
public:
    ShenweiKeep() : MaxCardsSkill("shenwei")
    {
    }

    int getExtra(const Player *target) const
    {
        if (target->hasSkill(this))
            return 2;
        else
            return 0;
    }
};

class Shenji : public TargetModSkill
{
public:
    Shenji() : TargetModSkill("shenji")
    {
    }

    int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasSkill(this) && from->getWeapon() == NULL)
            return 2;
        else
            return 0;
    }
};

class ChenqingViewAsSkill : public ViewAsSkill
{
public:
    ChenqingViewAsSkill() : ViewAsSkill("chenqing")
    {
        response_pattern = "@@chenqing!";
    }

    
    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 4 && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 4) {
            DummyCard *xt = new DummyCard;
            xt->addSubcards(cards);
            return xt;
        }

        return NULL;
    }
};

class Chenqing : public TriggerSkill
{
public:
    Chenqing() : TriggerSkill("chenqing")
    {
        events << Dying;
		view_as_skill = new ChenqingViewAsSkill;
	}

    bool trigger(TriggerEvent, Room *room, ServerPlayer *caiwenji, QVariant &data) const
    {
		DyingStruct dying_data = data.value<DyingStruct>();
		QList<ServerPlayer *> targets = room->getOtherPlayers(caiwenji);
		targets.removeOne(dying_data.who);
		if (targets.isEmpty() || caiwenji->getMark("chenqing_times") > 0)
			return false;
        ServerPlayer *target = room->askForPlayerChosen(caiwenji, targets, objectName(), "@chenqing-save:" + dying_data.who->objectName(), true, true);
		if (target) {
            caiwenji->broadcastSkillInvoke(objectName());
			room->addPlayerMark(caiwenji, "chenqing_times");
			target->drawCards(4);
            QList<int> all_cards = target->forceToDiscard(10086, true);
            QList<int> to_discard = target->forceToDiscard(4, true);
			if (all_cards.length() > 4){
			    const Card *card = room->askForCard(target, "@@chenqing!", "@chenqing-discard:" + caiwenji->objectName() + ":" + dying_data.who->objectName(), data, Card::MethodNone);
			    if (card != NULL && card->subcardsLength() == 4) {
					to_discard = card->getSubcards();
				}
			}
			DummyCard *dummy_card = new DummyCard(to_discard);
            dummy_card->deleteLater();
            CardMoveReason mreason(CardMoveReason::S_REASON_THROW, target->objectName(), QString(), objectName(), QString());
            room->throwCard(dummy_card, mreason, target);
			if (to_discard.length() != 4)
			    return false;
			QStringList suitlist;
            foreach(int card_id, to_discard){
				const Card *card = Sanguosha->getCard(card_id);
                QString suit = card->getSuitString();
                if (!suitlist.contains(suit))
                    suitlist << suit;
                else{
					return false;
				}
            }
			Peach *peach = new Peach(Card::NoSuit, 0);
			peach->setSkillName(QString("_%1").arg(objectName()));
            if (!target->isCardLimited(peach, Card::MethodUse) && !target->isProhibited(dying_data.who, peach) && !(room->getCurrent()->hasSkill("wansha") && target != room->getCurrent()))
                room->useCard(CardUseStruct(peach, target, dying_data.who));
            else
                delete peach;
		}
		return false;
    }

};

class ChenqingClear : public TriggerSkill
{
public:
    ChenqingClear() : TriggerSkill("#chenqing-clear")
    {
        events << TurnStart;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
		if (player->isLord() && !room->getTag("ExtraTurn").toBool()){
			foreach (ServerPlayer *p, room->getAlivePlayers()) {
                room->setPlayerMark(p, "chenqing_times", 0);
            }
		}
		return false;
    }
};

class MozhiViewAsSkill : public OneCardViewAsSkill
{
public:
    MozhiViewAsSkill() : OneCardViewAsSkill("mozhi")
    {
        response_pattern = "@@mozhi";
		response_or_use = true;
    }

	bool viewFilter(const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;
        QString mozhi_card = Self->property("mozhi").toString();
        if (mozhi_card.isEmpty()) return false;
        Card *use_card = Sanguosha->cloneCard(mozhi_card);
        use_card->addSubcard(to_select);
        return use_card->isAvailable(Self);
    }

    const Card *viewAs(const Card *originalCard) const
    {
        QString mozhi_card = Self->property("mozhi").toString();
        if (mozhi_card.isEmpty()) return NULL;
        Card *use_card = Sanguosha->cloneCard(mozhi_card);
        use_card->setCanRecast(false);
        use_card->addSubcard(originalCard);
		use_card->setSkillName("mozhi");
        return use_card;
     }
};

class Mozhi : public PhaseChangeSkill
{
public:
    Mozhi() : PhaseChangeSkill("mozhi")
    {
        events << EventPhaseStart;
		view_as_skill = new MozhiViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPhase() == Player::Finish) {
			QStringList mozhi_list = player->tag[objectName()].toStringList();
            int times = 0;
			foreach (QString mozhi_card, mozhi_list) {
			    if (times >= 2)
					break;
				if (player->isKongcheng() && player->getHandPile().isEmpty())
				    break;
				Card *use_card = Sanguosha->cloneCard(mozhi_card);
				if (!use_card || (!use_card->isKindOf("BasicCard") && !use_card->isNDTrick()) || !use_card->isAvailable(player))
					break;
			    room->setPlayerProperty(player, "mozhi", mozhi_card);
			    if (!room->askForUseCard(player, "@@mozhi", "@mozhi-use:::" + mozhi_card))
					break;
				++times;
			}
		}
		return false;
    }
};

class MozhiRecord : public TriggerSkill
{
public:
    MozhiRecord() : TriggerSkill("#mozhi-record")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded;
        global = true;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive)
                player->tag.remove("mozhi");
        } else {
            if (player->getPhase() != Player::Play)
                return false;
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                if (response.m_isUse)
                    card = response.m_card;
            }
            if (card && card->getHandlingMethod() == Card::MethodUse && player->getPhase() == Player::Play) {
				if (card->isKindOf("BasicCard") || card->isNDTrick()) {
					QStringList list = player->tag["mozhi"].toStringList();
                    list.append(card->objectName());
                    player->tag["mozhi"] = list;
				}
			}
        }
        return false;
    }
};

class Xingwu : public TriggerSkill
{
public:
    Xingwu() : TriggerSkill("xingwu")
    {
        events << PreCardUsed << CardResponded << EventPhaseStart << CardsMoveOneTime;
		view_as_skill = new dummyVS;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                if (response.m_isUse)
                    card = response.m_card;
            }
            if (card && card->getTypeId() != Card::TypeSkill && card->getHandlingMethod() == Card::MethodUse) {
                int n = player->getMark(objectName());
                if (card->isBlack())
                    n |= 1;
                else if (card->isRed())
                    n |= 2;
                player->setMark(objectName(), n);
            }
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Discard) {
                int n = player->getMark(objectName());
                bool red_avail = ((n & 2) == 0), black_avail = ((n & 1) == 0);
                if (player->isKongcheng() || (!red_avail && !black_avail))
                    return false;
                QString pattern = ".|.|.|hand";
                if (red_avail != black_avail)
                    pattern = QString(".|%1|.|hand").arg(red_avail ? "red" : "black");
                const Card *card = room->askForCard(player, pattern, "@xingwu", QVariant(), Card::MethodNone);
                if (card) {
                    room->notifySkillInvoked(player, objectName());
                    player->broadcastSkillInvoke(objectName());

                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);

                    player->addToPile(objectName(), card);
                }
            } else if (player->getPhase() == Player::RoundStart) {
                player->setMark(objectName(), 0);
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && player->getPile(objectName()).length() >= 3) {
                player->clearOnePrivatePile(objectName());
                QList<ServerPlayer *> males;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->isMale())
                        males << p;
                }
                if (males.isEmpty()) return false;

                ServerPlayer *target = room->askForPlayerChosen(player, males, objectName(), "@xingwu-choose");
                player->broadcastSkillInvoke(objectName());
                room->damage(DamageStruct(objectName(), player, target, 2));

                if (!player->isAlive()) return false;
                QList<const Card *> equips = target->getEquips();
                if (!equips.isEmpty()) {
                    DummyCard *dummy = new DummyCard;
                    dummy->deleteLater();
                    foreach (const Card *equip, equips) {
                        if (player->canDiscard(target, equip->getEffectiveId()))
                            dummy->addSubcard(equip);
                    }
                    if (dummy->subcardsLength() > 0)
                        room->throwCard(dummy, target, player);
                }
            }
        }
        return false;
    }
};

class Luoyan : public TriggerSkill
{
public:
    Luoyan() : TriggerSkill("luoyan")
    {
        events << CardsMoveOneTime << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill && data.toString() == objectName()) {
            room->handleAcquireDetachSkills(player, "-tianxiang|-liuli", true);
        } else if (triggerEvent == EventAcquireSkill && data.toString() == objectName()) {
            if (!player->getPile("xingwu").isEmpty()) {
                room->notifySkillInvoked(player, objectName());
                room->handleAcquireDetachSkills(player, "tianxiang|liuli");
            }
        } else if (triggerEvent == CardsMoveOneTime && player->isAlive() && player->hasSkill(this, true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && move.to_pile_name == "xingwu") {
                if (player->getPile("xingwu").length() == 1) {
                    room->notifySkillInvoked(player, objectName());
                    room->handleAcquireDetachSkills(player, "tianxiang|liuli");
                }
            } else if (move.from == player && move.from_places.contains(Player::PlaceSpecial)
                && move.from_pile_names.contains("xingwu")) {
                if (player->getPile("xingwu").isEmpty())
                    room->handleAcquireDetachSkills(player, "-tianxiang|-liuli", true);
            }
        }
        return false;
    }
};

class Xiaoguo : public TriggerSkill
{
public:
    Xiaoguo() : TriggerSkill("xiaoguo")
    {
        events << EventPhaseStart;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Finish)
            return false;
        ServerPlayer *yuejin = room->findPlayerBySkillName(objectName());
        if (!yuejin || yuejin == player)
            return false;
        if (yuejin->canDiscard(yuejin, "h") && room->askForCard(yuejin, ".Basic", "@xiaoguo", QVariant(), objectName(), player)) {
            if (!room->askForCard(player, ".Equip", "@xiaoguo-discard", QVariant()))
                room->damage(DamageStruct("xiaoguo", yuejin, player));
            else {
                if (yuejin->isAlive())
                    yuejin->drawCards(1, objectName());
            }
        }
        return false;
    }
};

class Kangkai : public TriggerSkill
{
public:
    Kangkai() : TriggerSkill("kangkai")
    {
        events << TargetConfirmed;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            foreach (ServerPlayer *to, use.to) {
                if (!player->isAlive()) break;
                if (player->distanceTo(to) <= 1 && TriggerSkill::triggerable(player) && to->isAlive()) {
                    player->tag["KangkaiSlash"] = data;
                    bool will_use = room->askForSkillInvoke(player, objectName(), QVariant::fromValue(to));
                    player->tag.remove("KangkaiSlash");
                    if (!will_use) continue;

                    player->broadcastSkillInvoke(objectName());
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), to->objectName());

                    player->drawCards(1, "kangkai");
                    if (!player->isNude() && player != to) {
                        const Card *card = NULL;
                        if (player->getCardCount() > 1) {
                            card = room->askForCard(player, "..!", "@kangkai-give:" + to->objectName(), data, Card::MethodNone);
                            if (!card)
                                card = player->getCards("he").at(qrand() % player->getCardCount());
                        } else {
                            Q_ASSERT(player->getCardCount() == 1);
                            card = player->getCards("he").first();
                        }
                        CardMoveReason r(CardMoveReason::S_REASON_GIVE, player->objectName(), objectName(), QString());
                        room->obtainCard(to, card, r);
                        if (card->getTypeId() == Card::TypeEquip && room->getCardOwner(card->getEffectiveId()) == to && !to->isLocked(card)) {
                            to->tag["KangkaiSlash"] = data;
                            to->tag["KangkaiGivenCard"] = QVariant::fromValue(card);
                            bool will_use = room->askForSkillInvoke(to, "kangkai_use", "use");
                            to->tag.remove("KangkaiSlash");
                            to->tag.remove("KangkaiGivenCard");
                            if (will_use)
                                room->useCard(CardUseStruct(card, to, to));
                        }
                    }
                }
            }
        }
        return false;
    }
};

class Shenxian : public TriggerSkill
{
public:
    Shenxian() : TriggerSkill("shenxian")
    {
        events << CardsMoveOneTime << EventPhaseStart;
        frequency = Frequent;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark(objectName()) > 0)
                        p->setMark(objectName(), 0);
                }
            }
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::NotActive && player->getMark(objectName()) == 0 && move.from && move.from->isAlive()
                && move.from->objectName() != player->objectName() && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->getTypeId() == Card::TypeBasic) {
                        if (room->askForSkillInvoke(player, objectName(), data)) {
                            player->setMark(objectName(), 1);
                            player->broadcastSkillInvoke(objectName());
                            player->drawCards(1, objectName());
                        }
                        break;
                    }
                }
            }
        }
        return false;
    }
};

QiangwuCard::QiangwuCard()
{
    target_fixed = true;
}

void QiangwuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    JudgeStruct judge;
    judge.pattern = ".";
    judge.who = source;
    judge.reason = "qiangwu";
    room->judge(judge);

    bool ok = false;
    int num = judge.pattern.toInt(&ok);
    if (ok)
        room->setPlayerMark(source, "qiangwu", num);
}

class QiangwuViewAsSkill : public ZeroCardViewAsSkill
{
public:
    QiangwuViewAsSkill() : ZeroCardViewAsSkill("qiangwu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QiangwuCard");
    }

    const Card *viewAs() const
    {
        return new QiangwuCard;
    }
};

class Qiangwu : public TriggerSkill
{
public:
    Qiangwu() : TriggerSkill("qiangwu")
    {
        events << EventPhaseChanging << PreCardUsed << FinishJudge;
        view_as_skill = new QiangwuViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, "qiangwu", 0);
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == "qiangwu")
                judge->pattern = QString::number(judge->card->getNumber());
        } else if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && player->getMark("qiangwu") > 0
                && use.card->getNumber() > player->getMark("qiangwu")) {
                if (use.m_addHistory) {
                    room->addPlayerHistory(player, use.card->getClassName(), -1);
                    use.m_addHistory = false;
                    data = QVariant::fromValue(use);
                }
            }
        }
        return false;
    }
};

class QiangwuTargetMod : public TargetModSkill
{
public:
    QiangwuTargetMod() : TargetModSkill("#qiangwu-target")
    {
    }

    int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (card->getNumber() < from->getMark("qiangwu"))
            return 1000;
        else
            return 0;
    }

    int getResidueNum(const Player *from, const Card *card) const
    {
        if (from->getMark("qiangwu") > 0
            && (card->getNumber() > from->getMark("qiangwu") || card->hasFlag("Global_SlashAvailabilityChecker")))
            return 1000;
        else
            return 0;
    }
};

class Kuangfu : public TriggerSkill
{
public:
    Kuangfu() : TriggerSkill("kuangfu")
    {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *panfeng, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (damage.card && damage.card->isKindOf("Slash") && target->hasEquip()
            && !target->hasFlag("Global_DebutFlag") && !damage.chain && !damage.transfer) {
            QStringList equiplist;
            for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
                if (!target->getEquip(i)) continue;
                if (panfeng->canDiscard(target, target->getEquip(i)->getEffectiveId()) || panfeng->getEquip(i) == NULL)
                    equiplist << QString::number(i);
            }
            if (equiplist.isEmpty() || !panfeng->askForSkillInvoke(this, QVariant::fromValue(target)))
                return false;
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, panfeng->objectName(), target->objectName());
            panfeng->broadcastSkillInvoke(objectName());
            int equip_index = room->askForChoice(panfeng, "kuangfu_equip", equiplist.join("+"), QVariant::fromValue(target)).toInt();
            const Card *card = target->getEquip(equip_index);
            int card_id = card->getEffectiveId();

            QStringList choicelist;
            if (panfeng->canDiscard(target, card_id))
                choicelist << "throw";
            if (equip_index > -1 && panfeng->getEquip(equip_index) == NULL)
                choicelist << "move";

            QString choice = room->askForChoice(panfeng, "kuangfu", choicelist.join("+"));

            if (choice == "move") {
                room->moveCardTo(card, panfeng, Player::PlaceEquip);
            } else {
                room->throwCard(card, target, panfeng);
            }
        }

        return false;
    }
};

class Meibu : public TriggerSkill
{
public:
    Meibu() : TriggerSkill("meibu")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *sunluyu, room->getOtherPlayers(player)) {
                if (!player->inMyAttackRange(sunluyu) && TriggerSkill::triggerable(sunluyu)
                    && room->askForSkillInvoke(sunluyu, objectName(), QVariant::fromValue(player))) {
                    sunluyu->broadcastSkillInvoke(objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, sunluyu->objectName(), player->objectName());
                    room->acquireSkill(player, "zhixi");
					if (sunluyu->getMark("mumu") == 0) {
                        QVariantList sunluyus = player->tag[objectName()].toList();
                        sunluyus << QVariant::fromValue(sunluyu);
                        player->tag[objectName()] = QVariant::fromValue(sunluyus);
                        room->insertAttackRangePair(player, sunluyu);
					}
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;

            QVariantList sunluyus = player->tag[objectName()].toList();
            foreach (QVariant sunluyu, sunluyus) {
                ServerPlayer *s = sunluyu.value<ServerPlayer *>();
                room->removeAttackRangePair(player, s);
            }
            room->detachSkillFromPlayer(player, "zhixi");
        }
        return false;
    }
};

class ZhixiFilter : public FilterSkill
{
public:
    ZhixiFilter() : FilterSkill("#zhixi-filter")
    {
	}

    bool viewFilter(const Card *to_select) const
    {
        return to_select->getTypeId() == Card::TypeTrick;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName("zhixi");
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

MumuCard::MumuCard()
{
    target_fixed = true;
}

void MumuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
	QList<ServerPlayer *> targets1;
	QList<ServerPlayer *> targets2;
    foreach (ServerPlayer *p, room->getAllPlayers()) {
	    if (source->canDiscard(p, "e"))
			targets1 << p;
	    if (p->getArmor())
			targets2 << p;
	}
	QStringList choices;
	if (!targets1.isEmpty())
		choices << "discard_equip";
	if (!targets2.isEmpty())
		choices << "obtain_armor";
	
	if (!choices.isEmpty()){
		QList<ServerPlayer *> targets = targets1;
        QString prompt = "@mumu-equip";
        QString choice = room->askForChoice(source, "mumu", choices.join("+"));
	    if (choice == "obtain_armor"){
		    targets = targets2;
            prompt = "@mumu-armor";
	    }
        source->tag["MumuChoice"] = choice;//for AI
	    ServerPlayer *target = room->askForPlayerChosen(source, targets, "mumu", prompt);
		source->tag.remove("MumuChoice");//for AI
	    if (choice == "obtain_armor"){
	        WrappedCard *armor = target->getArmor();
            if (armor){
				CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, source->objectName());
                room->obtainCard(source, armor, reason, true);
			}
		} else {
		    if (source->canDiscard(target, "e"))
				room->throwCard(room->askForCardChosen(source, target, "e", "mumu", false, Card::MethodDiscard), target, source);
			source->drawCards(1);
		}
	}
	int used_id = subcards.first();
    const Card *c = Sanguosha->getCard(used_id);
    if (c->isKindOf("Slash") || (c->isBlack() && c->isKindOf("TrickCard"))) {
        source->addMark("mumu");
        QString translation = Sanguosha->translate(":meibu");
        QString in_attack = Sanguosha->translate("meibu_in_attack");
        if (translation.endsWith(in_attack)) {
            translation.remove(in_attack);
            Sanguosha->addTranslationEntry(":meibu", translation.toStdString().c_str());
            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }
    }
}

class MumuVS : public OneCardViewAsSkill
{
public:
    MumuVS() : OneCardViewAsSkill("mumu")
    {
        filter_pattern = ".!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasUsed("MumuCard"))
			return false;
		if (player->hasEquip())
			return true;
		foreach(const Player *p, player->getAliveSiblings())
			if (p->hasEquip())
				return true;
	    return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        MumuCard *mm = new MumuCard;
        mm->addSubcard(originalCard);
        return mm;
    }
};

class Mumu : public PhaseChangeSkill
{
public:
    Mumu() : PhaseChangeSkill("mumu")
    {
        view_as_skill = new MumuVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPhase() == Player::RoundStart;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getMark("mumu") > 0) {
            target->setMark("mumu", 0);
            QString translation = Sanguosha->translate(":meibu");
            QString in_attack = Sanguosha->translate("meibu_in_attack");
            if (!translation.endsWith(in_attack)) {
                translation.append(in_attack);
                Sanguosha->addTranslationEntry(":meibu", translation.toStdString().c_str());
                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                target->getRoom()->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
            }
        }
        return false;
    }
};

class Zhixi : public TriggerSkill
{
public:
    Zhixi() : TriggerSkill("zhixi")
    {
        events << CardUsed << EventLoseSkill;
		frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == CardUsed)
            return 6;

        return TriggerSkill::getPriority(triggerEvent);
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("TrickCard") && TriggerSkill::triggerable(player) && player->getMark("#zhixi") < 1) {
                room->sendCompulsoryTriggerLog(player, objectName());
                room->addPlayerTip(player, "#zhixi");
				if (!player->hasSkill("#zhixi-filter", true)) {
                    room->acquireSkill(player, "#zhixi-filter", false);
                    room->filterCards(player, player->getCards("he"), true);
                }
            }
        } else if (triggerEvent == EventLoseSkill) {
            if (data.toString() == objectName()) {
                room->removePlayerTip(player, "#zhixi");
				if (player->hasSkill("#zhixi-filter", true)) {
                    room->detachSkillFromPlayer(player, "#zhixi-filter");
                    room->filterCards(player, player->getCards("he"), true);
                }
            }
        }
        return false;
    }
};

YinbingCard::YinbingCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void YinbingCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->addToPile("yinbing", this);
}

class YinbingViewAsSkill : public ViewAsSkill
{
public:
    YinbingViewAsSkill() : ViewAsSkill("yinbing")
    {
        response_pattern = "@@yinbing";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->getTypeId() != Card::TypeBasic;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0) return NULL;

        Card *acard = new YinbingCard;
        acard->addSubcards(cards);
        acard->setSkillName(objectName());
        return acard;
    }
};

class Yinbing : public TriggerSkill
{
public:
    Yinbing() : TriggerSkill("yinbing")
    {
        events << EventPhaseStart << Damaged;
        view_as_skill = new YinbingViewAsSkill;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish && !player->isNude()) {
            room->askForUseCard(player, "@@yinbing", "@yinbing", -1, Card::MethodNone);
        } else if (triggerEvent == Damaged && !player->getPile("yinbing").isEmpty()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel"))) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());

                QList<int> ids = player->getPile("yinbing");
                room->fillAG(ids, player);
                int id = room->askForAG(player, ids, false, objectName());
                room->clearAG(player);
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), player->objectName(), "yinbing", QString());
                room->throwCard(Sanguosha->getCard(id), reason, NULL);
            }
        }

        return false;
    }
};

class Juedi : public PhaseChangeSkill
{
public:
    Juedi() : PhaseChangeSkill("juedi")
    {
		view_as_skill = new dummyVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start
            && !target->getPile("yinbing").isEmpty();
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        QStringList choices;
		choices << "self";
	    QList<ServerPlayer *> playerlist;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (p->getHp() <= target->getHp())
                playerlist << p;
        }
		if (!playerlist.isEmpty())
			choices << "give";
		choices << "cancel";
		QString choice = room->askForChoice(target, objectName(), choices.join("+"));
		if (choice == "cancel") {
            return false;
        } else {
            if (choice == "give") {
                ServerPlayer *to_give = room->askForPlayerChosen(target, playerlist, objectName(), "@juedi", true, true);
				if (to_give) {
                    target->broadcastSkillInvoke(objectName());
                    room->recover(to_give, RecoverStruct(target));
                    DummyCard *dummy = new DummyCard(target->getPile("yinbing"));
                    dummy->deleteLater();
                    room->obtainCard(to_give, dummy);
                }
            } else {
				LogMessage log;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                log.from = target;
                room->sendLog(log);
                room->notifySkillInvoked(target, objectName());
                target->broadcastSkillInvoke(objectName());
                int len = target->getPile("yinbing").length();
                target->clearOnePrivatePile("yinbing");
                if (target->isAlive())
                    room->drawCards(target, len, objectName());
            }
        }
        return false;
    }
};

FenxunCard::FenxunCard()
{
}

bool FenxunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void FenxunCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    effect.from->tag["FenxunTarget"] = QVariant::fromValue(effect.to);
    room->setFixedDistance(effect.from, effect.to, 1);
}

class FenxunViewAsSkill : public OneCardViewAsSkill
{
public:
    FenxunViewAsSkill() : OneCardViewAsSkill("fenxun")
    {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("FenxunCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const
    {
        FenxunCard *first = new FenxunCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Fenxun : public TriggerSkill
{
public:
    Fenxun() : TriggerSkill("fenxun")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new FenxunViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->tag["FenxunTarget"].value<ServerPlayer *>() != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *dingfeng, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != dingfeng)
                return false;
        }
        ServerPlayer *target = dingfeng->tag["FenxunTarget"].value<ServerPlayer *>();

        if (target) {
            room->removeFixedDistance(dingfeng, target, 1);
            dingfeng->tag.remove("FenxunTarget");
        }
        return false;
    }
};

class Duanbing : public TriggerSkill
{
public:
    Duanbing() : TriggerSkill("duanbing")
    {
        events << TargetChosen;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
		CardUseStruct use = data.value<CardUseStruct>();
		if (use.card->isKindOf("Slash") && !use.card->hasFlag("slashDisableExtraTarget")) {
			QList<ServerPlayer *> available_targets;
			bool no_distance_limit = false;
			if (use.card->hasFlag("slashNoDistanceLimit")){
				no_distance_limit = true;
				room->setPlayerFlag(player, "slashNoDistanceLimit");
			}
			foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (use.to.contains(p) || room->isProhibited(player, p, use.card)) continue;
                if (use.card->targetFilter(QList<const Player *>(), p, player) && player->distanceTo(p) == 1)
                    available_targets << p;
            }
			if (no_distance_limit)
				room->setPlayerFlag(player, "-slashNoDistanceLimit");
			if (available_targets.isEmpty())
				return false;
			ServerPlayer *extra = room->askForPlayerChosen(player, available_targets, objectName(), "@duanbing-add", true);
			if (extra) {
				LogMessage log;
                log.type = "#QiaoshuiAdd";
                log.from = player;
                log.to << extra;
                log.card_str = use.card->toString();
                log.arg = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                player->broadcastSkillInvoke(objectName());
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), extra->objectName());
			    use.to.append(extra);
                room->sortByActionOrder(use.to);
		        data = QVariant::fromValue(use);
			}
		}
        return false;
    }
};

class Gongao : public TriggerSkill
{
public:
    Gongao() : TriggerSkill("gongao")
    {
        events << BuryVictim;
        frequency = Compulsory;
    }

	int getPriority(TriggerEvent) const
    {
        return -2;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &) const
    {
		foreach (ServerPlayer *zhugedan, room->getAlivePlayers()) {
            if (!TriggerSkill::triggerable(zhugedan)) continue;
            zhugedan->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(zhugedan, objectName());

            LogMessage log;
            log.type = "#GainMaxHp";
            log.from = zhugedan;
            log.arg = "1";
            log.arg2 = QString::number(zhugedan->getMaxHp() + 1);
            room->sendLog(log);

            room->setPlayerProperty(zhugedan, "maxhp", zhugedan->getMaxHp() + 1);

            if (zhugedan->isWounded())
                room->recover(zhugedan, RecoverStruct(zhugedan));
        }
        return false;
    }
};

class Juyi : public PhaseChangeSkill
{
public:
    Juyi() : PhaseChangeSkill("juyi")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("juyi") == 0
            && target->isWounded()
            && target->getMaxHp() > target->aliveCount();
    }

    bool onPhaseChange(ServerPlayer *zhugedan) const
    {
        Room *room = zhugedan->getRoom();

        zhugedan->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(zhugedan, objectName());
        //room->doLightbox("$JuyiAnimate");

        room->setPlayerMark(zhugedan, "juyi", 1);
        room->addPlayerMark(zhugedan, "@waked");
        int diff = zhugedan->getHandcardNum() - zhugedan->getMaxHp();
        if (diff < 0)
            room->drawCards(zhugedan, -diff, objectName());
        if (zhugedan->getMark("juyi") == 1)
            room->handleAcquireDetachSkills(zhugedan, "benghuai|weizhong");

        return false;
    }
};

class Weizhong : public TriggerSkill
{
public:
    Weizhong() : TriggerSkill("weizhong")
    {
        events << MaxHpChanged;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        player->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());

        player->drawCards(1, objectName());
        return false;
    }
};

class Zhendu : public TriggerSkill
{
public:
    Zhendu() : TriggerSkill("zhendu")
    {
        events << EventPhaseStart;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Play)
            return false;

        foreach (ServerPlayer *hetaihou, room->getOtherPlayers(player)) {
            if (player->isDead() || !TriggerSkill::triggerable(hetaihou) || !hetaihou->canDiscard(hetaihou, "h"))
                continue;

            if (room->askForCard(hetaihou, ".", "@zhendu-discard:" + player->objectName(), QVariant(), objectName(), player)) {
                Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
                analeptic->setSkillName("_zhendu");
                room->useCard(CardUseStruct(analeptic, player, QList<ServerPlayer *>()), true);
                if (player->isAlive())
                    room->damage(DamageStruct(objectName(), hetaihou, player));
            }
        }
        return false;
    }
	
	virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Analeptic"))
            return 0;
        else
            return -1;
    }
};

class QiluanCount : public TriggerSkill
{
public:
    QiluanCount() : TriggerSkill("#qiluan-count")
    {
        events << Death << TurnStart;
        global = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
            ServerPlayer *killer = death.damage ? death.damage->from : NULL;
            ServerPlayer *current = room->getCurrent();

            if (killer && current && (current->isAlive() || death.who == current)
                && current->getPhase() != Player::NotActive) {
                killer->addMark("qiluan");
            }
        } else {
            foreach(ServerPlayer *p, room->getAlivePlayers())
                p->setMark("qiluan", 0);
        }

        return false;
    }
};

class Qiluan : public PhaseChangeSkill
{
public:
    Qiluan() : PhaseChangeSkill("qiluan")
    {
        frequency = Frequent;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 1;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getMark(objectName()) > 0 && TriggerSkill::triggerable(p)) {
                    int n = 3*p->getMark(objectName());
				    if (p->askForSkillInvoke(this, "prompt:::" + QString::number(n))){
                        p->broadcastSkillInvoke(objectName());
                        p->drawCards(n, objectName());
					}
                }
            }
        }
        return false;
    }
};

XiemuCard::XiemuCard()
{
    target_fixed = true;
}

void XiemuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QString kingdom = room->askForKingdom(source, "xiemu");
    room->addPlayerTip(source, "#xiemu_" + kingdom);
}

class XiemuViewAsSkill : public OneCardViewAsSkill
{
public:
    XiemuViewAsSkill() : OneCardViewAsSkill("xiemu")
    {
        filter_pattern = "Slash";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("XiemuCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        XiemuCard *card = new XiemuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Xiemu : public TriggerSkill
{
public:
    Xiemu() : TriggerSkill("xiemu")
    {
        events << TargetConfirmed << EventPhaseStart;
        view_as_skill = new XiemuViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && player != use.from && use.card->getTypeId() != Card::TypeSkill
                && use.card->isBlack() && use.to.contains(player)
                && player->getMark("#xiemu_" + use.from->getKingdom()) > 0 && player->askForSkillInvoke(objectName())) {
                player->drawCards(2, objectName());
            }
        } else {
            if (player->getPhase() == Player::RoundStart) {
                foreach (QString kingdom, Sanguosha->getKingdoms()) {
                    QString markname = "#xiemu_" + kingdom;
                    if (player->getMark(markname) > 0)
                        room->removePlayerTip(player, markname);
                }
            }
        }
        return false;
    }
};

class Naman : public TriggerSkill
{
public:
    Naman() : TriggerSkill("naman")
    {
        events << CardsMoveOneTime;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place != Player::DiscardPile) return false;
        const Card *to_obtain = NULL;
        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
            if (move.from && player->objectName() == move.from->objectName())
                return false;
            to_obtain = move.reason.m_extraData.value<const Card *>();
            if (!to_obtain || !to_obtain->isKindOf("Slash"))
                return false;
        } else {
            return false;
        }
        if (to_obtain) {
            foreach(int id, to_obtain->getSubcards()){
				if (room->getCardPlace(id) != Player::DiscardPile)
					return false;
			}
			if (room->askForSkillInvoke(player, objectName(), data)){
                player->broadcastSkillInvoke(objectName());
                room->obtainCard(player, to_obtain);
			}
        }

        return false;
    }
};

class Shushen : public TriggerSkill
{
public:
    Shushen() : TriggerSkill("shushen")
    {
        events << HpRecover;
		view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        RecoverStruct recover_struct = data.value<RecoverStruct>();
        int recover = recover_struct.recover;
        for (int i = 0; i < recover; i++) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "shushen-invoke", true, true);
            if (target) {
                player->broadcastSkillInvoke(objectName());
                if (target->isWounded() && room->askForChoice(target, objectName(), "recover+draw", data) == "recover")
                    room->recover(target, RecoverStruct(player));
                else
                    target->drawCards(2, objectName());
            } else {
                break;
            }
        }
        return false;
    }
};

class Shenzhi : public PhaseChangeSkill
{
public:
    Shenzhi() : PhaseChangeSkill("shenzhi")
    {
    }

    virtual bool onPhaseChange(ServerPlayer *ganfuren) const
    {
        Room *room = ganfuren->getRoom();
        if (ganfuren->getPhase() != Player::Start || ganfuren->isKongcheng())
            return false;
        if (room->askForSkillInvoke(ganfuren, objectName())) {
            // As the cost, if one of her handcards cannot be throwed, the skill is unable to invoke
            foreach (const Card *card, ganfuren->getHandcards()) {
                if (ganfuren->isJilei(card))
                    return false;
            }
            //==================================
            int handcard_num = ganfuren->getHandcardNum();
            ganfuren->broadcastSkillInvoke(objectName());
            ganfuren->throwAllHandCards();
            if (handcard_num >= ganfuren->getHp())
                room->recover(ganfuren, RecoverStruct(ganfuren));
        }
        return false;
    }
};

class Fulu : public OneCardViewAsSkill
{
public:
    Fulu() : OneCardViewAsSkill("fulu")
    {
        filter_pattern = "Slash";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE && pattern == "slash";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        ThunderSlash *acard = new ThunderSlash(originalCard->getSuit(), originalCard->getNumber());
        acard->addSubcard(originalCard->getId());
        acard->setSkillName(objectName());
        return acard;
    }
};

class Zhuji : public TriggerSkill
{
public:
    Zhuji() : TriggerSkill("zhuji")
    {
        events << DamageCaused << FinishJudge;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature != DamageStruct::Thunder || !damage.from)
                return false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (TriggerSkill::triggerable(p) && room->askForSkillInvoke(p, objectName(), data)) {
                    p->broadcastSkillInvoke(objectName());
                    JudgeStruct judge;
                    judge.good = true;
                    judge.reason = objectName();
                    judge.pattern = ".";
                    judge.who = damage.from;

                    room->judge(judge);
                    if (judge.pattern == "black") {
                        LogMessage log;
                        log.type = "#ZhujiBuff";
                        log.from = p;
                        log.to << damage.to;
                        log.arg = QString::number(damage.damage);
                        log.arg2 = QString::number(++damage.damage);
                        room->sendLog(log);

                        data = QVariant::fromValue(damage);
                    }
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName()) {
                judge->pattern = (judge->card->isRed() ? "red" : "black");
                if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge && judge->card->isRed())
                    player->obtainCard(judge->card);
            }
        }
        return false;
    }
};

class SpZhenwei : public TriggerSkill
{
public:
    SpZhenwei() : TriggerSkill("spzhenwei")
    {
        events << TargetConfirming << EventPhaseChanging;
		view_as_skill = new dummyVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->getPile("zhenweipile").isEmpty()) {
                        DummyCard *dummy = new DummyCard(p->getPile("zhenweipile"));
                        dummy->deleteLater();
                        room->obtainCard(p, dummy);
                    }
                }
            }
            return false;
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card == NULL || use.to.length() != 1 || !(use.card->isKindOf("Slash") || (use.card->getTypeId() == Card::TypeTrick && use.card->isBlack())))
                return false;
            ServerPlayer *wp = room->findPlayerBySkillName(objectName());
            if (wp == NULL || wp->getHp() <= player->getHp())
                return false;
            if (!room->askForCard(wp, "..", QString("@sp_zhenwei:%1").arg(player->objectName()), data, objectName()))
                return false;
            QString choice = room->askForChoice(wp, objectName(), "draw+null", data);
            if (choice == "draw") {
                room->drawCards(wp, 1, objectName());

                if (use.card->isKindOf("Slash")) {
                    if (!use.from->canSlash(wp, use.card, false))
                        return false;
                }

                if (use.from->isProhibited(wp, use.card))
                    return false;

				if (use.card->isKindOf("DelayedTrick") && wp->containsTrick(use.card->objectName()))
					return false;

                if (use.card->isKindOf("Collateral")) {
                    ServerPlayer *victim = player->tag["collateralVictim"].value<ServerPlayer *>();
                    player->tag.remove("collateralVictim");
                    wp->tag["collateralVictim"] = QVariant::fromValue(victim);
                }

                use.to = QList<ServerPlayer *>();
                use.to << wp;
                data = QVariant::fromValue(use);
				room->getThread()->trigger(TargetConfirming, room, wp, data);
                return true;
            } else {
                room->setCardFlag(use.card, "zhenweinull");
                use.from->addToPile("zhenweipile", use.card);

                use.nullified_list << "_ALL_TARGETS";
                data = QVariant::fromValue(use);
            }
            return false;
        }
        return false;
    }
};

class Junbing : public TriggerSkill
{
public:
    Junbing() : TriggerSkill("junbing")
    {
        events << EventPhaseStart;
		view_as_skill = new dummyVS;
    }

    bool triggerable(const ServerPlayer *player) const
    {
        return player && player->isAlive() && player->getPhase() == Player::Finish && player->getHandcardNum() <= 1;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        ServerPlayer *simalang = room->findPlayerBySkillName(objectName());
        if (!simalang || !simalang->isAlive())
            return false;
        if (player->askForSkillInvoke("junbing_invoke", "prompt:" + simalang->objectName())) {
			LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = player;
            log.to << simalang;
            log.arg = objectName();
            room->sendLog(log);
            simalang->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(simalang, objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), simalang->objectName());
			
			
            player->drawCards(1);
            if (player->objectName() != simalang->objectName() && !player->isKongcheng()) {
                DummyCard *cards = player->wholeHandCards();
                cards->deleteLater();
                CardMoveReason reason = CardMoveReason(CardMoveReason::S_REASON_GIVE, player->objectName());
                room->moveCardTo(cards, simalang, Player::PlaceHand, reason);

                int x = qMin(cards->subcardsLength(), simalang->getHandcardNum());

                if (x > 0) {
                    const Card *return_cards = room->askForExchange(simalang, objectName(), x, x, true, QString("@junbing-return:%1::%2").arg(player->objectName()).arg(cards->subcardsLength()));
                    CardMoveReason return_reason = CardMoveReason(CardMoveReason::S_REASON_GIVE, simalang->objectName());
                    room->moveCardTo(return_cards, player, Player::PlaceHand, return_reason);
                }
            }
        }
        return false;
    }
};

QujiCard::QujiCard()
{
}

bool QujiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (subcardsLength() <= targets.length())
        return false;
    return to_select->isWounded();
}

bool QujiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    if (targets.length() > 0) {
        foreach (const Player *p, targets) {
            if (!p->isWounded())
                return false;
        }
        return true;
    }
    return false;
}

void QujiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach(ServerPlayer *p, targets)
        room->cardEffect(this, source, p);

    foreach (int id, getSubcards()) {
        if (Sanguosha->getCard(id)->isBlack()) {
            room->loseHp(source);
            break;
        }
    }
}

void QujiCard::onEffect(const CardEffectStruct &effect) const
{
    RecoverStruct recover;
    recover.who = effect.from;
    recover.recover = 1;
    effect.to->getRoom()->recover(effect.to, recover);
}

class Quji : public ViewAsSkill
{
public:
    Quji() : ViewAsSkill("quji")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        return selected.length() < Self->getLostHp();
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->isWounded() && !player->hasUsed("QujiCard");
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == Self->getLostHp()) {
            QujiCard *quji = new QujiCard;
            quji->addSubcards(cards);
            return quji;
        }
        return NULL;
    }
};

class Qizhi : public TriggerSkill
{
public:
    Qizhi() : TriggerSkill("qizhi")
    {
        events << TargetSpecified << EventPhaseChanging;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
		return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(player) && player->getPhase() != Player::NotActive) {
			CardUseStruct use = data.value<CardUseStruct>();
		    if (use.card->getTypeId() != Card::TypeBasic && use.card->getTypeId() != Card::TypeTrick)
				return false;
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && player->canDiscard(p, "he"))
                    targets << p;
            }
            if (targets.isEmpty()) return false;
			ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@qizhi-target", true, true);
			if (target){
                room->addPlayerMark(player, "#qizhi");
                player->broadcastSkillInvoke(objectName());
                int id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
                CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, player->objectName(), target->objectName(), objectName(), QString());
                room->throwCard(Sanguosha->getCard(id), reason, target, player);
                target->drawCards(1, objectName());
			}
        } else if (triggerEvent == EventPhaseChanging) {
			if (data.value<PhaseChangeStruct>().to == Player::NotActive)
                room->setPlayerMark(player, "#qizhi", 0);
		}
        
        return false;
    }
};

class Jinqu : public PhaseChangeSkill
{
public:
    Jinqu() : PhaseChangeSkill("jinqu")
    {
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        if (player->getPhase() == Player::Finish) {
            Room *room = player->getRoom();
            if (room->askForSkillInvoke(player, objectName())) {
                player->broadcastSkillInvoke(objectName());
                player->drawCards(2, objectName());
				int n = player->getHandcardNum() - player->getMark("#qizhi");
				if (n > 0)
				    room->askForDiscard(player, objectName(), n, n, false, false, "@jinqu-discard");
			}
        }

        return false;
    }
};

class Fentian : public PhaseChangeSkill
{
public:
    Fentian() : PhaseChangeSkill("fentian")
    {
        frequency = Compulsory;
    }

    bool onPhaseChange(ServerPlayer *hanba) const
    {
        if (hanba->getPhase() != Player::Finish)
            return false;

        if (hanba->getHandcardNum() >= hanba->getHp())
            return false;

        QList<ServerPlayer*> targets;
        Room* room = hanba->getRoom();

        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (hanba->inMyAttackRange(p) && !p->isNude())
                targets << p;
        }

        if (targets.isEmpty())
            return false;

        room->sendCompulsoryTriggerLog(hanba, objectName());
        hanba->broadcastSkillInvoke(objectName());
        ServerPlayer *target = room->askForPlayerChosen(hanba, targets, objectName(), "@fentian-choose");
		if (target){
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, hanba->objectName(), target->objectName());
            int id = room->askForCardChosen(hanba, target, "he", objectName());
            hanba->addToPile("burn", id);
		}
        return false;
    }
};

class FentianRange : public AttackRangeSkill
{
public:
    FentianRange() : AttackRangeSkill("#fentian")
    {

    }

    int getExtra(const Player *target, bool) const
    {
        if (target->hasSkill(this))
            return target->getPile("burn").length();

        return 0;
    }
};

class Zhiri : public PhaseChangeSkill
{
public:
    Zhiri() : PhaseChangeSkill("zhiri")
    {
        frequency = Wake;
    }

    bool onPhaseChange(ServerPlayer *hanba) const
    {
        if (hanba->getMark(objectName()) > 0 || hanba->getPhase() != Player::Start)
            return false;

        if (hanba->getPile("burn").length() < 3)
            return false;

        Room *room = hanba->getRoom();
		room->sendCompulsoryTriggerLog(hanba, objectName());
        hanba->broadcastSkillInvoke(objectName());

        room->setPlayerMark(hanba, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(hanba) && hanba->getMark("zhiri") > 0)
            room->acquireSkill(hanba, "xintan");

        return false;
    };

};

XintanCard::XintanCard()
{
}

bool XintanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void XintanCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->loseHp(effect.to);
}

class Xintan : public ViewAsSkill
{
public:
    Xintan() : ViewAsSkill("xintan")
    {
        expand_pile = "burn";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getPile("burn").length() >= 2 && !player->hasUsed("XintanCard");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && Self->getPile("burn").contains(to_select->getId());
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            XintanCard *xt = new XintanCard;
            xt->addSubcards(cards);
            return xt;
        }
        return NULL;
    }
};

class Kunfen : public PhaseChangeSkill
{
public:
    Kunfen() : PhaseChangeSkill("kunfen")
    {

    }

    virtual Frequency getFrequency(const Player *target) const
    {
        if (target != NULL) {
            return target->getMark("fengliang") > 0 ? NotFrequent : Compulsory;
        }

        return Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Finish;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        if (invoke(target))
            effect(target);

        return false;
    }

private:
    bool invoke(ServerPlayer *target) const
    {
        if (getFrequency(target) == Compulsory){
			Room *room = target->getRoom();
			room->sendCompulsoryTriggerLog(target, objectName());
			return true;
		}else
			return target->askForSkillInvoke(this);
    }

    void effect(ServerPlayer *target) const
    {
        Room *room = target->getRoom();

        target->broadcastSkillInvoke(objectName());

        room->loseHp(target);
        if (target->isAlive())
            target->drawCards(2, objectName());
    }
};

class Fengliang : public TriggerSkill
{
public:
    Fengliang() : TriggerSkill("fengliang")
    {
        frequency = Wake;
        events << Dying;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark(objectName()) == 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		DyingStruct dying = data.value<DyingStruct>();
        if (dying.who != player)
            return false;

        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());

        room->addPlayerMark(player, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(player) && player->getMark(objectName()) > 0) {
            int recover = 2 - player->getHp();
            room->recover(player, RecoverStruct(NULL, NULL, recover));
            room->handleAcquireDetachSkills(player, "tiaoxin");

            if (player->hasSkill("kunfen", true)) {
                QString translation = Sanguosha->translate(":kunfen-frequent");
                Sanguosha->addTranslationEntry(":kunfen", translation.toStdString().c_str());
                room->doNotify(player, QSanProtocol::S_COMMAND_UPDATE_SKILL, QVariant("kunfen"));
            }
        }

        return false;
    }
};

JiqiaoCard::JiqiaoCard()
{
	target_fixed = true;
	will_throw = true;
}

void JiqiaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QList<int> ids = room->getNCards(subcards.length() * 2, false);
    CardsMoveStruct move(ids, source, Player::PlaceTable,
        CardMoveReason(CardMoveReason::S_REASON_TURNOVER, source->objectName(), "jiqiao", QString()));
    room->moveCardsAtomic(move, true);
	QList<int> card_to_throw;
    QList<int> card_to_gotback;
    foreach (int id, ids) {
        if (Sanguosha->getCard(id)->isKindOf("TrickCard"))
            card_to_gotback << id;
        else
            card_to_throw << id;
    }
    if (!card_to_gotback.isEmpty()) {
        DummyCard *dummy = new DummyCard(card_to_gotback);
        dummy->deleteLater();
        room->obtainCard(source, dummy);
    }
    if (!card_to_throw.isEmpty()) {
        DummyCard *dummy2 = new DummyCard(card_to_throw);
        dummy2->deleteLater();
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, source->objectName(), "jiqiao", QString());
        room->throwCard(dummy2, reason, NULL);
    }
}

class JiqiaoViewAsSkill : public ViewAsSkill
{
public:
    JiqiaoViewAsSkill() : ViewAsSkill("jiqiao")
    {
        response_pattern = "@@jiqiao";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        JiqiaoCard *jiqiao_card = new JiqiaoCard;
        jiqiao_card->addSubcards(cards);
        jiqiao_card->setSkillName(objectName());
        return jiqiao_card;
    }
};

class Jiqiao : public PhaseChangeSkill
{
public:
    Jiqiao() : PhaseChangeSkill("jiqiao")
    {
        view_as_skill = new JiqiaoViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *yueying) const
    {
        Room *room = yueying->getRoom();
        if (yueying->getPhase() == Player::Play && yueying->canDiscard(yueying, "he"))
            room->askForUseCard(yueying, "@@jiqiao", "@jiqiao-card", -1, Card::MethodDiscard);
        return false;
    }
};

class Linglong : public MaxCardsSkill
{
public:
    Linglong() : MaxCardsSkill("linglong")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill("linglong") && !target->getOffensiveHorse() && !target->getDefensiveHorse())
            return 1;
        else
            return 0;
    }

    int getEffectIndex(const ServerPlayer *, const QString &prompt) const
    {
        if (prompt == "MaxCardsSkill")
			return 2;
		return -1;
    }
};

class LinglongTrigger : public TriggerSkill
{
public:
    LinglongTrigger() : TriggerSkill("#linglong")
    {
        frequency = Compulsory;
        events << CardAsked << CardsMoveOneTime << EventAcquireSkill << EventLoseSkill << GameStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *yueying, QVariant &data) const
    {
		if (triggerEvent == CardAsked && yueying->hasSkill("linglong") && !yueying->getArmor() && yueying->hasArmorEffect("eight_diagram")){
            QString pattern = data.toStringList().first();

            if (pattern != "jink")
                return false;
		    room->sendCompulsoryTriggerLog(yueying, "linglong");
            yueying->broadcastSkillInvoke("linglong", 1);
            if (!yueying->askForSkillInvoke("eight_diagram"))
				return false;
            room->setEmotion(yueying, "armor/eight_diagram");
			JudgeStruct judge;
            judge.pattern = ".|red";
            judge.good = true;
            judge.reason = "eight_diagram";
            judge.who = yueying;

            room->judge(judge);

            if (judge.isGood()) {
                Jink *jink = new Jink(Card::NoSuit, 0);
                jink->setSkillName("eight_diagram");
                room->provide(jink);
                return true;
            }
        }else if (triggerEvent == EventLoseSkill && data.toString() == "linglong") {
            room->handleAcquireDetachSkills(yueying, "-qicai", true);
			yueying->setMark("linglong_qicai", 0);
		} else if ((triggerEvent == EventAcquireSkill && data.toString() == "linglong") || (triggerEvent == GameStart && yueying->hasSkill("linglong"))) {
            if (yueying->getTreasure() == NULL) {
                room->notifySkillInvoked(yueying, "linglong");
                room->handleAcquireDetachSkills(yueying, "qicai");
				yueying->setMark("linglong_qicai", 1);
            }
        } else if (triggerEvent == CardsMoveOneTime && yueying->isAlive() && yueying->hasSkill(this, true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == yueying && move.to_place == Player::PlaceEquip) {
                if (yueying->getTreasure() != NULL && yueying->getMark("linglong_qicai") > 0) {
                    room->handleAcquireDetachSkills(yueying, "-qicai", true);
					yueying->setMark("linglong_qicai", 0);
				}
            } else if (move.from == yueying && move.from_places.contains(Player::PlaceEquip)) {
				if (yueying->getTreasure() == NULL && yueying->getMark("linglong_qicai") == 0) {
                    room->notifySkillInvoked(yueying, "linglong");
                    room->handleAcquireDetachSkills(yueying, "qicai");
					yueying->setMark("linglong_qicai", 1);
                }
            }
		}

        return false;
    }
};

class Chongzhen : public TriggerSkill
{
public:
    Chongzhen() : TriggerSkill("chongzhen")
    {
        events << CardResponded << TargetSpecified;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->getSkillName() == "longdan"
                && resp.m_who != NULL && !resp.m_who->isKongcheng()) {
                QVariant data = QVariant::fromValue(resp.m_who);
                if (player->askForSkillInvoke(this, data)) {
                    player->broadcastSkillInvoke("chongzhen");
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), resp.m_who->objectName());
                    int card_id = room->askForCardChosen(player, resp.m_who, "h", objectName());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
                    room->obtainCard(player, Sanguosha->getCard(card_id), reason, false);
                }
            }
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == "longdan") {
                foreach (ServerPlayer *p, use.to) {
                    if (p->isKongcheng()) continue;
                    QVariant data = QVariant::fromValue(p);
                    p->setFlags("ChongzhenTarget");
                    bool invoke = player->askForSkillInvoke(this, data);
                    p->setFlags("-ChongzhenTarget");
                    if (invoke) {
                        player->broadcastSkillInvoke("chongzhen");
						room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
                        int card_id = room->askForCardChosen(player, p, "h", objectName());
                        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
                        room->obtainCard(player, Sanguosha->getCard(card_id), reason, false);
                    }
                }
            }
        }
        return false;
    }
};

LihunCard::LihunCard()
{

}

bool LihunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->isMale() && to_select != Self;
}

void LihunCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
    room->moveCardTo(this, NULL, Player::DiscardPile, reason, true);
	card_use.from->turnOver();
}

void LihunCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    effect.to->setFlags("LihunTarget");
    DummyCard *dummy_card = new DummyCard(effect.to->handCards());
    dummy_card->deleteLater();
    if (!effect.to->isKongcheng()) {
        CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, effect.from->objectName(),
            effect.to->objectName(), "lihun", QString());
        room->moveCardTo(dummy_card, effect.to, effect.from, Player::PlaceHand, reason, false);
    }
}

class LihunSelect : public OneCardViewAsSkill
{
public:
    LihunSelect() : OneCardViewAsSkill("lihun")
    {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LihunCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        LihunCard *card = new LihunCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Lihun : public TriggerSkill
{
public:
    Lihun() : TriggerSkill("lihun")
    {
        events << EventPhaseStart << EventPhaseEnd;
        view_as_skill = new LihunSelect;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasUsed("LihunCard");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *diaochan, QVariant &) const
    {
        if (triggerEvent == EventPhaseEnd && diaochan->getPhase() == Player::Play) {
            ServerPlayer *target = NULL;
            foreach (ServerPlayer *other, room->getOtherPlayers(diaochan)) {
                if (other->hasFlag("LihunTarget")) {
                    other->setFlags("-LihunTarget");
                    target = other;
                    break;
                }
            }

            if (!target || target->getHp() < 1 || diaochan->isNude())
                return false;

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, diaochan->objectName(), target->objectName());
            diaochan->broadcastSkillInvoke(objectName(), 2);
            DummyCard *to_goback;
            if (diaochan->getCardCount() <= target->getHp()) {
                to_goback = diaochan->isKongcheng() ? new DummyCard : diaochan->wholeHandCards();
                for (int i = 0; i < 4; i++)
                    if (diaochan->getEquip(i))
                        to_goback->addSubcard(diaochan->getEquip(i)->getEffectiveId());
            } else
                to_goback = (DummyCard *)room->askForExchange(diaochan, objectName(), target->getHp(), target->getHp(), true, "LihunGoBack");

            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, diaochan->objectName(),
                target->objectName(), objectName(), QString());
            room->moveCardTo(to_goback, diaochan, target, Player::PlaceHand, reason);
            to_goback->deleteLater();
        } else if (triggerEvent == EventPhaseStart && diaochan->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("LihunTarget"))
                    p->setFlags("-LihunTarget");
            }
        }

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("LihunCard"))
            return 1;
        return -1;
    }
};

class Kuiwei : public TriggerSkill
{
public:
    Kuiwei() : TriggerSkill("kuiwei")
    {
        events << EventPhaseStart;
    }

    static int getWeaponCount(ServerPlayer *caoren)
    {
        int n = 0;
        foreach (ServerPlayer *p, caoren->getRoom()->getAlivePlayers()) {
            if (p->getWeapon()) n++;
        }
        return n;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->isAlive()
            && (target->hasSkill(this) || target->getMark("@kuiwei") > 0);
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *caoren, QVariant &) const
    {
        if (caoren->getPhase() == Player::Finish) {
            if (!caoren->hasSkill(this))
                return false;
            if (!caoren->askForSkillInvoke(this))
                return false;

            caoren->broadcastSkillInvoke(objectName());
            int n = getWeaponCount(caoren);
            caoren->drawCards(n + 2, objectName());
            caoren->turnOver();

            if (caoren->getMark("@kuiwei") == 0)
                room->addPlayerMark(caoren, "@kuiwei");
        } else if (caoren->getPhase() == Player::Draw) {
            if (caoren->getMark("@kuiwei") == 0)
                return false;
            room->removePlayerMark(caoren, "@kuiwei");
            int n = getWeaponCount(caoren);
            if (n > 0) {
                caoren->broadcastSkillInvoke(objectName());
                LogMessage log;
                log.type = "#KuiweiDiscard";
                log.from = caoren;
                log.arg = QString::number(n);
                log.arg2 = objectName();
                room->sendLog(log);

                room->askForDiscard(caoren, objectName(), n, n, false, true);
            }
        }
        return false;
    }
};

class Yanzheng : public OneCardViewAsSkill
{
public:
    Yanzheng() : OneCardViewAsSkill("yanzheng")
    {
        filter_pattern = ".|.|.|equipped";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "nullification" && player->getHandcardNum() > player->getHp();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Nullification *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->getHandcardNum() > player->getHp() && !player->getEquips().isEmpty();
    }
};

SPPackage::SPPackage()
: Package("sp")
{
    General *yangxiu = new General(this, "yangxiu", "wei", 3); // SP 001
    yangxiu->addSkill(new Jilei);
    yangxiu->addSkill(new JileiClear);
    yangxiu->addSkill(new Danlao);
    related_skills.insertMulti("jilei", "#jilei-clear");

    General *sp_diaochan = new General(this, "sp_diaochan", "qun", 3, false); // *SP 002
    sp_diaochan->addSkill(new Lihun);
    sp_diaochan->addSkill("biyue");

    General *sp_yuanshu = new General(this, "sp_yuanshu", "qun"); // SP 004
    sp_yuanshu->addSkill(new Yongsi);
    sp_yuanshu->addSkill(new SpJixi);

	General *sp_sunshangxiang = new General(this, "sp_sunshangxiang", "shu", 3, false); // sp 001
    sp_sunshangxiang->addSkill(new Liangzhu);
    sp_sunshangxiang->addSkill(new Fanxiang);
	sp_sunshangxiang->addRelateSkill("xiaoji");

    General *sp_caoren = new General(this, "sp_caoren", "wei"); // *SP 003
    sp_caoren->addSkill(new Kuiwei);
    sp_caoren->addSkill(new Yanzheng);

    General *sp_pangde = new General(this, "sp_pangde", "wei"); // SP 006
    sp_pangde->addSkill("mashu");
    sp_pangde->addSkill(new Juesi);

    General *sp_guanyu = new General(this, "sp_guanyu", "wei"); // sp 003
    sp_guanyu->addSkill("wusheng");
    sp_guanyu->addSkill(new Danji);
    sp_guanyu->addRelateSkill("nuzhan");
	related_skills.insertMulti("nuzhan", "#nuzhan");

    General *shenlvbu1 = new General(this, "shenlvbu1", "god", 8, true, true); // SP 008 (2-1)
    shenlvbu1->addSkill("mashu");
    shenlvbu1->addSkill("wushuang");

    General *shenlvbu2 = new General(this, "shenlvbu2", "god", 4, true, true); // SP 008 (2-2)
    shenlvbu2->addSkill("mashu");
    shenlvbu2->addSkill("wushuang");
    shenlvbu2->addSkill(new Xiuluo);
    shenlvbu2->addSkill(new ShenweiKeep);
    shenlvbu2->addSkill(new Shenwei);
    shenlvbu2->addSkill(new Shenji);
    related_skills.insertMulti("shenwei", "#shenwei-draw");

    General *sp_caiwenji = new General(this, "sp_caiwenji", "wei", 3, false); // SP 009
    sp_caiwenji->addSkill(new Chenqing);
	sp_caiwenji->addSkill(new ChenqingClear);
	related_skills.insertMulti("chenqing", "#chenqing-clear");
	sp_caiwenji->addSkill(new Mozhi);

	General *sp_zhaoyun = new General(this, "sp_zhaoyun", "qun", 3); // *SP 001
    sp_zhaoyun->addSkill("longdan");
    sp_zhaoyun->addSkill(new Chongzhen);

    General *sp_machao = new General(this, "sp_machao", "qun"); // SP 011
    sp_machao->addSkill(new Skill("zhuiji", Skill::Compulsory));
    sp_machao->addSkill(new Shichou);

    General *sp_jiaxu = new General(this, "sp_jiaxu", "wei", 3); // SP 012
    sp_jiaxu->addSkill(new Zhenlve);
	sp_jiaxu->addSkill(new ZhenlveTrick);
	related_skills.insertMulti("zhenlve", "#zhenlve-trick");
    sp_jiaxu->addSkill(new Jianshu);
    sp_jiaxu->addSkill(new Yongdi);

    General *caohong = new General(this, "caohong", "wei"); // SP 013
    caohong->addSkill(new Yuanhu);

    General *guanyinping = new General(this, "guanyinping", "shu", 3, false); // SP 014
    guanyinping->addSkill(new Xueji);
    guanyinping->addSkill(new Huxiao);
    guanyinping->addSkill(new HuxiaoTargetMod);
    guanyinping->addSkill(new Wuji);
    guanyinping->addSkill(new WujiCount);
    related_skills.insertMulti("wuji", "#wuji-count");
    related_skills.insertMulti("huxiao", "#huxiao-target");

	General *liuxie = new General(this, "liuxie", "qun", 3);
    liuxie->addSkill(new Tianming);
    liuxie->addSkill(new Mizhao);

    General *lingju = new General(this, "lingju", "qun", 3, false);
    lingju->addSkill(new Jieyuan);
    lingju->addSkill(new Fenxin);

    General *fuwan = new General(this, "fuwan", "qun", 4);
    fuwan->addSkill(new Moukui);

    General *xiahouba = new General(this, "xiahouba", "shu"); // SP 019
    xiahouba->addSkill(new Baobian);
	xiahouba->addRelateSkill("tiaoxin");
	xiahouba->addRelateSkill("paoxiao");
	xiahouba->addRelateSkill("shensu");

    General *erqiao = new General(this, "erqiao", "wu", 3, false); // SP 021
    erqiao->addSkill(new Xingwu);
	erqiao->addSkill(new DetachEffectSkill("xingwu", "xingwu"));
	related_skills.insertMulti("xingwu", "#xingwu-clear");
    erqiao->addSkill(new Luoyan);
	erqiao->addRelateSkill("liuli");
	erqiao->addRelateSkill("tianxiang");

    General *yuejin = new General(this, "yuejin", "wei", 4, true); // SP 024
    yuejin->addSkill(new Xiaoguo);

    General *caoang = new General(this, "caoang", "wei"); // SP 026
    caoang->addSkill(new Kangkai);

    General *sp_zhugejin = new General(this, "sp_zhugejin", "wu", 3); // SP 027
    sp_zhugejin->addSkill("hongyuan");
	sp_zhugejin->addSkill("#hongyuan");
    sp_zhugejin->addSkill("huanshi");
    sp_zhugejin->addSkill("mingzhe");

    General *xingcai = new General(this, "xingcai", "shu", 3, false); // SP 028
    xingcai->addSkill(new Shenxian);
    xingcai->addSkill(new Qiangwu);
    xingcai->addSkill(new QiangwuTargetMod);
    related_skills.insertMulti("qiangwu", "#qiangwu-target");

    General *panfeng = new General(this, "panfeng", "qun", 4, true); // SP 029
    panfeng->addSkill(new Kuangfu);

    General *zumao = new General(this, "zumao", "wu"); // SP 030
    zumao->addSkill(new Yinbing);
    zumao->addSkill(new Juedi);

    General *dingfeng = new General(this, "dingfeng", "wu", 4, true); // SP 031
    dingfeng->addSkill(new Duanbing);
    dingfeng->addSkill(new Fenxun);

    General *zhugedan = new General(this, "zhugedan", "wei", 4); // SP 032
    zhugedan->addSkill(new Gongao);
    zhugedan->addSkill(new Juyi);
    zhugedan->addRelateSkill("weizhong");
	zhugedan->addRelateSkill("benghuai");

    General *hetaihou = new General(this, "hetaihou", "qun", 3, false); // SP 033
    hetaihou->addSkill(new Zhendu);
    hetaihou->addSkill(new Qiluan);
    hetaihou->addSkill(new QiluanCount);
    related_skills.insertMulti("qiluan", "#qiluan-count");

    General *sunluyu = new General(this, "sunluyu", "wu", 3, false); // SP 034
    sunluyu->addSkill(new Meibu);
    sunluyu->addSkill(new Mumu);

    General *maliang = new General(this, "maliang", "shu", 3); // SP 035
    maliang->addSkill(new Xiemu);
    maliang->addSkill(new Naman);

    General *ganfuren = new General(this, "ganfuren", "shu", 3, false); // SP 037
    ganfuren->addSkill(new Shushen);
    ganfuren->addSkill(new Shenzhi);

    General *huangjinleishi = new General(this, "huangjinleishi", "qun", 3, false); // SP 038
    huangjinleishi->addSkill(new Fulu);
    huangjinleishi->addSkill(new Zhuji);

    General *sp_wenpin = new General(this, "sp_wenpin", "wei"); // SP 039
    sp_wenpin->addSkill(new SpZhenwei);

    General *simalang = new General(this, "simalang", "wei", 3); // SP 040
    simalang->addSkill(new Junbing);
    simalang->addSkill(new Quji);

	General *wangji = new General(this, "wangji", "wei", 3);
	wangji->addSkill(new Qizhi);
	wangji->addSkill(new Jinqu);

    General *sp_jiangwei = new General(this, "sp_jiangwei", "wei");
    sp_jiangwei->addSkill(new Kunfen);
    sp_jiangwei->addSkill(new Fengliang);
	sp_jiangwei->addRelateSkill("tiaoxin");

	General *sp_huangyueying = new General(this, "sp_huangyueying", "qun", 3, false);
	sp_huangyueying->addSkill(new Jiqiao);
	sp_huangyueying->addSkill(new Linglong);
	sp_huangyueying->addSkill(new LinglongTrigger);
	related_skills.insertMulti("linglong", "#linglong");

    addMetaObject<LihunCard>();
	addMetaObject<JuesiCard>();
	addMetaObject<ShichouCard>();
	addMetaObject<JianshuCard>();
    addMetaObject<YuanhuCard>();
    addMetaObject<XuejiCard>();
	addMetaObject<MizhaoCard>();
    addMetaObject<QiangwuCard>();
    addMetaObject<YinbingCard>();
    addMetaObject<FenxunCard>();
	addMetaObject<MumuCard>();
    addMetaObject<XiemuCard>();
    addMetaObject<QujiCard>();
	addMetaObject<JiqiaoCard>();

    skills << new SpJixiRecord << new Nuzhan << new Nuzhan_tms << new MozhiRecord << new Weizhong << new Zhixi << new ZhixiFilter;
}

ADD_PACKAGE(SP)

MiscellaneousPackage::MiscellaneousPackage()
: Package("miscellaneous")
{
    General *hanba = new General(this, "hanba", "qun", 4, false);
    hanba->addSkill(new Fentian);
    hanba->addSkill(new FentianRange);
	hanba->addSkill(new DetachEffectSkill("fentian", "burn"));
    related_skills.insertMulti("fentian", "#fentian");
	related_skills.insertMulti("fentian", "#fentian-clear");
    hanba->addSkill(new Zhiri);
    hanba->addRelateSkill("xintan");

    skills << new Xintan;
    addMetaObject<XintanCard>();
}

ADD_PACKAGE(Miscellaneous)
