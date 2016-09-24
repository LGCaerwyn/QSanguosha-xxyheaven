#include "yjcm2014.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
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

DingpinCard::DingpinCard()
{
}

bool DingpinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->isWounded() && !to_select->hasFlag("dingpin");
}

void DingpinCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();

    JudgeStruct judge;
    judge.who = effect.to;
    judge.good = true;
    judge.pattern = ".|black";
    judge.reason = "dingpin";

    room->judge(judge);

    if (judge.isGood()) {
        room->setPlayerFlag(effect.to, "dingpin");
        effect.to->drawCards(effect.to->getLostHp(), "dingpin");
    } else {
        effect.from->turnOver();
    }
}

class DingpinViewAsSkill : public OneCardViewAsSkill
{
public:
    DingpinViewAsSkill() : OneCardViewAsSkill("dingpin")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("dingpin") == 0xE) return false;
        if (!player->hasFlag("dingpin") && player->isWounded()) return true;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (!p->hasFlag("dingpin") && p->isWounded()) return true;
        }
        return false;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return !to_select->isEquipped() && (Self->getMark("dingpin") & (1 << int(to_select->getTypeId()))) == 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        DingpinCard *card = new DingpinCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Dingpin : public TriggerSkill
{
public:
    Dingpin() : TriggerSkill("dingpin")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded << BeforeCardsMove;
        view_as_skill = new DingpinViewAsSkill;
        global = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("dingpin"))
                        room->setPlayerFlag(p, "-dingpin");
                }
                if (player->getMark("dingpin") > 0)
                    room->setPlayerMark(player, "dingpin", 0);
            }
        } else {
            if (!player->isAlive() || player->getPhase() == Player::NotActive) return false;
            if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
                const Card *card = NULL;
                if (triggerEvent == PreCardUsed) {
                    card = data.value<CardUseStruct>().card;
                } else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    if (resp.m_isUse)
                        card = resp.m_card;
                }
                if (!card || card->getTypeId() == Card::TypeSkill) return false;
                recordDingpinCardType(room, player, card);
            } else if (triggerEvent == BeforeCardsMove) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (player != move.from
                    || ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_DISCARD))
                    return false;
                foreach (int id, move.card_ids) {
                    const Card *c = Sanguosha->getCard(id);
                    recordDingpinCardType(room, player, c);
                }
            }
        }
        return false;
    }

private:
    void recordDingpinCardType(Room *room, ServerPlayer *player, const Card *card) const
    {
        if (player->getMark("dingpin") == 0xE) return;
        int typeID = (1 << int(card->getTypeId()));
        int mark = player->getMark("dingpin");
        if ((mark & typeID) == 0)
            room->setPlayerMark(player, "dingpin", mark | typeID);
    }
};

class Faen : public TriggerSkill
{
public:
    Faen() : TriggerSkill("faen")
    {
        events << TurnedOver << ChainStateChanged;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == ChainStateChanged && !player->isChained()) return false;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (!player->isAlive()) return false;
            if (TriggerSkill::triggerable(p)
                && room->askForSkillInvoke(p, objectName(), QVariant::fromValue(player))) {
                p->broadcastSkillInvoke(objectName());
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
                player->drawCards(1, objectName());
            }
        }
        return false;
    }
};

class SidiVS : public OneCardViewAsSkill
{
public:
    SidiVS() : OneCardViewAsSkill("sidi")
    {
        response_pattern = "@@sidi";
        filter_pattern = ".|.|.|sidi";
        expand_pile = "sidi";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class Sidi : public TriggerSkill
{
public:
    Sidi() : TriggerSkill("sidi")
    {
        events << CardResponded << EventPhaseStart << EventPhaseChanging;
        //frequency = Frequent;
        view_as_skill = new SidiVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                room->setPlayerMark(player, "sidi", 0);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse && resp.m_card->isKindOf("Jink")) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (TriggerSkill::triggerable(p) && (p == player || p->getPhase() != Player::NotActive)
                        && room->askForSkillInvoke(p, objectName(), data)) {
                        p->broadcastSkillInvoke(objectName(), 1);
                        QList<int> ids = room->getNCards(1, false); // For UI
                        CardsMoveStruct move(ids, p, Player::PlaceTable,
                            CardMoveReason(CardMoveReason::S_REASON_TURNOVER, p->objectName(), "sidi", QString()));
                        room->moveCardsAtomic(move, true);
                        p->addToPile("sidi", ids);
                    }
                }
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (player->getPhase() != Player::Play) return false;
                if (TriggerSkill::triggerable(p) && p->getPile("sidi").length() > 0) {
                    const Card *card = room->askForCard(p, "@@sidi", "@sidi-remove", data, Card::MethodNone);
					if (card) {
						LogMessage log;
                        log.type = "#InvokeSkill";
                        log.from = p;
                        log.arg = objectName();
                        room->sendLog(log);
                        room->notifySkillInvoked(p, objectName());
                        p->broadcastSkillInvoke(objectName(), 2);
					    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
						CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), p->objectName(), objectName(), QString());
                        room->throwCard(card, reason, NULL);
				        room->addPlayerMark(player, "sidi");
					}
				}
            }
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 2;
    }
};

class SidiTargetMod : public TargetModSkill
{
public:
    SidiTargetMod() : TargetModSkill("#sidi-target")
    {
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        return card->isKindOf("Slash") ? -from->getMark("sidi") : 0;
    }
};

class ShenduanViewAsSkill : public OneCardViewAsSkill
{
public:
    ShenduanViewAsSkill() : OneCardViewAsSkill("shenduan")
    {
        response_pattern = "@@shenduan";
		expand_pile = "#shenduan";
    }

    bool viewFilter(const Card *to_select) const
    {
        return Self->getPile("#shenduan").contains(to_select->getEffectiveId());
    }

    const Card *viewAs(const Card *originalCard) const
    {
        SupplyShortage *ss = new SupplyShortage(originalCard->getSuit(), originalCard->getNumber());
        ss->addSubcard(originalCard);
        ss->setSkillName("shenduan");
        return ss;
    }
};

class Shenduan : public TriggerSkill
{
public:
    Shenduan() : TriggerSkill("shenduan")
    {
        events << CardsMoveOneTime;
        view_as_skill = new ShenduanViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != player)
            return false;
        if (move.to_place == Player::DiscardPile
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)) {

            int i = 0;
            QList<int> shenduan_card;
            foreach (int card_id, move.card_ids) {
                const Card *c = Sanguosha->getCard(card_id);
                if ((move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)
                    && c->isBlack() && c->getTypeId() == Card::TypeBasic && room->getCardPlace(card_id) == Player::DiscardPile) {
                    shenduan_card << card_id;
                }
                i++;
            }
            if (shenduan_card.isEmpty())
                return false;

            do {
				room->notifyMoveToPile(player, shenduan_card, "shenduan", Player::PlaceTable, true, true);
                const Card *use = room->askForUseCard(player, "@@shenduan", "@shenduan-use");
				if (use == NULL){
				    room->notifyMoveToPile(player, shenduan_card, "shenduan", Player::PlaceTable, false, false);
				    break;
				}
                int card_id = use->getSubcards().first();
                shenduan_card.removeOne(card_id);
				QList<int> shenduan_card2;
				foreach (int id, shenduan_card) {
                    if (room->getCardPlace(id) == Player::DiscardPile)
                        shenduan_card2 << id;
                }
                shenduan_card = shenduan_card2;
            } while (!shenduan_card.isEmpty());
        }
        return false;
    }
};

class ShenduanUse : public TriggerSkill
{
public:
    ShenduanUse() : TriggerSkill("#shenduan")
    {
        events << PreCardUsed;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("SupplyShortage") && use.card->getSkillName() == "shenduan") {
            QList<int> ids = StringList2IntList(player->tag["shenduan_forAI"].toString().split("+"));
			room->notifyMoveToPile(player, ids, "shenduan", Player::PlaceTable, false, false);
        }
        return false;
    }
};

class ShenduanTargetMod : public TargetModSkill
{
public:
    ShenduanTargetMod() : TargetModSkill("#shenduan-target")
    {
        pattern = "SupplyShortage";
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "shenduan")
            return 1000;
        else
            return 0;
    }
};

class YonglveViewAsSkill : public OneCardViewAsSkill
{
public:
    YonglveViewAsSkill() : OneCardViewAsSkill("yonglve")
    {
        response_pattern = "@@yonglve";
		expand_pile = "#yonglve";
    }

    bool viewFilter(const Card *to_select) const
    {
        return Self->getPile("#yonglve").contains(to_select->getEffectiveId());
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class Yonglve : public PhaseChangeSkill
{
public:
    Yonglve() : PhaseChangeSkill("yonglve")
    {
		view_as_skill = new YonglveViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() != Player::Judge) return false;
        Room *room = target->getRoom();
        foreach (ServerPlayer *hs, room->getOtherPlayers(target)) {
            if (target->isDead() || target->getJudgingArea().isEmpty()) break;
            if (!TriggerSkill::triggerable(hs) || !hs->inMyAttackRange(target)) continue;
			QList<int> judge_card;
			foreach(const Card *card, target->getJudgingArea()){
			    judge_card << card->getEffectiveId();
			}
			if (judge_card.isEmpty())
                return false;
			room->notifyMoveToPile(hs, judge_card, "yonglve", Player::PlaceTable, true, true);
			const Card *card = room->askForCard(hs, "@@yonglve", "@yonglve-use:" + target->objectName(), QVariant(), Card::MethodNone);
            room->notifyMoveToPile(hs, judge_card, "yonglve", Player::PlaceTable, false, false);
			if (card) {
                hs->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(hs, objectName());
                LogMessage log;
                log.from = hs;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                room->sendLog(log);
				room->throwCard(card, NULL, hs);
				if (hs->isAlive() && target->isAlive() && hs->canSlash(target, false)) {
                    room->setTag("YonglveUser", QVariant::fromValue(hs));
                    Slash *slash = new Slash(Card::NoSuit, 0);
                    slash->setSkillName("_yonglve");
                    room->useCard(CardUseStruct(slash, hs, target));
                }
			}
        }
        return false;
    }
	
	virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return 0;
        else
            return -1;
    }
};

class YonglveSlash : public TriggerSkill
{
public:
    YonglveSlash() : TriggerSkill("#yonglve")
    {
        events << PreDamageDone << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == "yonglve")
                damage.card->setFlags("YonglveDamage");
        } else if (!player->hasFlag("Global_ProcessBroken")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.card->getSkillName() == "yonglve" && !use.card->hasFlag("YonglveDamage")) {
                ServerPlayer *hs = room->getTag("YonglveUser").value<ServerPlayer *>();
                if (hs)
                    hs->drawCards(1, "yonglve");
            }
        }
        return false;
    }
};

class Benxi : public TriggerSkill
{
public:
    Benxi() : TriggerSkill("benxi")
    {
        events << EventPhaseChanging << CardUsed;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, "#benxi", 0);
        } else if (triggerEvent == CardUsed && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill
                && player->isAlive() && player->getPhase() != Player::NotActive) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                room->addPlayerMark(player, "#benxi");
            }
        }
        return false;
    }
};

// the part of Armor ignorance is coupled in Player::hasArmorEffect

class BenxiTargetMod : public TargetModSkill
{
public:
    BenxiTargetMod() : TargetModSkill("#benxi-target")
    {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("benxi") && isAllAdjacent(from, card))
            return 1;
        else
            return 0;
    }

private:
    bool isAllAdjacent(const Player *from, const Card *card) const
    {
        int rangefix = 0;
        if (card->isVirtualCard() && from->getOffensiveHorse()
            && card->getSubcards().contains(from->getOffensiveHorse()->getEffectiveId()))
            rangefix = 1;
        foreach (const Player *p, from->getAliveSiblings()) {
            if (from->distanceTo(p, rangefix) != 1)
                return false;
        }
        return true;
    }
};

class BenxiDistance : public DistanceSkill
{
public:
    BenxiDistance() : DistanceSkill("#benxi-dist")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        return -from->getMark("#benxi");
    }
};

class Qiangzhi : public TriggerSkill
{
public:
    Qiangzhi() : TriggerSkill("qiangzhi")
    {
        events << EventPhaseStart << CardUsed << CardResponded;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPhase() == Player::Play;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            player->setMark(objectName(), 0);
            if (TriggerSkill::triggerable(player)) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->isKongcheng())
                        targets << p;
                }
                if (targets.isEmpty()) return false;
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "qiangzhi-invoke", true, true);
                if (target) {
                    player->broadcastSkillInvoke(objectName(), 1);
                    int id = room->askForCardChosen(player, target, "h", objectName());
                    room->showCard(target, id);
                    player->setMark(objectName(), int(Sanguosha->getCard(id)->getTypeId()));
                }
            }
        } else if (player->getMark(objectName()) > 0) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed) {
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    card = resp.m_card;
            }
            if (card && int(card->getTypeId()) == player->getMark(objectName())
                && room->askForSkillInvoke(player, objectName(), data)) {
                if (!player->hasSkill(this)) {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);
                }
                player->broadcastSkillInvoke(objectName(), 2);
                player->drawCards(1, objectName());
            }
        }
        return false;
    }
};

class Xiantu : public TriggerSkill
{
public:
    Xiantu() : TriggerSkill("xiantu")
    {
        events << EventPhaseStart << EventPhaseChanging << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!player->isAlive()) return false;
                if (TriggerSkill::triggerable(p) && room->askForSkillInvoke(p, objectName())) {
                    p->broadcastSkillInvoke(objectName(), 1);
                    p->setFlags("XiantuInvoked");
                    p->drawCards(2, objectName());
                    if (p->isAlive() && player->isAlive()) {
                        if (!p->isNude()) {
                            int num = qMin(2, p->getCardCount(true));
                            const Card *to_give = room->askForExchange(p, objectName(), num, num, true,
                                QString("@xiantu-give::%1:%2").arg(player->objectName()).arg(num));
                            player->obtainCard(to_give, false);
                            delete to_give;
                        }
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                QList<ServerPlayer *> zhangsongs;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("XiantuInvoked")) {
                        p->setFlags("-XiantuInvoked");
                        zhangsongs << p;
                    }
                }
                if (player->getMark("XiantuKill") > 0) {
                    player->setMark("XiantuKill", 0);
                    return false;
                }
                foreach (ServerPlayer *zs, zhangsongs) {
                    LogMessage log;
                    log.type = "#Xiantu";
                    log.from = player;
                    log.to << zs;
                    log.arg = objectName();
                    room->sendLog(log);
                    zs->broadcastSkillInvoke(objectName(), 2);
                    room->loseHp(zs);
                }
            }
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.damage && death.damage->from && death.damage->from->getPhase() == Player::Play)
                death.damage->from->addMark("XiantuKill");
        }
        return false;
    }
};

class Zhongyong : public TriggerSkill
{
public:
    Zhongyong() : TriggerSkill("zhongyong")
    {
        events << SlashMissed;
		view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        const Card *jink = effect.jink;
        if (!jink) return false;
        QList<int> ids;
        if (!jink->isVirtualCard()) {
            if (room->getCardPlace(jink->getEffectiveId()) == Player::DiscardPile)
                ids << jink->getEffectiveId();
        } else {
            foreach (int id, jink->getSubcards()) {
                if (room->getCardPlace(id) == Player::DiscardPile)
                    ids << id;
            }
        }
        if (ids.isEmpty()) return false;

        room->fillAG(ids, player);
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(effect.to), objectName(),
            "zhongyong-invoke:" + effect.to->objectName(), true, true);
        room->clearAG(player);
        if (!target) return false;
        player->broadcastSkillInvoke(objectName());
        DummyCard *dummy = new DummyCard(ids);
        room->obtainCard(target, dummy);
        delete dummy;

        if (player->isAlive() && effect.to->isAlive() && target != player) {
            if (!player->canSlash(effect.to, NULL, false))
                return false;
            if (room->askForUseSlashTo(player, effect.to, QString("zhongyong-slash:%1").arg(effect.to->objectName()), false, true))
                return true;
        }
        return false;
    }
};

ShenxingCard::ShenxingCard()
{
    target_fixed = true;
}

void ShenxingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    if (source->isAlive())
        room->drawCards(source, 1, "shenxing");
}

class Shenxing : public ViewAsSkill
{
public:
    Shenxing() : ViewAsSkill("shenxing")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        ShenxingCard *card = new ShenxingCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getCardCount(true) >= 2 && player->canDiscard(player, "he");
    }
};

BingyiAskCard::BingyiAskCard()
{
}

bool BingyiAskCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    return targets.length() < Self->getHandcardNum();
}

void BingyiAskCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    room->sortByActionOrder(use.to);
    foreach(ServerPlayer *p, use.to)
        room->drawCards(p, 1, "bingyi");
}

class BingyiAsk : public ZeroCardViewAsSkill
{
public:
    BingyiAsk() : ZeroCardViewAsSkill("bingyiask")
    {
        response_pattern = "@@bingyiask!";
    }

    virtual const Card *viewAs() const
    {
        return new BingyiAskCard;
    }
};

class Bingyi : public PhaseChangeSkill
{
public:
    Bingyi() : PhaseChangeSkill("bingyi")
    {
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
		Room *room = target->getRoom();
        if (target->getPhase() != Player::Finish || target->isKongcheng()) return false;
        if (room->askForSkillInvoke(target, objectName())){
            target->broadcastSkillInvoke(objectName());
			room->showAllCards(target);
			bool trigger_this = true;
			Card::Color color = Card::Colorless;
            foreach (const Card *c, target->getHandcards()) {
                if (color == Card::Colorless)
                    color = c->getColor();
                else if (c->getColor() != color){
                    trigger_this = false;
                    break;
				}
            }
			if (trigger_this)
				if (!room->askForUseCard(target, "@@bingyiask!", "@bingyi-card:::" + QString::number(target->getHandcardNum())))
					room->drawCards(target, 1, "bingyi");
		}
        return false;
    }
};

class Zenhui : public TriggerSkill
{
public:
    Zenhui() : TriggerSkill("zenhui")
    {
        events << TargetSpecifying << CardFinished;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == CardFinished && (use.card->isKindOf("Slash") || (use.card->isNDTrick() && use.card->isBlack()))) {
            use.from->setFlags("-ZenhuiUser_" + use.card->toString());
            return false;
        }
        if (!TriggerSkill::triggerable(player) || player->getPhase() != Player::Play || player->hasFlag(objectName()))
            return false;

        if (use.to.length() == 1 && !use.card->targetFixed()
            && (use.card->isKindOf("Slash") || (use.card->isNDTrick() && use.card->isBlack()))) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p != player && p != use.to.first() && !room->isProhibited(player, p, use.card) && use.card->targetFilter(QList<const Player *>(), p, player))
                    targets << p;
            }
            if (targets.isEmpty()) return false;
            use.from->tag["zenhui"] = data;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "zenhui-invoke:" + use.to.first()->objectName(), true, true);
            use.from->tag.remove("zenhui");
            if (target) {
                player->setFlags(objectName());

                // Collateral
                ServerPlayer *collateral_victim = NULL;
                if (use.card->isKindOf("Collateral")) {
                    QList<ServerPlayer *> victims;
                    foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
                        if (target->canSlash(p))
                            victims << p;
                    }
                    Q_ASSERT(!victims.isEmpty());
                    collateral_victim = room->askForPlayerChosen(player, victims, "zenhui_collateral", "@zenhui-collateral:" + target->objectName());
                    target->tag["collateralVictim"] = QVariant::fromValue((collateral_victim));

                    LogMessage log;
                    log.type = "#CollateralSlash";
                    log.from = player;
                    log.to << collateral_victim;
                    room->sendLog(log);
                }

                player->broadcastSkillInvoke(objectName());

                bool extra_target = true;
                if (!target->isNude()) {
                    const Card *card = room->askForCard(target, "..", "@zenhui-give:" + player->objectName(), data, Card::MethodNone);
                    if (card) {
                        extra_target = false;
                        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(),
                            player->objectName(), objectName(), QString());
                        reason.m_playerId = player->objectName();
                        room->moveCardTo(card, target, player, Player::PlaceHand, reason);

                        if (target->isAlive()) {
                            LogMessage log;
                            log.type = "#BecomeUser";
                            log.from = target;
                            log.card_str = use.card->toString();
                            room->sendLog(log);

                            target->setFlags("ZenhuiUser_" + use.card->toString()); // For AI
                            use.from = target;
                            data = QVariant::fromValue(use);
                        }
                    }
                }
                if (extra_target) {
                    LogMessage log;
                    log.type = "#BecomeTarget";
                    log.from = target;
                    log.card_str = use.card->toString();
                    room->sendLog(log);

                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
                    if (use.card->isKindOf("Collateral") && collateral_victim)
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), collateral_victim->objectName());

                    use.to.append(target);
                    room->sortByActionOrder(use.to);
                    data = QVariant::fromValue(use);
                }
            }
        }
        return false;
    }
};

class Jiaojin : public TriggerSkill
{
public:
    Jiaojin() : TriggerSkill("jiaojin")
    {
        events << DamageInflicted;
		view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isMale() && player->canDiscard(player, "he")) {
            if (room->askForCard(player, ".Equip", "@jiaojin", data, objectName())) {

                LogMessage log;
                log.type = "#Jiaojin";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(--damage.damage);
                room->sendLog(log);

                if (damage.damage < 1)
                    return true;
                data = QVariant::fromValue(damage);
            }
        }
        return false;
    }
};

class Youdi : public PhaseChangeSkill
{
public:
    Youdi() : PhaseChangeSkill("youdi")
    {
		view_as_skill = new dummyVS;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() != Player::Finish || target->isNude()) return false;
        Room *room = target->getRoom();
        QList<ServerPlayer *> players;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (p->canDiscard(target, "he")) players << p;
        }
        if (players.isEmpty()) return false;
        ServerPlayer *player = room->askForPlayerChosen(target, players, objectName(), "youdi-invoke", true, true);
        if (player) {
            target->broadcastSkillInvoke(objectName());
            int id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, target, player);
            if (!Sanguosha->getCard(id)->isKindOf("Slash") && player->isAlive() && !player->isNude()) {
                int id2 = room->askForCardChosen(target, player, "he", "youdi_obtain");
				CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, target->objectName());
                room->obtainCard(target, Sanguosha->getCard(id2),
                    reason, room->getCardPlace(id2) != Player::PlaceHand);
            }
        }
        return false;
    }
};

class Qieting : public PhaseChangeSkill
{
public:
    Qieting() : PhaseChangeSkill("qieting")
    {
        view_as_skill = new dummyVS;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 1;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPhase() == Player::NotActive && target->getMark("qieting") == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        foreach (ServerPlayer *caifuren, room->getAllPlayers()) {
            if (!TriggerSkill::triggerable(caifuren) || caifuren == player) continue;
            QStringList choices;
            for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
                if (player->getEquip(i) && !caifuren->getEquip(i))
                    choices << QString::number(i);
            }
            choices << "draw" << "cancel";
            QString choice = room->askForChoice(caifuren, objectName(), choices.join("+"), QVariant::fromValue(player));
            if (choice == "cancel") {
                continue;
            } else {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                log.from = caifuren;
                room->sendLog(log);
                room->notifySkillInvoked(caifuren, objectName());
                caifuren->broadcastSkillInvoke(objectName());
                if (choice == "draw") {
                    caifuren->drawCards(1, objectName());
                } else {
                    int index = choice.toInt();
                    const Card *card = player->getEquip(index);
                    room->moveCardTo(card, caifuren, Player::PlaceEquip);
                }
            }
        }
        return false;
    }
};

class QietingRecord : public TriggerSkill
{
public:
    QietingRecord() : TriggerSkill("#qieting-record")
    {
        events << PreCardUsed << TargetSpecified << TurnStart;
        global = true;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 6;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed && player->isAlive() && player->getPhase() != Player::NotActive) {
            CardUseStruct use = data.value<CardUseStruct>();
			if (use.card->getTypeId() != Card::TypeSkill)
                room->setCardFlag(use.card, "QietingRecord");
		} else if (triggerEvent == TargetSpecified) {
			CardUseStruct use = data.value<CardUseStruct>();
			if (use.card->hasFlag("QietingRecord")){
				ServerPlayer *current = room->getCurrent();
				if (!current || current->isDead() || current->getPhase() == Player::NotActive) return false;
				foreach (ServerPlayer *p, use.to) {
                    if (p != current) {
                        current->addMark("qieting");
                        return false;
                    }
                }
			}
        } else if (triggerEvent == TurnStart) {
            player->setMark("qieting", 0);
        }
        return false;
    }
};

XianzhouDamageCard::XianzhouDamageCard()
{
    mute = true;
}

void XianzhouDamageCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    foreach (ServerPlayer *p, room->getAlivePlayers()) {
		if (card_use.to.contains(p))
            room->damage(DamageStruct("xianzhou", card_use.from, p));
	}
}

bool XianzhouDamageCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < Self->getMark("xianzhou") && Self->inMyAttackRange(to_select);
}


XianzhouCard::XianzhouCard()
{
}

bool XianzhouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void XianzhouCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    room->removePlayerMark(card_use.from, "@handover");

    DummyCard *dummy = new DummyCard;
    foreach (const Card *c, card_use.from->getEquips())
        dummy->addSubcard(c);

	CardMoveReason reason(CardMoveReason::S_REASON_GIVE, card_use.from->objectName(), card_use.to.first()->objectName(), "xianzhou", QString());
    room->obtainCard(card_use.to.first(), dummy, reason, false);
	card_use.from->setMark("xianzhou_len", dummy->subcardsLength());
    delete dummy;
}

void XianzhouCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    int len = effect.from->getMark("xianzhou_len");
	effect.from->setMark("xianzhou_len", 0);
	room->setPlayerMark(effect.to, "xianzhou", len);

    if (!room->askForUseCard(effect.to, "@xianzhou", "@xianzhou-damage:" + effect.from->objectName() + "::" + QString::number(len)) && effect.from->isWounded())
        room->recover(effect.from, RecoverStruct(effect.to, NULL, len));
}

class Xianzhou : public ZeroCardViewAsSkill
{
public:
    Xianzhou() : ZeroCardViewAsSkill("xianzhou")
    {
        frequency = Skill::Limited;
        limit_mark = "@handover";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@handover") > 0 && player->getEquips().length() > 0;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@xianzhou";
    }

    virtual const Card *viewAs() const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@xianzhou") {
            return new XianzhouDamageCard;
        } else {
            return new XianzhouCard;
        }
    }
};

class Jianying : public TriggerSkill
{
public:
    Jianying() : TriggerSkill("jianying")
    {
        events << CardUsed << CardResponded << EventPhaseChanging;
        frequency = Frequent;
        global = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if ((triggerEvent == CardUsed || triggerEvent == CardResponded) && player->getPhase() == Player::Play) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else if (triggerEvent == CardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    card = resp.m_card;
            }
            if (!card || card->getTypeId() == Card::TypeSkill) return false;
            int suit = player->getMark("JianyingSuit"), number = player->getMark("JianyingNumber");
            player->setMark("JianyingSuit", int(card->getSuit()) > 3 ? 0 : (int(card->getSuit()) + 1));
            player->setMark("JianyingNumber", card->getNumber());
            if (player->isAlive() && player->hasSkill(this)
                && ((suit > 0 && int(card->getSuit()) + 1 == suit)
                || (number > 0 && card->getNumber() == number))
                && room->askForSkillInvoke(player, objectName(), data)) {
                player->broadcastSkillInvoke(objectName());
                room->drawCards(player, 1, objectName());
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().from == Player::Play) {
                player->setMark("JianyingSuit", 0);
                player->setMark("JianyingNumber", 0);
            }
        }
        return false;
    }
};

class Shibei : public MasochismSkill
{
public:
    Shibei() : MasochismSkill("shibei")
    {
        frequency = Compulsory;
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &) const
    {
        Room *room = player->getRoom();
		int shibei_index = player->tag["ShibieDamage"].toList().last().toInt();
        if (shibei_index != 0) {
            player->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());

            if (shibei_index > 0)
                room->recover(player, RecoverStruct(player));
            else
                room->loseHp(player);
        }
    }
};

class ShibeiRecord : public TriggerSkill
{
public:
    ShibeiRecord() : TriggerSkill("#shibei-record")
    {
        events << PreDamageDone << DamageComplete << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
		return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (triggerEvent == PreDamageDone) {
            QVariantList shibei_list = player->tag["ShibieDamage"].toList();
			if (!room->getCurrent())
				shibei_list << 0;
			else if (!player->hasFlag("ShibeiFirst")) {
				room->setPlayerFlag(player, "ShibeiFirst");
				shibei_list << 1;
			} else
				shibei_list << -1;
            player->tag["ShibieDamage"] = QVariant::fromValue(shibei_list);
			
		} else if (triggerEvent == DamageComplete) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.prevented) return false;
			QVariantList shibei_list = player->tag["ShibieDamage"].toList();
            shibei_list.takeLast();
            player->tag["ShibieDamage"] = QVariant::fromValue(shibei_list);
		} else if (triggerEvent == EventPhaseChanging) {
			if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
					p->tag.remove("ShibieDamage");
				    room->setPlayerFlag(p, "-ShibeiFirst");
				}
            }
		}
        return false;
    }
};

YJCM2014Package::YJCM2014Package()
    : Package("YJCM2014")
{
    General *caifuren = new General(this, "caifuren", "qun", 3, false); // YJ 301
    caifuren->addSkill(new Qieting);
    caifuren->addSkill(new QietingRecord);
    caifuren->addSkill(new Xianzhou);
    related_skills.insertMulti("qieting", "#qieting-record");

    General *caozhen = new General(this, "caozhen", "wei"); // YJ 302
    caozhen->addSkill(new Sidi);
    caozhen->addSkill(new SidiTargetMod);
    caozhen->addSkill(new DetachEffectSkill("sidi", "sidi"));
    related_skills.insertMulti("sidi", "#sidi-target");
	related_skills.insertMulti("sidi", "#sidi-clear");

    General *chenqun = new General(this, "chenqun", "wei", 3); // YJ 303
    chenqun->addSkill(new Dingpin);
    chenqun->addSkill(new Faen);

    General *guyong = new General(this, "guyong", "wu", 3); // YJ 304
    guyong->addSkill(new Shenxing);
    guyong->addSkill(new Bingyi);

    General *hanhaoshihuan = new General(this, "hanhaoshihuan", "wei"); // YJ 305
    hanhaoshihuan->addSkill(new Shenduan);
    hanhaoshihuan->addSkill(new ShenduanUse);
    hanhaoshihuan->addSkill(new ShenduanTargetMod);
    hanhaoshihuan->addSkill(new Yonglve);
    hanhaoshihuan->addSkill(new YonglveSlash);
    related_skills.insertMulti("shenduan", "#shenduan");
    related_skills.insertMulti("shenduan", "#shenduan-target");
    related_skills.insertMulti("yonglve", "#yonglve");

    General *jvshou = new General(this, "jvshou", "qun", 3); // YJ 306
    jvshou->addSkill(new Jianying);
    jvshou->addSkill(new Shibei);
    jvshou->addSkill(new ShibeiRecord);
    related_skills.insertMulti("shibei", "#shibei-record");

    General *sunluban = new General(this, "sunluban", "wu", 3, false); // YJ 307
    sunluban->addSkill(new Zenhui);
    sunluban->addSkill(new Jiaojin);

    General *wuyi = new General(this, "wuyi", "shu"); // YJ 308
    wuyi->addSkill(new Benxi);
    wuyi->addSkill(new BenxiTargetMod);
    wuyi->addSkill(new BenxiDistance);
    related_skills.insertMulti("benxi", "#benxi-target");
    related_skills.insertMulti("benxi", "#benxi-dist");

    General *zhangsong = new General(this, "zhangsong", "shu", 3); // YJ 309
    zhangsong->addSkill(new Qiangzhi);
    zhangsong->addSkill(new Xiantu);

    General *zhoucang = new General(this, "zhoucang", "shu"); // YJ 310
    zhoucang->addSkill(new Zhongyong);

    General *zhuhuan = new General(this, "zhuhuan", "wu"); // YJ 311
    zhuhuan->addSkill(new Youdi);

    addMetaObject<DingpinCard>();
    addMetaObject<ShenxingCard>();
    addMetaObject<BingyiAskCard>();
    addMetaObject<XianzhouCard>();
    addMetaObject<XianzhouDamageCard>();

    skills << new BingyiAsk;
}

ADD_PACKAGE(YJCM2014)
