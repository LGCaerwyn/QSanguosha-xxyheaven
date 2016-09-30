#include "legend.h"
#include "general.h"
#include "skill.h"
#include "card.h"
#include "standard.h"
#include "engine.h"
#include "clientplayer.h"
#include "maneuvering.h"

class dummyVS : public ZeroCardViewAsSkill
{
public:
    dummyVS() : ZeroCardViewAsSkill("dummy")
    {
    }

    virtual const Card *viewAs() const
    {
        return NULL;
    }
};

class NewShensuViewAsSkill : public ViewAsSkill
{
public:
    NewShensuViewAsSkill() : ViewAsSkill("newshensu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@newshensu");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("2"))
            return selected.isEmpty() && to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
        else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
		slash->setSkillName("newshensu");
		if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("2")) {
			if (cards.length() != 1) return NULL;
			slash->addCostcards(cards);
		}
		return slash;
    }
};

class NewShensu : public TriggerSkill
{
public:
    NewShensu() : TriggerSkill("newshensu")
    {
        events << EventPhaseChanging << CardExtraCost;
        view_as_skill = new NewShensuViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *xiahouyuan, QVariant &data) const
    {
		if (triggerEvent == EventPhaseChanging) {
			PhaseChangeStruct change = data.value<PhaseChangeStruct>();
			int index = 0;
			if (change.to == Player::Judge && !xiahouyuan->isSkipped(Player::Judge)
					&& !xiahouyuan->isSkipped(Player::Draw) && Slash::IsAvailable(xiahouyuan))
				index = 1;
			else if (Slash::IsAvailable(xiahouyuan) && change.to == Player::Play && !xiahouyuan->isSkipped(Player::Play)
					&& xiahouyuan->canDiscard(xiahouyuan, "he"))
				index = 2;
			else if (change.to == Player::Discard && !xiahouyuan->isSkipped(Player::Discard) && Slash::IsAvailable(xiahouyuan))
				index = 3;
			else
				return false;
			room->setPlayerMark(xiahouyuan, "ShensuIndex", index);
			room->setPlayerFlag(xiahouyuan, "slashNoDistanceLimit");
			if (!room->askForUseCard(xiahouyuan, "@@newshensu"+QString::number(index), "@newshensu"+QString::number(index), index))
				room->setPlayerFlag(xiahouyuan, "-slashNoDistanceLimit");
			room->setPlayerMark(xiahouyuan, "ShensuIndex", 0);
		} else if (triggerEvent == CardExtraCost) {
            if (!data.canConvert<CardUseStruct>()) return false;
			CardUseStruct use = data.value<CardUseStruct>();
			if (use.card->getSkillName() != objectName()) return false;
			switch (xiahouyuan->getMark("ShensuIndex")) {
			case 1: {
				xiahouyuan->skip(Player::Judge, true);
				xiahouyuan->skip(Player::Draw, true);
				break;
			}
			case 2: {
				QList<int> extracost = use.card->getCostcards();
				if (!extracost.isEmpty()) {
					DummyCard *dummy = new DummyCard(extracost);
					CardMoveReason reason(CardMoveReason::S_REASON_THROW, xiahouyuan->objectName(), QString(), "newshensu", QString());
					room->moveCardTo(dummy, xiahouyuan, NULL, Player::DiscardPile, reason, true);
					delete dummy;
				}
				xiahouyuan->skip(Player::Play, true);
				break;
			}
			case 3: {
				xiahouyuan->skip(Player::Discard, true);
				xiahouyuan->turnOver();
				break;
			}
			default:
				break;
			}
		}
        return false;
    }
};

class NewLiegong : public TriggerSkill
{
public:
    NewLiegong() : TriggerSkill("newliegong")
    {
        events << TargetSpecified << DamageCaused;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (triggerEvent == TargetSpecified) {
			CardUseStruct use = data.value<CardUseStruct>();
			if (player->getPhase() != Player::Play || !use.card->isKindOf("Slash"))
				return false;
			QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
			int index = 0;
			QStringList liegong_list;
            foreach (ServerPlayer *p, use.to) {
				int i = 0;
                if (player->getHandcardNum() <= p->getHandcardNum())
					i++;
                if (player->getHp() <= p->getHp())
					i++;
                if (player->getAttackRange() >= p->getAttackRange())
					i++;
				
				if (i > 0 && player->askForSkillInvoke(this, QVariant::fromValue(p))) {
                    player->broadcastSkillInvoke(objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());

					LogMessage log;
					log.type = "#NoJink";
					log.from = p;
					room->sendLog(log);
					jink_list.replace(index, QVariant(0));
				}
				if (i > 1)
					liegong_list << p->objectName();
				
				index++;
			}
			use.card->setTag("liegongAddDamage", QVariant::fromValue(liegong_list));
			player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
		} else if (triggerEvent == DamageCaused) {
			DamageStruct damage = data.value<DamageStruct>();
			if (damage.card == NULL) return false;
			ServerPlayer *target = damage.to;
			if (target == NULL || target->isDead()) return false;
			QStringList liegong_list = damage.card->tag["liegongAddDamage"].toStringList();
			if (liegong_list.contains(target->objectName()) && player->askForSkillInvoke(this, QVariant::fromValue(target))) {
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
				damage.damage++;
				data = QVariant::fromValue(damage);
			}
		}
        return false;
    }
};

class NewKuanggu : public TriggerSkill
{
public:
    NewKuanggu() : TriggerSkill("newkuanggu")
    {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!player->tag["NewKuangguDamage"].toList().last().toBool()) return false;
		for (int i = 0; i < damage.damage; i++) {
			if (player->isAlive() && player->askForSkillInvoke(this, data)) {
                player->broadcastSkillInvoke(objectName());
				if (player->isWounded() && room->askForChoice(player, objectName(), "recover+draw", data) == "recover")
					room->recover(player, RecoverStruct(player, NULL, 1));
				else
					player->drawCards(1);
			} else
				break;
        }
        return false;
    }
};

class NewKuangguRecord : public TriggerSkill
{
public:
    NewKuangguRecord() : TriggerSkill("#newkuanggu-record")
    {
        events << PreDamageDone << DamageComplete << EventPhaseChanging;
        global = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreDamageDone) {
			DamageStruct damage = data.value<DamageStruct>();
			ServerPlayer *weiyan = damage.from;
			if (weiyan == NULL) return false;
			QVariantList kuanggu_list = weiyan->tag["NewKuangguDamage"].toList();
			kuanggu_list << (weiyan->distanceTo(player) <= 1);
            weiyan->tag["NewKuangguDamage"] = QVariant::fromValue(kuanggu_list);
		} else if (triggerEvent == DamageComplete) {
			DamageStruct damage = data.value<DamageStruct>();
			if (damage.prevented) return false;
			ServerPlayer *weiyan = damage.from;
			if (weiyan == NULL) return false;
			QVariantList kuanggu_list = weiyan->tag["NewKuangguDamage"].toList();
            kuanggu_list.takeLast();
            weiyan->tag["NewKuangguDamage"] = QVariant::fromValue(kuanggu_list);
		} else if (triggerEvent == EventPhaseChanging) {
			if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
					p->tag.remove("NewKuangguDamage");
            }
		}
        return false;
    }
};

QimouCard::QimouCard()
{
    target_fixed = true;
}

void QimouCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
	ServerPlayer *weiyan = card_use.from;
	room->removePlayerMark(weiyan, "@strategy");
	QStringList cost_num;
    for (int i = 1; i < weiyan->getHp(); cost_num << QString::number(i++)) {}
    int num = room->askForChoice(weiyan, "qimou_losehp", cost_num.join("+")).toInt();
	room->loseHp(weiyan, num);
	room->setPlayerMark(weiyan, "#qimou", num);
}

class QimouViewAsSkill : public ZeroCardViewAsSkill
{
public:
    QimouViewAsSkill() : ZeroCardViewAsSkill("qimou")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@strategy") > 0 && player->getHp() > 0;
    }

    virtual const Card *viewAs() const
    {
        return new QimouCard;
    }
};

class Qimou : public TriggerSkill
{
public:
    Qimou() : TriggerSkill("qimou")
    {
        events << EventPhaseChanging;
        frequency = Limited;
        limit_mark = "@strategy";
		view_as_skill = new QimouViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *weiyan, QVariant &data) const
    {
        if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
            room->setPlayerMark(weiyan, "#qimou", 0);
        }
        return false;
    }
};

class QimouDistance : public DistanceSkill
{
public:
    QimouDistance() : DistanceSkill("#qimou-distance")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        return -from->getMark("#qimou");
    }
};

class QimouTargetMod : public TargetModSkill
{
public:
    QimouTargetMod() : TargetModSkill("#qimou-target")
    {
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        return from->getMark("#qimou");
    }
};

NewTianxiangCard::NewTianxiangCard()
{
}

void NewTianxiangCard::onEffect(const CardEffectStruct &effect) const
{
    DamageStruct damage = effect.from->tag.value("NewTianxiangDamage").value<DamageStruct>();
    damage.to = effect.to;
    damage.transfer = true;
    damage.transfer_from = effect.from->objectName();
    damage.transfer_reason = "newtianxiang";
    effect.from->tag["TransferDamage"] = QVariant::fromValue(damage);
}

class NewTianxiangViewAsSkill : public OneCardViewAsSkill
{
public:
    NewTianxiangViewAsSkill() : OneCardViewAsSkill("newtianxiang")
    {
        filter_pattern = ".|heart|.|hand!";
        response_pattern = "@@newtianxiang";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        NewTianxiangCard *tianxiangCard = new NewTianxiangCard;
        tianxiangCard->addSubcard(originalCard);
        return tianxiangCard;
    }
};

class NewTianxiang : public TriggerSkill
{
public:
    NewTianxiang() : TriggerSkill("newtianxiang")
    {
        events << DamageInflicted << DamageComplete << EventPhaseStart << Death << DamageCaused;
        view_as_skill = new NewTianxiangViewAsSkill;
    }

	int getPriority(TriggerEvent triggerEvent) const
    {
		if (triggerEvent == DamageComplete)
			return -2;

		return TriggerSkill::getPriority(triggerEvent);
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if ((triggerEvent == DamageInflicted || triggerEvent == DamageCaused) && player->getMark("#newtianxiang") > 0) {
			LogMessage log;
            log.type = "#TianxiangEffect";
            log.from = player;
            log.arg = (triggerEvent == DamageInflicted) ? "tianxiang_inflicted" : "tianxiang_caused";
            log.arg2 = objectName();
            room->sendLog(log);
			return true;
		}
		if (triggerEvent == DamageInflicted) {
			if (TriggerSkill::triggerable(player) && player->getMark("tianxiang_null") == 0 && player->canDiscard(player, "h")) {
				player->tag["NewTianxiangDamage"] = data;
				return room->askForUseCard(player, "@@newtianxiang", "@newtianxiang-card", -1, Card::MethodDiscard);
			}
		} else if (triggerEvent == DamageComplete) {
			DamageStruct damage = data.value<DamageStruct>();
			if (player->isAlive() && damage.transfer && damage.transfer_reason == "newtianxiang") {
                ServerPlayer *xiaoqiao = room->findPlayer(damage.transfer_from);
				if (xiaoqiao) {
					if (room->askForChoice(xiaoqiao, objectName(), "drawcards+addbuff") == "drawcards")
						player->drawCards(player->getLostHp(), objectName());
					else {
                        room->addPlayerTip(player, "#newtianxiang");
                        xiaoqiao->tag["TianxiangTarget"] = QVariant::fromValue(player);
                        room->addPlayerMark(xiaoqiao, "tianxiang_null");
					}
				}
			}
		} else if (triggerEvent == EventPhaseStart || triggerEvent == Death) {
            if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != player)
                    return false;
            } else if (player->getPhase() != Player::RoundStart) {
				return false;
			}
            room->setPlayerMark(player, "tianxiang_null", 0);
            ServerPlayer *target = player->tag["TianxiangTarget"].value<ServerPlayer *>();
			if (target) {
				room->removePlayerTip(target, "#newtianxiang");
				player->tag.remove("TianxiangTarget");
			}
        }
        return false;
    }
};

NewQiangxiCard::NewQiangxiCard()
{
}

bool NewQiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    int rangefix = 0;
    if (!subcards.isEmpty() && Self->getWeapon() && Self->getWeapon()->getId() == subcards.first()) {
        const Weapon *card = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += card->getRange() - Self->getAttackRange(false);;
    }

    return Self->inMyAttackRange(to_select, rangefix);
}

void NewQiangxiCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
	if (subcards.isEmpty()) {
		room->loseHp(card_use.from);
		room->setPlayerFlag(card_use.from, "NewQiangxiHp");
	} else {
		CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
        room->moveCardTo(this, NULL, Player::DiscardPile, reason, true);
		room->setPlayerFlag(card_use.from, "NewQiangxiWeapon");
	}
}

void NewQiangxiCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->getRoom()->damage(DamageStruct("newqiangxi", effect.from, effect.to));
}

class NewQiangxiViewAsSkill : public ViewAsSkill
{
public:
    NewQiangxiViewAsSkill() : ViewAsSkill("newqiangxi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
		if (player->hasFlag("NewQiangxiWeapon") && player->getHp() < 1) return false;
        return player->usedTimes("NewQiangxiCard") < 2;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select) && !Self->hasFlag("NewQiangxiWeapon");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty() && !Self->hasFlag("NewQiangxiHp") && Self->getHp() > 0)
            return new NewQiangxiCard;
        else if (cards.length() == 1) {
            NewQiangxiCard *card = new NewQiangxiCard;
            card->addSubcards(cards);
            return card;
        }
        return NULL;
    }
};

class NewQiangxi : public TriggerSkill
{
public:
    NewQiangxi() : TriggerSkill("newqiangxi")
    {
        events << EventPhaseChanging;
		view_as_skill = new NewQiangxiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *dianwei, QVariant &data) const
    {
        if (data.value<PhaseChangeStruct>().from == Player::Play) {
            room->setPlayerFlag(dianwei, "-NewQiangxiWeapon");
			room->setPlayerFlag(dianwei, "-NewQiangxiHp");
        }
        return false;
    }
};

NewLianhuanCard::NewLianhuanCard()
{
    
}

bool NewLianhuanCard::targetFixed() const
{
    return isEquipped();
}

bool NewLianhuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    IronChain *chain = new IronChain(Card::SuitToBeDecided, -1);
    chain->addSubcard(this);
	chain->setSkillName(getSkillName());
    return chain->targetFilter(targets, to_select, Self);
}

bool NewLianhuanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    IronChain *chain = new IronChain(Card::SuitToBeDecided, -1);
    chain->addSubcard(this);
	chain->setSkillName(getSkillName());
    return chain->targetsFeasible(targets, Self);
}

const Card *NewLianhuanCard::validate(CardUseStruct &cardUse) const
{
	if (cardUse.to.isEmpty())
		return this;
	IronChain *chain = new IronChain(Card::SuitToBeDecided, -1);
    chain->addSubcard(this);
    chain->setSkillName(getSkillName());
    return chain;
}

void NewLianhuanCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *pangtong = card_use.from;

    LogMessage log;
    log.type = "$RecastCardWithSkill";
    log.from = pangtong;
	log.arg = getSkillName();
    log.card_str = QString::number(card_use.card->getSubcards().first());
    room->sendLog(log);
    pangtong->broadcastSkillInvoke(getSkillName());

    CardMoveReason reason(CardMoveReason::S_REASON_RECAST, pangtong->objectName());
    reason.m_skillName = getSkillName();
    room->moveCardTo(this, pangtong, NULL, Player::DiscardPile, reason);

    pangtong->drawCards(1, "recast");
}

class NewLianhuanViewAsSkill : public OneCardViewAsSkill
{
public:
    NewLianhuanViewAsSkill() : OneCardViewAsSkill("newlianhuan")
    {
        filter_pattern = ".|club";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        NewLianhuanCard *skill_card = new NewLianhuanCard;
        skill_card->addSubcard(originalCard);
        return skill_card;
    }
};

class NewLianhuan : public TriggerSkill
{
public:
    NewLianhuan() : TriggerSkill("newlianhuan")
    {
        events << CardFinished;
		view_as_skill = new NewLianhuanViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *pangtong, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getSkillName() == objectName() && use.to.length() == 1)
            pangtong->drawCards(1);
        return false;
    }
};

NewNiepanCard::NewNiepanCard()
{
    target_fixed = true;
}

void NewNiepanCard::use(Room *room, ServerPlayer *pangtong, QList<ServerPlayer *> &) const
{
    room->removePlayerMark(pangtong, "@nirvana");

    pangtong->throwAllHandCardsAndEquips();
    QList<const Card *> tricks = pangtong->getJudgingArea();
    foreach (const Card *trick, tricks) {
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, pangtong->objectName());
        room->throwCard(trick, reason, NULL);
    }

    room->recover(pangtong, RecoverStruct(pangtong, NULL, 3 - pangtong->getHp()));
    pangtong->drawCards(3, objectName());

    if (pangtong->isChained())
        room->setPlayerProperty(pangtong, "chained", false);

    if (!pangtong->faceUp())
        pangtong->turnOver();
}

class NewNiepanViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NewNiepanViewAsSkill() : ZeroCardViewAsSkill("newniepan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@nirvana") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new NewNiepanCard;
    }
};

class NewNiepan : public TriggerSkill
{
public:
    NewNiepan() : TriggerSkill("newniepan")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@nirvana";
		view_as_skill = new NewNiepanViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@nirvana") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != pangtong)
            return false;

        if (pangtong->askForSkillInvoke(this, data)) {
            pangtong->broadcastSkillInvoke(objectName());

            NewNiepanCard *skill_card = new NewNiepanCard;
			QList<ServerPlayer *> targets;
            skill_card->use(room, pangtong, targets);
            delete skill_card;
        }

        return false;
    }
};

class NewLuanjiViewAsSkill : public ViewAsSkill
{
public:
    NewLuanjiViewAsSkill() : ViewAsSkill("newluanji")
    {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("newluanji") == 0xF) return false;
        return true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !to_select->isEquipped() && (Self->getMark("newluanji") & (1 << int(to_select->getSuit()))) == 0;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            ArcheryAttack *aa = new ArcheryAttack(Card::SuitToBeDecided, 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        } else
            return NULL;
    }
};

class NewLuanji : public TriggerSkill
{
public:
    NewLuanji() : TriggerSkill("newluanji")
    {
        events << PreCardUsed << CardResponded << EventPhaseChanging;
		view_as_skill = new NewLuanjiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (triggerEvent == PreCardUsed) {
			CardUseStruct use = data.value<CardUseStruct>();
			if (use.card->getSkillName() != objectName()) return false;
            foreach (int id, use.card->getSubcards()) {
                const Card *card = Sanguosha->getCard(id);
                if (player->getMark("newluanji") == 0xF) return false;
				int suitID = (1 << int(card->getSuit()));
				int mark = player->getMark("newluanji");
				if ((mark & suitID) == 0)
					room->setPlayerMark(player, "newluanji", mark | suitID);
            }
		} else if (triggerEvent == CardResponded) {
			CardResponseStruct resp = data.value<CardResponseStruct>();
			const Card *card_star = resp.m_card;
			if (!card_star->isKindOf("Jink")) return false;
			const Card *r_card = resp.m_data.value<const Card *>();
            if (r_card && r_card->getSkillName() == objectName() && player->isWounded() && player->askForSkillInvoke("luanji_draw", "prompt"))
				player->drawCards(1);
		} else if (triggerEvent == EventPhaseChanging) {
			if (data.value<PhaseChangeStruct>().to == Player::NotActive)
				room->setPlayerMark(player, "newluanji", 0);
		}
        return false;
    }
};

NewZaiqiCard::NewZaiqiCard()
{
}

bool NewZaiqiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    return targets.length() < Self->getMark("newzaiqi");
}

void NewZaiqiCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *menghuo = effect.from;
	Room *room = menghuo->getRoom();
	ServerPlayer *target = effect.to;
	if (menghuo && menghuo->isAlive() && menghuo->isWounded() && room->askForChoice(target, "newzaiqi", "recover+draw") == "recover")
		room->recover(menghuo, RecoverStruct(target, NULL, 1));
	else
		effect.to->drawCards(1);
}

class NewZaiqiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NewZaiqiViewAsSkill() : ZeroCardViewAsSkill("newzaiqi")
    {
        response_pattern = "@@newzaiqi";
    }

    virtual const Card *viewAs() const
    {
        return new NewZaiqiCard;
    }
};

class NewZaiqi : public PhaseChangeSkill
{
public:
    NewZaiqi() : PhaseChangeSkill("newzaiqi")
    {
		view_as_skill = new NewZaiqiViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
		Room *room = target->getRoom();
        if (target->getPhase() != Player::Discard || target->getMark("newzaiqi") == 0) return false;
        room->askForUseCard(target, "@@newzaiqi", "@newzaiqi-card:::" + QString::number(target->getMark("newzaiqi")));
		return false;
    }
};

class NewZaiqiRecord : public TriggerSkill
{
public:
    NewZaiqiRecord() : TriggerSkill("#newzaiqi-record")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        global = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *menghuo, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
			CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
			if (move.to_place == Player::DiscardPile && menghuo->getPhase() != Player::NotActive) {
				foreach (int card_id, move.card_ids) {
					if (Sanguosha->getCard(card_id)->isRed())
						room->addPlayerMark(menghuo, "newzaiqi");
				}
			}
		} else if (triggerEvent == EventPhaseChanging) {
			if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                room->setPlayerMark(menghuo, "newzaiqi", 0);
            }
		}
        return false;
    }
};

class NewLieren : public TriggerSkill
{
public:
    NewLieren() : TriggerSkill("newlieren")
    {
        events << TargetSpecified << Pindian;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhurong, QVariant &data) const
    {
		if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(zhurong)) {
			CardUseStruct use = data.value<CardUseStruct>();
			if (!use.card->isKindOf("Slash")) return false;
			foreach (ServerPlayer *target, use.to) {
				if (zhurong->isDead() || zhurong->isKongcheng()) break;
				if (target->isAlive() && !target->isKongcheng() && room->askForSkillInvoke(zhurong, objectName(), QVariant::fromValue(target))) {
                    zhurong->broadcastSkillInvoke(objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, zhurong->objectName(), target->objectName());
					if (zhurong->pindian(target, objectName(), NULL) && !target->isNude()) {
						int card_id = room->askForCardChosen(zhurong, target, "he", objectName());
						CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, zhurong->objectName());
						room->obtainCard(zhurong, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
					}
				}
			}
		} else if (triggerEvent == Pindian) {
			PindianStruct *pindian = data.value<PindianStruct *>();
			if (pindian->reason != objectName() || pindian->success) return false;
			const Card *to_obtain = pindian->to_card;
			if (zhurong && to_obtain && room->getCardPlace(to_obtain->getEffectiveId()) == Player::PlaceTable)
				zhurong->obtainCard(to_obtain);
		}
        return false;
    }
};

#include "thicket.h"
class NewYinghun: public Yinghun {
public:
    NewYinghun(): Yinghun() {
        setObjectName("newyinghun");
    }

    virtual int getYinghunNum(ServerPlayer *sunjian) const{
        if (sunjian->getEquips().length() >= sunjian->getHp())
			return sunjian->getMaxHp();
        return sunjian->getLostHp();
    }
};

NewDimengCard::NewDimengCard()
{
	will_sort = false;
}

bool NewDimengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self)
        return false;

    if (targets.isEmpty())
        return true;

    if (targets.length() == 1) {
        return qAbs(to_select->getHandcardNum() - targets.first()->getHandcardNum()) == subcardsLength();
    }

    return false;
}

bool NewDimengCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void NewDimengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
	ServerPlayer *first = targets.at(0);
    ServerPlayer *second = targets.at(1);
	
    CardMoveReason reason1(CardMoveReason::S_REASON_SECRETLY_PUT, first->objectName(), second->objectName(), "newdimeng", QString());
    CardMoveReason reason2(CardMoveReason::S_REASON_SECRETLY_PUT, second->objectName(), first->objectName(), "newdimeng", QString());

	QList<CardsMoveStruct> move_to_table;
	QList<int> all = first->handCards();
	foreach (int id, second->handCards())
        all << id;
	QList<ServerPlayer *> to_notify;
	to_notify << source << first << second;
	CardsMoveStruct move1(first->handCards(), NULL, Player::PlaceTable, reason1);
	CardsMoveStruct move2(second->handCards(), NULL, Player::PlaceTable, reason2);
	move_to_table.push_back(move2);
	move_to_table.push_back(move1);
	if (!move_to_table.isEmpty()) {
		QList<ServerPlayer *> other_players;
        foreach (ServerPlayer *p, room->getAllPlayers(true)) {
			if (!to_notify.contains(p))
				other_players.append(p);
		}
		LogMessage log1;
        log1.type = "$NewDimeng";
        log1.from = first;
        log1.card_str = IntList2StringList(first->handCards()).join("+");
        room->sendLog(log1, to_notify);
		LogMessage log2;
        log2.type = "$NewDimeng";
        log2.from = second;
        log2.card_str = IntList2StringList(second->handCards()).join("+");
        room->sendLog(log2, to_notify);
		if (!other_players.isEmpty()) {
			LogMessage log3;
			log3.type = "#NewDimengOthers";
			log3.from = first;
			log3.arg = QString::number(first->getHandcardNum());
			room->sendLog(log3, other_players);
			LogMessage log4;
			log4.type = "#NewDimengOthers";
			log4.from = second;
			log4.arg = QString::number(second->getHandcardNum());
			room->sendLog(log4, other_players);
		}
		room->moveCardsAtomic(move_to_table, false);
		room->fillAG(all, to_notify);
		while (!all.isEmpty()) {
			int card_id = room->askForAG(first, all, false, "newdimeng");
			all.removeOne(card_id);
			if (!other_players.isEmpty()) {
				LogMessage log6;
				log6.type = "#TakeNCards";
				log6.from = first;
				log6.arg = "1";
				room->sendLog(log6, other_players);
			}
			room->takeAG(first, card_id, true, to_notify);
			qSwap(first, second);
		}
		room->clearAG();
	}
}

class NewDimeng : public ViewAsSkill
{
public:
    NewDimeng() : ViewAsSkill("newdimeng")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        NewDimengCard *dimeng = new NewDimengCard;
        dimeng->addSubcards(cards);
        return dimeng;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("NewDimengCard");
    }
};

NewLegendPackage::NewLegendPackage()
: Package("new_legend")
{
	General *xiahouyuan = new General(this, "new_xiahouyuan", "wei");
	xiahouyuan->addSkill(new NewShensu);

	General *huangzhong = new General(this, "new_huangzhong", "shu");
	huangzhong->addSkill(new NewLiegong);

    General *weiyan = new General(this, "new_weiyan", "shu");
    weiyan->addSkill(new NewKuanggu);
    weiyan->addSkill(new NewKuangguRecord);
    related_skills.insertMulti("newkuanggu", "#newkuanggu-record");
	weiyan->addSkill(new Qimou);
	weiyan->addSkill(new QimouDistance);
	weiyan->addSkill(new QimouTargetMod);
	related_skills.insertMulti("qimou", "#qimou-distance");
	related_skills.insertMulti("qimou", "#qimou-target");

	General *xiaoqiao = new General(this, "new_xiaoqiao", "wu", 3, false);
    xiaoqiao->addSkill(new NewTianxiang);
	xiaoqiao->addSkill("hongyan");

	General *dianwei = new General(this, "new_dianwei", "wei", 4);
	dianwei->addSkill(new NewQiangxi);

	General *pangtong = new General(this, "new_pangtong", "shu", 3);
	pangtong->addSkill(new NewLianhuan);
	pangtong->addSkill(new NewNiepan);

	General *yuanshao = new General(this, "new_yuanshao$", "qun");
	yuanshao->addSkill(new NewLuanji);
	yuanshao->addSkill("xueyi");

    General *menghuo = new General(this, "new_menghuo", "shu", 4);
    menghuo->addSkill("huoshou");
    menghuo->addSkill(new NewZaiqi);
    menghuo->addSkill(new NewZaiqiRecord);
	related_skills.insertMulti("newzaiqi", "#newzaiqi-record");

    General *zhurong = new General(this, "new_zhurong", "shu", 4, false);
    zhurong->addSkill("juxiang");
	zhurong->addSkill(new NewLieren);

    General *sunjian = new General(this, "new_sunjian", "wu");
    sunjian->addSkill(new NewYinghun);

	General *lusu = new General(this, "new_lusu", "wu", 3);
    lusu->addSkill("haoshi");
	lusu->addSkill(new NewDimeng);

	addMetaObject<QimouCard>();
    addMetaObject<NewTianxiangCard>();
	addMetaObject<NewQiangxiCard>();
	addMetaObject<NewLianhuanCard>();
    addMetaObject<NewNiepanCard>();
	addMetaObject<NewZaiqiCard>();
	addMetaObject<NewDimengCard>();
}

ADD_PACKAGE(NewLegend)
