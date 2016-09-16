#include "ol.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "yjcm2013.h"
#include "engine.h"
#include "clientplayer.h"
#include "json.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCommandLinkButton>
#include "settings.h"

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

BifaCard::BifaCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool BifaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->getPile("bifa").isEmpty() && to_select != Self;
}

void BifaCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    target->tag["BifaSource" + QString::number(getEffectiveId())] = QVariant::fromValue(source);
    target->addToPile("bifa", this, false);
}

class BifaViewAsSkill : public OneCardViewAsSkill
{
public:
    BifaViewAsSkill() : OneCardViewAsSkill("bifa")
    {
        filter_pattern = ".|.|.|hand";
        response_pattern = "@@bifa";
    }

    const Card *viewAs(const Card *originalcard) const
    {
        Card *card = new BifaCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class Bifa : public TriggerSkill
{
public:
    Bifa() : TriggerSkill("bifa")
    {
        events << EventPhaseStart;
        view_as_skill = new BifaViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish && !player->isKongcheng()) {
            room->askForUseCard(player, "@@bifa", "@bifa-remove", -1, Card::MethodNone);
        } else if (player->getPhase() == Player::RoundStart && player->getPile("bifa").length() > 0) {
            player->broadcastSkillInvoke(objectName(), 2);
			int card_id = player->getPile("bifa").first();
            ServerPlayer *chenlin = player->tag["BifaSource" + QString::number(card_id)].value<ServerPlayer *>();
            QList<int> ids;
            ids << card_id;

            LogMessage log;
            log.type = "$BifaView";
            log.from = player;
            log.card_str = QString::number(card_id);
            log.arg = "bifa";
            room->sendLog(log, player);

            room->fillAG(ids, player);
            const Card *cd = Sanguosha->getCard(card_id);
            QString pattern;
            if (cd->isKindOf("BasicCard"))
                pattern = "BasicCard";
            else if (cd->isKindOf("TrickCard"))
                pattern = "TrickCard";
            else if (cd->isKindOf("EquipCard"))
                pattern = "EquipCard";
            QVariant data_for_ai = QVariant::fromValue(pattern);
            pattern.append("|.|.|hand");
            const Card *to_give = NULL;
            if (!player->isKongcheng() && chenlin && chenlin->isAlive())
                to_give = room->askForCard(player, pattern, "@bifa-give", data_for_ai, Card::MethodNone, chenlin);
            if (chenlin && to_give) {
                CardMoveReason reasonG(CardMoveReason::S_REASON_GIVE, player->objectName(), chenlin->objectName(), "bifa", QString());
                room->obtainCard(chenlin, to_give, reasonG, false);
                CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, player->objectName(), "bifa", QString());
                room->obtainCard(player, cd, reason, false);
            } else {
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                room->throwCard(cd, reason, NULL);
                room->loseHp(player);
            }
            room->clearAG(player);
            player->tag.remove("BifaSource" + QString::number(card_id));
        }
        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return 1;
    }
};

SongciCard::SongciCard()
{
    mute = true;
}

bool SongciCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
	return targets.isEmpty() && to_select->getMark("songci" + Self->objectName()) == 0 && to_select->getHandcardNum() != to_select->getHp();
}

void SongciCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;

    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, card_use.from, data);
    use = data.value<CardUseStruct>();

	LogMessage log;
    log.from = card_use.from;
    log.to = card_use.to;
    log.type = "#UseCard";
    log.card_str = card_use.card->toString();
    room->sendLog(log);
	
	room->notifySkillInvoked(card_use.from, "songci");
	
	if (card_use.to.first()->getHandcardNum() > card_use.to.first()->getHp())
        card_use.from->broadcastSkillInvoke("songci", 2);
	else if (card_use.to.first()->getHandcardNum() < card_use.to.first()->getHp())
        card_use.from->broadcastSkillInvoke("songci", 1);

    thread->trigger(CardUsed, room, card_use.from, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, card_use.from, data);
}

void SongciCard::onEffect(const CardEffectStruct &effect) const
{
    int handcard_num = effect.to->getHandcardNum();
    int hp = effect.to->getHp();
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.to, "songci" + effect.from->objectName());
    if (handcard_num > hp)
        room->askForDiscard(effect.to, "songci", 2, 2, false, true);
    else if (handcard_num < hp)
        effect.to->drawCards(2, "songci");
}

class SongciViewAsSkill : public ZeroCardViewAsSkill
{
public:
    SongciViewAsSkill() : ZeroCardViewAsSkill("songci")
    {
    }

    const Card *viewAs() const
    {
        return new SongciCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("songci" + player->objectName()) == 0 && player->getHandcardNum() != player->getHp()) return true;
        foreach(const Player *sib, player->getAliveSiblings())
            if (sib->getMark("songci" + player->objectName()) == 0 && sib->getHandcardNum() != sib->getHp())
                return true;
        return false;
    }
};

class Songci : public TriggerSkill
{
public:
    Songci() : TriggerSkill("songci")
    {
        events << Death;
        view_as_skill = new SongciViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target && target->hasSkill(this);
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player) return false;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("songci" + player->objectName()) > 0)
                room->setPlayerMark(p, "songci" + player->objectName(), 0);
        }
        return false;
    }
};

class AocaiVeiw : public OneCardViewAsSkill
{
public:
    AocaiVeiw() : OneCardViewAsSkill("aocai_view")
    {
        expand_pile = "#aocai",
        response_pattern = "@@aocai_view";
    }

	bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QStringList aocai = Self->property("aocai").toString().split("+");
        foreach (QString id, aocai) {
            bool ok;
            if (id.toInt(&ok) == to_select->getEffectiveId() && ok)
                return true;
        }
        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class AocaiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    AocaiViewAsSkill() : ZeroCardViewAsSkill("aocai")
    {
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getPhase() != Player::NotActive || player->hasFlag("Global_AocaiFailed")) return false;
        if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        else if (pattern == "slash" || pattern == "jink" || pattern.contains("analeptic"))
            return true;
        return false;
    }

    const Card *viewAs() const
    {
        AocaiCard *aocai_card = new AocaiCard;
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
            pattern = "analeptic";
        aocai_card->setUserString(pattern);
        return aocai_card;
    }
};

class Aocai : public TriggerSkill
{
public:
    Aocai() : TriggerSkill("aocai")
    {
        events << CardAsked;
        view_as_skill = new AocaiViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &) const
    {
        
        return false;
    }

    static int view(Room *room, ServerPlayer *player, QList<int> &ids, QList<int> &enabled)
    {
        int result = -1, index = -1;
		LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = player;
        log.card_str = IntList2StringList(ids).join("+");
        room->sendLog(log, player);

        player->broadcastSkillInvoke("aocai");
        room->notifySkillInvoked(player, "aocai");
        
		room->notifyMoveToPile(player, ids, "aocai", Player::PlaceTable, true, true);
        room->setPlayerProperty(player, "aocai", IntList2StringList(enabled).join("+"));
		const Card *card = room->askForCard(player, "@@aocai_view", "@aocai-view", QVariant(), Card::MethodNone);
		room->notifyMoveToPile(player, ids, "aocai", Player::PlaceTable, false, false);
        if (card == NULL)
		    room->setPlayerFlag(player, "Global_AocaiFailed");
		else {
            result = card->getSubcards().first();
			index = ids.indexOf(result);
			LogMessage log;
            log.type = "#AocaiUse";
            log.from = player;
            log.arg = "aocai";
            log.arg2 = QString("CAPITAL(%1)").arg(index + 1);
            room->sendLog(log);
        }
		room->returnToTopDrawPile(ids);
        return result;
    }
};

AocaiCard::AocaiCard()
{
}

bool AocaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool AocaiCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFixed();
}

bool AocaiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetsFeasible(targets, Self);
}

const Card *AocaiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled;
    foreach (int id, ids)
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "aocai";
    room->sendLog(log);

    int id = Aocai::view(room, user, ids, enabled);
    return Sanguosha->getCard(id);
}

const Card *AocaiCard::validate(CardUseStruct &cardUse) const
{
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();

	LogMessage log;
    log.from = user;
    log.to = cardUse.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);
	
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled;
    foreach (int id, ids)
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;

    int id = Aocai::view(room, user, ids, enabled);
	const Card *card = Sanguosha->getCard(id);
	if (card)
		room->setCardFlag(card, "hasSpecialEffects");
	return card;
}

DuwuCard::DuwuCard()
{
}

bool DuwuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return (targets.isEmpty() && qMax(0, to_select->getHp()) == subcardsLength() && Self->inMyAttackRange(to_select));
}

void DuwuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    room->damage(DamageStruct("duwu", effect.from, effect.to));
}

class DuwuViewAsSkill : public ViewAsSkill
{
public:
    DuwuViewAsSkill() : ViewAsSkill("duwu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasFlag("DuwuEnterDying");
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        DuwuCard *duwu = new DuwuCard;
        if (!cards.isEmpty())
            duwu->addSubcards(cards);
        return duwu;
    }
};

class Duwu : public TriggerSkill
{
public:
    Duwu() : TriggerSkill("duwu")
    {
        events << QuitDying;
        view_as_skill = new DuwuViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.damage && dying.damage->getReason() == "duwu" && !dying.damage->chain && !dying.damage->transfer) {
            ServerPlayer *from = dying.damage->from;
            if (from && from->isAlive()) {
                room->setPlayerFlag(from, "DuwuEnterDying");
                from->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(from, objectName());
                room->loseHp(from, 1);
            }
        }
        return false;
    }
};

ZhoufuCard::ZhoufuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool ZhoufuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && to_select->getPile("incantation").isEmpty();
}

void ZhoufuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    target->tag["ZhoufuSource" + QString::number(getEffectiveId())] = QVariant::fromValue(source);
    target->addToPile("incantation", this);
}

class ZhoufuViewAsSkill : public OneCardViewAsSkill
{
public:
    ZhoufuViewAsSkill() : OneCardViewAsSkill("zhoufu")
    {
        filter_pattern = ".|.|.|hand";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ZhoufuCard");
    }

    const Card *viewAs(const Card *originalcard) const
    {
        Card *card = new ZhoufuCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class Zhoufu : public TriggerSkill
{
public:
    Zhoufu() : TriggerSkill("zhoufu")
    {
        events << StartJudge << EventPhaseChanging;
        view_as_skill = new ZhoufuViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPile("incantation").length() > 0;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == StartJudge) {
            int card_id = player->getPile("incantation").first();

            JudgeStruct *judge = data.value<JudgeStruct *>();
            judge->card = Sanguosha->getCard(card_id);

            LogMessage log;
            log.type = "$ZhoufuJudge";
            log.from = player;
            log.arg = objectName();
            log.card_str = QString::number(judge->card->getEffectiveId());
            room->sendLog(log);

            room->moveCardTo(judge->card, NULL, judge->who, Player::PlaceJudge,
                CardMoveReason(CardMoveReason::S_REASON_JUDGE,
                judge->who->objectName(),
                QString(), QString(), judge->reason), true);
            judge->updateResult();
            room->setTag("SkipGameRule", true);
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                int id = player->getPile("incantation").first();
                ServerPlayer *zhangbao = player->tag["ZhoufuSource" + QString::number(id)].value<ServerPlayer *>();
                if (zhangbao && zhangbao->isAlive()) {
                    zhangbao->broadcastSkillInvoke(objectName());
					zhangbao->obtainCard(Sanguosha->getCard(id));
				}
            }
        }
        return false;
    }
};

class Yingbing : public TriggerSkill
{
public:
    Yingbing() : TriggerSkill("yingbing")
    {
        events << StartJudge;
        frequency = Frequent;
    }

    int getPriority(TriggerEvent) const
    {
        return -1;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        int id = judge->card->getEffectiveId();
        ServerPlayer *zhangbao = player->tag["ZhoufuSource" + QString::number(id)].value<ServerPlayer *>();
        if (zhangbao && TriggerSkill::triggerable(zhangbao)
            && zhangbao->askForSkillInvoke(this, data)) {
            zhangbao->broadcastSkillInvoke(objectName());
            zhangbao->drawCards(2, "yingbing");
        }
        return false;
    }
};

ShefuCard::ShefuCard()
{
    will_throw = false;
    target_fixed = true;
}

void ShefuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QString mark = "Shefu_" + user_string;
    room->setPlayerMark(source, mark, getEffectiveId() + 1);
    source->addToPile("ambush", this, false);
}

class ShefuViewAsSkill : public OneCardViewAsSkill
{
public:
    ShefuViewAsSkill() : OneCardViewAsSkill("shefu")
    {
        filter_pattern = ".|.|.|hand";
        response_pattern = "@@shefu";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        ShefuCard *card = new ShefuCard;
        card->addSubcard(originalCard);
        card->setSkillName("shefu");
        return card;
    }
};

class Shefu : public PhaseChangeSkill
{
public:
    Shefu() : PhaseChangeSkill("shefu")
    {
        view_as_skill = new ShefuViewAsSkill;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (target->getPhase() != Player::Finish || target->isKongcheng())
            return false;
        room->askForUseCard(target, "@@shefu", "@shefu-prompt", -1, Card::MethodNone);
        return false;
    }

    QString getSelectBox() const
    {
        return "guhuo_sbtd";
    }

    bool buttonEnabled(const QString &button_name) const
    {
        return Self->getMark("Shefu_" + button_name) == 0;
    }
};

class ShefuCancel : public TriggerSkill
{
public:
    ShefuCancel() : TriggerSkill("#shefu-cancel")
    {
        events << CardUsed << JinkEffect;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == JinkEffect) {
            bool invoked = false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (ShefuTriggerable(p, player, "jink")) {
                    room->setTag("ShefuData", data);
                    if (!room->askForSkillInvoke(p, "shefu_cancel", "data:::jink") || p->getMark("Shefu_jink") == 0)
                        continue;

                    p->broadcastSkillInvoke("shefu");

                    invoked = true;

                    LogMessage log;
                    log.type = "#ShefuEffect";
                    log.from = p;
                    log.to << player;
                    log.arg = "jink";
                    log.arg2 = "shefu";
                    room->sendLog(log);

                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "shefu", QString());
                    int id = p->getMark("Shefu_jink") - 1;
                    room->setPlayerMark(p, "Shefu_jink", 0);
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);
                }
            }
            return invoked;
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeBasic && use.card->getTypeId() != Card::TypeTrick)
                return false;
            QString card_name = use.card->objectName();
            if (card_name.contains("slash")) card_name = "slash";
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (ShefuTriggerable(p, player, card_name)) {
                    room->setTag("ShefuData", data);
                    if (!room->askForSkillInvoke(p, "shefu_cancel", "data:::" + card_name) || p->getMark("Shefu_" + card_name) == 0)
                        continue;

                    p->broadcastSkillInvoke("shefu");

                    LogMessage log;
                    log.type = "#ShefuEffect";
                    log.from = p;
                    log.to << player;
                    log.arg = card_name;
                    log.arg2 = "shefu";
                    room->sendLog(log);

                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "shefu", QString());
                    int id = p->getMark("Shefu_" + card_name) - 1;
                    room->setPlayerMark(p, "Shefu_" + card_name, 0);
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);

                    use.nullified_list << "_ALL_TARGETS";
                }
            }
            data = QVariant::fromValue(use);
        }
        return false;
    }

private:
    bool ShefuTriggerable(ServerPlayer *chengyu, ServerPlayer *user, QString card_name) const
    {
        return chengyu->getPhase() == Player::NotActive && chengyu != user && chengyu->hasSkill("shefu")
            && !chengyu->getPile("ambush").isEmpty() && chengyu->getMark("Shefu_" + card_name) > 0;
    }
};

class BenyuViewAsSkill : public ViewAsSkill
{
public:
    BenyuViewAsSkill() : ViewAsSkill("benyu")
    {
        response_pattern = "@@benyu";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() < Self->getMark("benyu"))
            return NULL;

        DummyCard *card = new DummyCard;
        card->addSubcards(cards);
        return card;
    }
};

class Benyu : public MasochismSkill
{
public:
    Benyu() : MasochismSkill("benyu")
    {
        view_as_skill = new BenyuViewAsSkill;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        if (!damage.from || damage.from->isDead())
            return;
        Room *room = target->getRoom();
        int from_handcard_num = damage.from->getHandcardNum(), handcard_num = target->getHandcardNum();
        if (handcard_num == from_handcard_num) {
            return;
        } else if (handcard_num < from_handcard_num && handcard_num < 5 && room->askForSkillInvoke(target, objectName(), QVariant::fromValue(damage.from))) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), damage.from->objectName());
            target->broadcastSkillInvoke(objectName());
            room->drawCards(target, qMin(5, from_handcard_num) - handcard_num, objectName());
        } else if (handcard_num > from_handcard_num) {
            room->setPlayerMark(target, objectName(), from_handcard_num + 1);
            //if (room->askForUseCard(target, "@@benyu", QString("@benyu-discard::%1:%2").arg(damage.from->objectName()).arg(from_handcard_num + 1), -1, Card::MethodDiscard)) 
            if (room->askForCard(target, "@@benyu", QString("@benyu-discard::%1:%2").arg(damage.from->objectName()).arg(from_handcard_num + 1), QVariant(), objectName(), damage.from))
                room->damage(DamageStruct(objectName(), target, damage.from));
        }
        return;
    }
};

class Canshi : public TriggerSkill
{
public:
    Canshi() : TriggerSkill("canshi")
    {
        events << EventPhaseStart << CardUsed << CardResponded;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw) {
                int n = 0;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->isWounded())
                        ++n;
                }

                if (n > 0 && player->askForSkillInvoke(this, "prompt:::" + QString::number(n))) {
                    player->broadcastSkillInvoke(objectName());
                    player->setFlags(objectName());
                    player->drawCards(n, objectName());
                    return true;
                }
            }
        } else {
            if (player->hasFlag(objectName())) {
                const Card *card = NULL;
                if (triggerEvent == CardUsed)
                    card = data.value<CardUseStruct>().card;
                else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    if (resp.m_isUse)
                        card = resp.m_card;
                }
                if (card != NULL && (card->isKindOf("BasicCard") || card->isKindOf("TrickCard"))) {
                    if (!room->askForDiscard(player, objectName(), 1, 1, false, true, "@canshi-discard")) {
                        QList<const Card *> cards = player->getCards("he");
                        const Card *c = cards.at(qrand() % cards.length());
                        room->throwCard(c, player);
                    }
                }
            }
        }
        return false;
    }
};

class Chouhai : public TriggerSkill
{
public:
    Chouhai() : TriggerSkill("chouhai")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->isKongcheng()) {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());

            DamageStruct damage = data.value<DamageStruct>();
            ++damage.damage;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class Guiming : public TriggerSkill // play audio effect only. This skill is coupled in Player::isWounded().
{
public:
    Guiming() : TriggerSkill("guiming$")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->hasLordSkill(this) && target->getPhase() == Player::RoundStart;
    }

    int getPriority(TriggerEvent) const
    {
        return 6;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        foreach (const ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getKingdom() == "wu" && p->isWounded() && p->getHp() == p->getMaxHp()) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                return false;
            }
        }

        return false;
    }
};

class Biluan : public PhaseChangeSkill
{
public:
    Biluan() : PhaseChangeSkill("biluan")
    {
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (target->getPhase() != Player::Draw)
			return false;

		foreach(ServerPlayer *p, room->getOtherPlayers(target)){
			if (p->distanceTo(target) == 1){
				if (target->askForSkillInvoke(this)) {
                    target->broadcastSkillInvoke(objectName());
                    QSet<QString> kingdom_set;
                    foreach(ServerPlayer *p, room->getAlivePlayers())
                        kingdom_set << p->getKingdom();

                    int n = kingdom_set.size();
					room->addPlayerMark(target, "#shixie_distance", n);
                    return true;
                }
				break;
			}
		}
        return false;
    }
};

class Lixia : public PhaseChangeSkill
{
public:
    Lixia() : PhaseChangeSkill("lixia")
    {
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (target->getPhase() != Player::Finish)
			return false;
		ServerPlayer *shixie = room->findPlayerBySkillName(objectName());
        if (shixie && shixie != target && !target->inMyAttackRange(shixie)) {
            room->sendCompulsoryTriggerLog(shixie, objectName());
            shixie->broadcastSkillInvoke(objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, shixie->objectName(), target->objectName());
			ServerPlayer *to_draw = shixie;
            if (room->askForChoice(shixie, objectName(), "self+other", QVariant(), "@lixia-choose::"+target->objectName()) == "other")
			    to_draw = target;
			to_draw->drawCards(1, objectName());
			room->setPlayerMark(shixie, "#shixie_distance", shixie->getMark("#shixie_distance")-1);
		}
        return false;
    }
};

class ShixieDistance : public DistanceSkill
{
public:
    ShixieDistance() : DistanceSkill("#shixie-distance")
    {
    }

    virtual int getCorrect(const Player *, const Player *to) const
    {
        return to->getMark("#shixie_distance");
    }
};

class Yishe : public TriggerSkill
{
public:
    Yishe() : TriggerSkill("yishe")
    {
        events << EventPhaseStart << CardsMoveOneTime;
	}

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() != Player::Finish || !player->getPile("rice").isEmpty())
				return false;
            if (!player->askForSkillInvoke(this))
                return false;
            player->broadcastSkillInvoke(objectName());
            player->drawCards(2);
			const Card *card = room->askForExchange(player, objectName(), 2, 2, true, "YishePush");
			player->addToPile("rice", card);
			delete card;
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_places.contains(Player::PlaceSpecial)
                && move.from_pile_names.contains("rice")) {
                if (player->getPile("rice").isEmpty() && player->isWounded()){
                    room->sendCompulsoryTriggerLog(player, objectName());
                    player->broadcastSkillInvoke(objectName());
					room->recover(player, RecoverStruct(player));
				}
            }
        }
        return false;
    }
};

class Bushi : public TriggerSkill
{
public:
    Bushi() : TriggerSkill("bushi")
    {
        events << Damage << Damaged;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
		for (int i = 0; i < damage.damage; i++) {
		    if (player->getPile("rice").isEmpty() || !target->isAlive() || !room->askForSkillInvoke(target, "bushi_obtain", "prompt:" + player->objectName()))
			    break;
			QString log_type = "#InvokeOthersSkill";
            if (target == player)
			    log_type = "#InvokeSkill";
			LogMessage log;
            log.type = log_type;
            log.from = target;
            log.to << player;
            log.arg = objectName();
            room->sendLog(log);
		    room->notifySkillInvoked(player, objectName());
            player->broadcastSkillInvoke(objectName());
			QList<int> ids = player->getPile("rice");
            room->fillAG(ids, target);
            int id = room->askForAG(target, ids, false, objectName());
            room->clearAG(target);
			CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, target->objectName(), objectName(), QString());
            room->obtainCard(target, Sanguosha->getCard(id), reason, true);
        }
        return false;
    }
};

class MidaoVS : public OneCardViewAsSkill
{
public:
    MidaoVS() : OneCardViewAsSkill("midao")
    {
        expand_pile = "rice";
        filter_pattern = ".|.|.|rice";
        response_pattern = "@@midao";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class Midao : public TriggerSkill
{
public:
    Midao() : TriggerSkill("midao")
    {
        events << AskForRetrial;
		view_as_skill = new MidaoVS;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPile("rice").isEmpty())
            return false;

        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@midao-card" << judge->who->objectName()
            << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, "@@midao", prompt, data, Card::MethodResponse, judge->who, true);
        if (card) {
			if (judge->who)
			    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), judge->who->objectName());
            player->broadcastSkillInvoke(objectName());
			room->notifySkillInvoked(player, objectName());
            room->retrial(card, player, judge, objectName());
        }
        return false;
    }
};

class FengpoRecord : public TriggerSkill
{
public:
    FengpoRecord() : TriggerSkill("#fengpo-record")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded;
        global = true;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::Play)
                room->setPlayerFlag(player, "-fengporec");
        } else {
            if (player->getPhase() != Player::Play || player->hasFlag("fengporec"))
                return false;

            const Card *c = NULL;
            if (triggerEvent == PreCardUsed)
                c = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    c = resp.m_card;
            }

            if (c && (c->isKindOf("Slash") || c->isKindOf("Duel"))){
				room->setCardFlag(c, "fengporecc");
                room->setPlayerFlag(player, "fengporec");
			}
            return false;
        }

        return false;
    }
};

class Fengpo : public TriggerSkill
{
public:
    Fengpo() : TriggerSkill("fengpo")
    {
        events << TargetSpecified << ConfirmDamage << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (event == TargetSpecified) {
            if (!TriggerSkill::triggerable(player))
				return false;
			CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.length() != 1) return false;
			ServerPlayer *target = use.to.first();
            if (use.card == NULL || (!use.card->hasFlag("fengporecc"))) return false;
            int n = 0;
            foreach (const Card *card, target->getHandcards())
                if (card->getSuit() == Card::Diamond)
                    ++n;
			if (!room->askForSkillInvoke(player, objectName(), QVariant::fromValue(target)))
				return false;
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
            player->broadcastSkillInvoke(objectName());
            QString choice = room->askForChoice(player, objectName(), "drawCards+addDamage");
            if (choice == "drawCards"){
				if (n > 0)
				    player->drawCards(n);
			}else if (choice == "addDamage")
				use.card->setTag("FengpoAddDamage", n);
        } else if (event == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL)
                return false;
			int n = damage.card->tag["FengpoAddDamage"].toInt();
            damage.damage = damage.damage+n;
            data = QVariant::fromValue(damage);
        } else if (event == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.card->setTag("FengpoAddDamage", 0);
        }
        return false;
    }
};

class Ranshang : public TriggerSkill
{
public:
    Ranshang() : TriggerSkill("ranshang")
    {
        events << EventPhaseStart << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish) {
            if (player->getMark("#kindle") > 0){
				room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
				room->loseHp(player, player->getMark("#kindle"));
			}
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
			if (damage.nature == DamageStruct::Fire) {
				room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
				player->gainMark("#kindle", damage.damage);
			}
        }
        return false;
    }
};

class Hanyong : public TriggerSkill
{
public:
    Hanyong() : TriggerSkill("hanyong")
    {
        events << CardUsed << ConfirmDamage;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (triggerEvent == CardUsed){
			if (!TriggerSkill::triggerable(player))
				return false;
			CardUseStruct use = data.value<CardUseStruct>();
		    if (player->getHp() >= room->getTag("Global_TurnCount").toInt())
			    return false;
            if (use.card->isKindOf("SavageAssault") || use.card->isKindOf("ArcheryAttack")) {
                if (player->askForSkillInvoke(objectName())){
                    player->broadcastSkillInvoke(objectName());
					room->setCardFlag(use.card, "HanyongEffect");
				}
			}
		} else if (triggerEvent == ConfirmDamage){
			DamageStruct damage = data.value<DamageStruct>();
            if (!damage.card || !damage.card->hasFlag("HanyongEffect"))
                return false;
            damage.damage++;
			data = QVariant::fromValue(damage);
		}
        return false;
    }
};

class Yawang : public TriggerSkill
{
public:
    Yawang() : TriggerSkill("yawang")
    {
        events << EventPhaseStart << EventPhaseChanging;
		frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
				int n = 0;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getHp() == player->getHp())
                        ++n;
                }
                player->setFlags(objectName());
				if (n > 0)
                    player->drawCards(n, objectName());
				room->setPlayerMark(player, "#yawang", n);
                return true;
            }
		} else if (triggerEvent == EventPhaseChanging) {
			PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
			room->setPlayerMark(player, "#yawang", 0);
        }
        return false;
    }
};

class YawangLimitation : public TriggerSkill
{
public:
    YawangLimitation() : TriggerSkill("#yawang")
    {
        events << EventPhaseStart << EventPhaseEnd << CardUsed << CardResponded;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == EventPhaseEnd)
            return -6;
        else
			return 6;
        return TriggerSkill::getPriority(triggerEvent);
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (player->getPhase() != Player::Play || !player->hasFlag("yawang"))
			return false;
        if (triggerEvent == EventPhaseStart) {
            if (player->getMark("#yawang") < 1){
				room->setPlayerCardLimitation(player, "use", ".|.|.|.", true);
				room->setPlayerFlag(player, "YawangLimitation");
			}
		} else if (triggerEvent == EventPhaseEnd) {
			if (player->hasFlag("YawangLimitation")){
				room->setPlayerFlag(player, "-YawangLimitation");
				room->removePlayerCardLimitation(player, "use", ".|.|.|.$1");
			}
		} else {
			const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    card = resp.m_card;
            }
            if (card != NULL && card->getTypeId() != Card::TypeSkill) {
                room->removePlayerMark(player, "#yawang");
			    if (player->getMark("#yawang") < 1){
					room->setPlayerCardLimitation(player, "use", ".|.|.|.", true);
					room->setPlayerFlag(player, "YawangLimitation");
				}
		    }
        }
        return false;
    }
};

class Xunzhi : public PhaseChangeSkill
{
public:
    Xunzhi() : PhaseChangeSkill("xunzhi")
    {
    }

    virtual bool onPhaseChange(ServerPlayer *cuiyan) const
    {
        Room *room = cuiyan->getRoom();

        if (cuiyan->getPhase() != Player::Start)
			return false;
        if (cuiyan->getNextAlive()->getHp() == cuiyan->getHp())
			return false;

        foreach (ServerPlayer *p, room->getOtherPlayers(cuiyan)) {
            if (p->getNextAlive() == cuiyan && p->getHp() == cuiyan->getHp())
			    return false;
        }

        if (cuiyan->askForSkillInvoke(objectName())) {
            cuiyan->broadcastSkillInvoke(objectName());
            room->loseHp(cuiyan);
            room->addPlayerMark(cuiyan, "#xunzhi", 2);
        }

        return false;
    }
};

class XunzhiKeep : public MaxCardsSkill
{
public:
    XunzhiKeep() : MaxCardsSkill("#xunzhi")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        return target->getMark("#xunzhi");
    }
};

LuanzhanCard::LuanzhanCard()
{
}

bool LuanzhanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QStringList available_targets = Self->property("luanzhan_available_targets").toString().split("+");

    return targets.length() < Self->getMark("#luanzhan") && available_targets.contains(to_select->objectName());
}

void LuanzhanCard::use(Room *, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
	foreach (ServerPlayer *p, targets)
	    p->setFlags("LuanzhanExtraTarget");
}

class LuanzhanViewAsSkill : public ZeroCardViewAsSkill
{
public:
    LuanzhanViewAsSkill() : ZeroCardViewAsSkill("luanzhan")
    {
		response_pattern = "@@luanzhan";
    }

    virtual const Card *viewAs() const
    {
        return new LuanzhanCard;
    }
};

class LuanzhanColl : public ZeroCardViewAsSkill
{
public:
    LuanzhanColl() : ZeroCardViewAsSkill("luanzhan_coll")
    {
		response_pattern = "@@luanzhan_coll";
    }

    virtual const Card *viewAs() const
    {
        return new ExtraCollateralCard;
    }
};

class Luanzhan : public TriggerSkill
{
public:
    Luanzhan() : TriggerSkill("luanzhan")
    {
        events << HpChanged << TargetChosen << TargetSpecified;
		view_as_skill = new LuanzhanViewAsSkill;
    }

    bool triggerable(const ServerPlayer *player) const
    {
        return player != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == HpChanged && !data.isNull() && data.canConvert<DamageStruct>()) {
			DamageStruct damage = data.value<DamageStruct>();
			if (damage.from && TriggerSkill::triggerable(damage.from)){
				room->addPlayerMark(damage.from, "#luanzhan");
			}
		} else if (triggerEvent == TargetSpecified && player->getMark("#luanzhan") > 0 && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
			if ((use.card->isKindOf("Slash") || (use.card->isNDTrick() && use.card->isBlack())) && use.to.length() < player->getMark("#luanzhan")) {
				room->setPlayerMark(player, "#luanzhan", 0);
			}
		} else if (triggerEvent == TargetChosen && player->getMark("#luanzhan") > 0 && TriggerSkill::triggerable(player)) {
			CardUseStruct use = data.value<CardUseStruct>();
			if (!use.card->isKindOf("Slash") && !(use.card->isNDTrick() && use.card->isBlack())) return false;
			bool no_distance_limit = false;
			if (use.card->isKindOf("Slash")) {
				if (use.card->hasFlag("slashDisableExtraTarget")) return false;
				if (use.card->hasFlag("slashNoDistanceLimit")){
				    no_distance_limit = true;
				    room->setPlayerFlag(player, "slashNoDistanceLimit");
			    }
			}
			QStringList available_targets;
			QList<ServerPlayer *> extra_targets;
			if (!use.card->isKindOf("AOE") && !use.card->isKindOf("GlobalEffect")) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (use.to.contains(p) || room->isProhibited(player, p, use.card)) continue;
                    if (use.card->targetFixed()) {
                        if (!use.card->isKindOf("Peach") || p->isWounded())
							available_targets.append(p->objectName());
                    } else {
                        if (use.card->targetFilter(QList<const Player *>(), p, player))
							available_targets.append(p->objectName());
                    }
                }
            }
			if (no_distance_limit)
				room->setPlayerFlag(player, "-slashNoDistanceLimit");
			if (!available_targets.isEmpty()) {
				if (use.card->isKindOf("Collateral")) {
					QStringList tos;
                    foreach(ServerPlayer *t, use.to)
                        tos.append(t->objectName());
					for (int i = player->getMark("#luanzhan"); i > 0; i--) {
						if (available_targets.isEmpty())
							break;
						room->setPlayerProperty(player, "extra_collateral", use.card->toString());
                        room->setPlayerProperty(player, "extra_collateral_current_targets", tos.join("+"));
						player->tag["luanzhan-use"] = data;
                        room->askForUseCard(player, "@@luanzhan_coll", "@luanzhan-add:::collateral:" + QString::number(i));
						player->tag.remove("luanzhan-use");
                        room->setPlayerProperty(player, "extra_collateral", QString());
                        room->setPlayerProperty(player, "extra_collateral_current_targets", QString());
						ServerPlayer *extra = NULL;
						foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                            if (p->hasFlag("ExtraCollateralTarget")) {
                                p->setFlags("-ExtraCollateralTarget");
                                extra = p;
                                break;
                            }
                        }
                        if (extra == NULL)
							break;
						extra->setFlags("LuanzhanExtraTarget");
						tos.append(extra->objectName());
						available_targets.removeOne(extra->objectName());
					}
				} else {
					room->setPlayerProperty(player, "luanzhan_available_targets", available_targets.join("+"));
					player->tag["luanzhan-use"] = data;
					room->askForUseCard(player, "@@luanzhan", "@luanzhan-add:::" + use.card->objectName() + ":" + QString::number(player->getMark("#luanzhan")));
				    player->tag.remove("luanzhan-use");
					room->setPlayerProperty(player, "luanzhan_available_targets", QString());
				}
				foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("LuanzhanExtraTarget")) {
                        p->setFlags("-LuanzhanExtraTarget");
						extra_targets << p;
                    }
                }
			}

			if (!extra_targets.isEmpty()) {
				if (use.card->isKindOf("Collateral")) {
					LogMessage log;
                    log.type = "#UseCard";
                    log.from = player;
                    log.to = extra_targets;
                    log.card_str = "@LuanzhanCard[no_suit:-]=.";
                    room->sendLog(log);
                    player->broadcastSkillInvoke("luanzhan");
					foreach (ServerPlayer *p, extra_targets) {
						ServerPlayer *victim = p->tag["collateralVictim"].value<ServerPlayer *>();
                        if (victim) {
                            LogMessage log;
                            log.type = "#LuanzhanCollateralSlash";
                            log.from = p;
                            log.to << victim;
                            room->sendLog(log);
                            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), victim->objectName());
                        }
					}
				}

				foreach (ServerPlayer *extra, extra_targets){
					use.to.append(extra);
				}
				room->sortByActionOrder(use.to);
		        data = QVariant::fromValue(use); 
			}
		}
        return false;
    }
};

class Zhidao : public TriggerSkill
{
public:
    Zhidao() : TriggerSkill("zhidao")
    {
        events << Damage;
		frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (!player->hasFlag("ZhidaoInvoked") && !target->isAllNude() && player->getPhase() == Player::Play) {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
            
            room->setPlayerFlag(player, "ZhidaoInvoked");
            int n = 0;
            if (!target->isKongcheng()) n++;
            if (!target->getEquips().isEmpty()) n++;
            if (!target->getJudgingArea().isEmpty()) n++;
            QList<int> ids = room->askForCardsChosen(player, target, "hej", objectName(), n, n);
			DummyCard *dummy = new DummyCard(ids);
			CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            room->obtainCard(player, dummy, reason, false);
			delete dummy;
        }

        return false;
    }
};

class ZhidaoProhibit : public ProhibitSkill
{
public:
    ZhidaoProhibit() : ProhibitSkill("#zhidao")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        if (card->getTypeId() == Card::TypeSkill)
            return false;

        if (from->hasFlag("ZhidaoInvoked"))
            return from != to;

        return false;
    }
};

class Jili : public TriggerSkill
{
public:
    Jili() : TriggerSkill("jili")
    {
        events << TargetConfirming;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if ((use.card->getTypeId() == Card::TypeBasic || use.card->isNDTrick()) && use.card->isRed()) {
            foreach (ServerPlayer *ybh, room->getOtherPlayers(target)) {
				if (TriggerSkill::triggerable(ybh) && use.from != ybh && !use.to.contains(ybh) && target->distanceTo(ybh) == 1){
                    ybh->broadcastSkillInvoke(objectName(), isGoodEffect(use.card, ybh) ? 2 : 1);
					room->notifySkillInvoked(ybh, objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, ybh->objectName(), target->objectName());
					LogMessage log;
                    log.type = "#JiliAdd";
                    log.from = ybh;
                    log.to << target;
                    log.card_str = use.card->toString();
                    log.arg = objectName();
                    room->sendLog(log);
					use.to.append(ybh);
				}
			}
			room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);
        }
        return false;
    }

private:
    static bool isGoodEffect(const Card *card, ServerPlayer *yanbaihu)
    {
        return card->isKindOf("Peach") || card->isKindOf("Analeptic") || card->isKindOf("ExNihilo")
                || card->isKindOf("AmazingGrace") || card->isKindOf("GodSalvation")
                || (card->isKindOf("IronChain") && yanbaihu->isChained());
    }
};

class Zhengnan : public TriggerSkill
{
public:
    Zhengnan() : TriggerSkill("zhengnan")
    {
        events << BuryVictim;
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
		foreach (ServerPlayer *guansuo, room->getAlivePlayers()) {
            if (TriggerSkill::triggerable(guansuo) && guansuo->askForSkillInvoke(this)){
                guansuo->broadcastSkillInvoke(objectName());
				guansuo->drawCards(3, objectName());
				QStringList skill_list;
				if (!guansuo->hasSkill("wusheng", true))
                    skill_list << "wusheng";
                if (!guansuo->hasSkill("dangxian", true))
                    skill_list << "dangxian";
				if (!guansuo->hasSkill("zhiman", true))
                    skill_list << "zhiman";
				if (!skill_list.isEmpty())
                    room->acquireSkill(guansuo, room->askForChoice(guansuo, objectName(), skill_list.join("+"), QVariant(), "@zhengnan-choose"));
			}
        }
        return false;
    }
};

class Xiefang : public DistanceSkill
{
public:
    Xiefang() : DistanceSkill("xiefang")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill(this)){
			int extra = 0;
			if (from->isFemale())
				extra ++;
            QList<const Player *> players = from->getAliveSiblings();
            foreach (const Player *p, players) {
                if (p->isFemale())
                    extra ++;
            }
			return -extra;
		} else
            return 0;
    }
};

class Zhaohuo : public TriggerSkill
{
public:
    Zhaohuo() : TriggerSkill("zhaohuo")
    {
        events << Dying;
		frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *taoqian, QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who != taoqian && taoqian->getMaxHp() > 1) {
			room->sendCompulsoryTriggerLog(taoqian, objectName());
            taoqian->broadcastSkillInvoke(objectName());
            room->loseMaxHp(taoqian, taoqian->getMaxHp() - 1);
        }
        return false;
    }
};

class Yibing : public TriggerSkill
{
public:
    Yibing() : TriggerSkill("yibing")
    {
        events << TargetConfirmed << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player)) {
			CardUseStruct use = data.value<CardUseStruct>();
			if (use.card->getTypeId() == Card::TypeSkill) return false;
			if (!use.to.contains(player)) return false;
            if (use.from && use.from->getHp() > player->getHp() && !player->hasFlag("Yibing1Used")) {
				room->setPlayerFlag(player, "Yibing1Used");
				if (player->askForSkillInvoke(this, "prompt1")) {
                    player->broadcastSkillInvoke(objectName());
					QList<int> basics;
                    foreach (int card_id, room->getDrawPile()) {
                        const Card *card = Sanguosha->getCard(card_id);
						if (card->getTypeId() == Card::TypeBasic) {
							bool will_append = true;
							foreach (const Card *c, player->getHandcards()) {
								if ((c->isKindOf("Slash") && card->isKindOf("Slash")) || c->objectName() == card->objectName()) {
									will_append = false;
									break;
								}
							}
							if (will_append)
							    basics << card_id;
						}
					}

					if (basics.isEmpty()){
                        LogMessage log;
                        log.type = "$TaoqianSearchFailed";
                        log.from = player;
						log.arg = "basic";
                        room->sendLog(log);
					} else {
						int index = qrand() % basics.length();
					    int id = basics.at(index);
					    player->obtainCard(Sanguosha->getCard(id), false);
					}
                }
			}
			if (use.from && use.from->getHp() > player->getHp() && !player->hasFlag("Yibing2Used")) {
				room->setPlayerFlag(player, "Yibing2Used");
                int n = use.from->getHp() - player->getHp();
				if (player->askForSkillInvoke(this, "prompt2:::" + QString::number(n))) {
                    player->broadcastSkillInvoke(objectName());
                    player->drawCards(n);
				}
			}
		} else if (triggerEvent == EventPhaseChanging) {
			PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("Yibing1Used"))
                        room->setPlayerFlag(p, "-Yibing1Used");
					if (p->hasFlag("Yibing2Used"))
                        room->setPlayerFlag(p, "-Yibing2Used");
                }
            }
		}
        return false;
    }
};

QingshouCard::QingshouCard()
{
	handling_method = Card::MethodNone;
}

bool QingshouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() == 0 && to_select->getMaxHp() > Self->getMaxHp();
}

void QingshouCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    DummyCard *dummy_card = new DummyCard;
    foreach (const Card *c, card_use.from->getHandcards())
        if (c->getTypeId() != Card::TypeBasic)
		    dummy_card->addSubcard(c);
	foreach (const Card *c, card_use.from->getEquips())
        dummy_card->addSubcard(c);

	CardMoveReason reason(CardMoveReason::S_REASON_GIVE, card_use.from->objectName(), card_use.to.first()->objectName(), "qingshou", QString());
    room->obtainCard(card_use.to.first(), dummy_card, reason, false);

	QSet<Card::CardType> types;
    foreach(int card_id, dummy_card->getSubcards())
        types << Sanguosha->getCard(card_id)->getTypeId();

    card_use.from->setMark("qingshou", types.size());

    delete dummy_card;
}

void QingshouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();

    LogMessage log;
    log.type = "#GainMaxHp";
    log.from = source;
    log.arg = QString::number(target->getMaxHp() - source->getMaxHp());
    log.arg2 = QString::number(target->getMaxHp());
    room->sendLog(log);

	room->setPlayerProperty(source, "maxhp", target->getMaxHp());

	if (source->getMark("qingshou") > 0) {
		room->recover(source, RecoverStruct(source, NULL, source->getMark("qingshou")));
		source->setMark("qingshou", 0);
	}

}

class Qingshou : public ZeroCardViewAsSkill
{
public:
    Qingshou() : ZeroCardViewAsSkill("qingshou")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
		if (player->hasUsed("QingshouCard")) return false;

		bool has_target = false;
		foreach(const Player *sib, player->getAliveSiblings())
            if (sib->getMaxHp() > player->getHp()){
				has_target = true;
				break;
			}
        if (!has_target) return false;

		foreach (const Card *card, player->getHandcards()) {
            if (card->getTypeId() != Card::TypeBasic)
                return true;
        }
		return player->hasEquip();
    }

    virtual const Card *viewAs() const
    {
        return new QingshouCard;
    }
};

class Zuyin : public TriggerSkill
{
public:
    Zuyin() : TriggerSkill("zuyin")
    {
        events << TargetConfirmed;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
            if (use.from == player)
				return false;
			foreach (ServerPlayer *to, use.to) {
                if (!player->isAlive()) break;
				if (to != use.from && player->distanceTo(to) <= 1 && TriggerSkill::triggerable(player)) {
                    player->tag["KangkaiSlash"] = data;
                    bool will_use = room->askForSkillInvoke(player, objectName(), QVariant::fromValue(to));
                    player->tag.remove("KangkaiSlash");
                    if (!will_use) continue;
                    player->broadcastSkillInvoke(objectName());
					QList<int> notbasics;
                    foreach (int card_id, room->getDrawPile())
						if (Sanguosha->getCard(card_id)->getTypeId() != Card::TypeBasic)
							notbasics << card_id;
					if (notbasics.isEmpty()){
                        LogMessage log;
                        log.type = "$SearchFailed";
                        log.from = player;
						log.arg = "notbasic";
                        room->sendLog(log);
						break;
					}
					int id = notbasics.at(qrand() % notbasics.length());
					int index = room->getDrawPile().indexOf(id);
					QList<int> ids;
					ids << id;
					CardsMoveStruct move(ids, player, Player::PlaceTable,
                        CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString()));
                    room->moveCardsAtomic(move, true);
                    if (Sanguosha->getCard(id)->getSuit() == use.card->getSuit()){
						use.nullified_list << to->objectName();
                        data = QVariant::fromValue(use);
						room->returnCardToDrawPile(id, index);
					} else {
						CardMoveReason reason(CardMoveReason::S_REASON_DRAW, player->objectName(), objectName(), QString());
                        room->obtainCard(player, Sanguosha->getCard(id), reason);
					}
				}
            }
        }
        return false;
    }
};

TianzuoDiscardCard::TianzuoDiscardCard()
{
	mute = true;
}

bool TianzuoDiscardCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return Self->canDiscard(to_select, "ej") && targets.length() < Self->getMark("tianzuonum");
}

void TianzuoDiscardCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QList<ServerPlayer *> recovers;
    foreach (ServerPlayer *target, targets) {
        if (!source->canDiscard(target, "ej")) continue;
        int to_throw = room->askForCardChosen(source, target, "ej", "tianzuo", false, Card::MethodDiscard);
		if (room->getCardPlace(to_throw) == Player::PlaceEquip)
            recovers << target;
        room->throwCard(to_throw, target, source);
    }
    foreach(ServerPlayer *p, recovers){
		room->recover(p, RecoverStruct(source));
	}
}

class TianzuoDiscard : public ZeroCardViewAsSkill
{
public:
    TianzuoDiscard() : ZeroCardViewAsSkill("tianzuo_discard")
    {
        response_pattern = "@@tianzuo_discard";
    }

    virtual const Card *viewAs() const
    {
        return new TianzuoDiscardCard;
    }
};

class Tianzuo : public PhaseChangeSkill
{
public:
    Tianzuo() : PhaseChangeSkill("tianzuo")
    {
		frequency = Compulsory;
    }

    virtual bool onPhaseChange(ServerPlayer *caojie) const
    {
        Room *room = caojie->getRoom();
        if (caojie->getPhase() != Player::Start || caojie->getHandcardNum() <= caojie->getHp())
            return false;
        room->sendCompulsoryTriggerLog(caojie, objectName());
        caojie->broadcastSkillInvoke(objectName());
		DummyCard *dummy = new DummyCard;
        foreach (const Card *card, caojie->getCards("he")) {
            if (card->getTypeId() != Card::TypeBasic)
                dummy->addSubcard(card);
        }
        if (dummy->subcardsLength() > 0){
			room->throwCard(dummy, caojie);
			QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getAllPlayers())
                if (!p->getCards("ej").isEmpty())
                   targets << p;
            int num = qMin(targets.length(), dummy->subcardsLength());
			if (num > 0) {
				room->setPlayerMark(caojie, "tianzuonum", num);
                room->askForUseCard(caojie, "@@tianzuo_discard", "@tianzuo-discard:::" + QString::number(num));
				room->setPlayerMark(caojie, "tianzuonum", 0);
			}
		}
        return false;
    }
};

GusheCard::GusheCard()
{
}

bool GusheCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < 3 && !to_select->isKongcheng() && to_select != Self;
}

void GusheCard::use(Room *, ServerPlayer *wangsitu, QList<ServerPlayer *> &targets) const
{
    wangsitu->multiPindian(targets, "gushe");
}

class GusheViewAsSkill : public ZeroCardViewAsSkill
{
public:
    GusheViewAsSkill() : ZeroCardViewAsSkill("gushe")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GusheCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new GusheCard;
    }
};

class Gushe : public TriggerSkill
{
public:
    Gushe() : TriggerSkill("gushe")
    {
        events << Pindian;
		view_as_skill = new GusheViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *wangsitu, QVariant &data) const
    {
        PindianStruct * pindian = data.value<PindianStruct *>();
		if (pindian->reason != "gushe") return false;
		QList<ServerPlayer *> losers;
		if (pindian->success) {
			losers << pindian->to;
		} else {
			losers << wangsitu;
			if (pindian->from_number == pindian->to_number)
                losers << pindian->to;
			wangsitu->gainMark("#rap");
			if (wangsitu->getMark("#rap") >= 7)
				room->killPlayer(wangsitu);
		}
		foreach (ServerPlayer *loser, losers) {
			if (loser->isDead()) continue;
			if (!room->askForDiscard(loser, "gushe", 1, 1, wangsitu->isAlive(), true, "@gushe-discard:" + wangsitu->objectName())) {
				wangsitu->drawCards(1);
			}
		}
        return false;
    }
};

class Jici : public TriggerSkill
{
public:
    Jici() : TriggerSkill("jici")
    {
        events << PindianVerifying;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *wangsitu, QVariant &data) const
    {
        PindianStruct * pindian = data.value<PindianStruct *>();
		if (pindian->reason != "gushe") return false;
		int n = wangsitu->getMark("#rap");
		if (pindian->from_number == n && wangsitu->askForSkillInvoke(objectName(), "prompt1")) {
            wangsitu->broadcastSkillInvoke(objectName());
			LogMessage log;
			log.type = "$ResetSkill";
			log.from = wangsitu;
			log.arg = "gushe";
			room->sendLog(log);
			room->addPlayerHistory(wangsitu, "GusheCard", 0);
		} else if (pindian->from_number < n && wangsitu->askForSkillInvoke(objectName(), "prompt2:::" + QString::number(n))) {
            wangsitu->broadcastSkillInvoke(objectName());
			LogMessage log;
			log.type = "$JiciAdd";
			log.from = wangsitu;
			pindian->from_number = qMin(pindian->from_number + n, 13);
			log.arg = QString::number(n);
			log.arg2 = QString::number(pindian->from_number);
			room->sendLog(log);
		}
        return false;
    }
};

class Tuifeng : public TriggerSkill
{
public:
    Tuifeng() : TriggerSkill("tuifeng")
    {
        events << Damaged << EventPhaseStart << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damaged && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            for (int i = 0; i < damage.damage; i++) {
                if (player->isNude()) break;
                const Card *card = room->askForCard(player, "..", "@tuifeng-put", data, Card::MethodNone);
                if (card) {
                    player->broadcastSkillInvoke(objectName());
                    room->notifySkillInvoked(player, objectName());
                    LogMessage log;
                    log.from = player;
                    log.type = "#InvokeSkill";
                    log.arg = objectName();
                    room->sendLog(log);
                    player->addToPile("tuifeng", card);
                } else break;
            }
        } else if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)) {
            if (player->getPhase() != Player::Start || player->getPile("tuifeng").isEmpty()) return false;
            player->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            int x = player->getPile("tuifeng").length();
            player->clearOnePrivatePile("tuifeng");
            player->drawCards(2*x);
            room->setPlayerMark(player, "#tuifeng", x);
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to != Player::NotActive) return false;
            room->setPlayerMark(player, "#tuifeng", 0);
        }
        return false;
    }
};

class TuifengTargetMod : public TargetModSkill
{
public:
    TuifengTargetMod() : TargetModSkill("#tuifeng-target")
    {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        return from->getMark("#tuifeng");
    }
};

ZiyuanCard::ZiyuanCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void ZiyuanCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, card_use.from->objectName(), card_use.to.first()->objectName(), "ziyuan", QString());
    room->obtainCard(card_use.to.first(), this, reason, true);
}

void ZiyuanCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->recover(effect.to, RecoverStruct(effect.from));
}

class Ziyuan : public ViewAsSkill
{
public:
    Ziyuan() : ViewAsSkill("ziyuan")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        int sum = 0;
        foreach(const Card *card, selected)
            sum += card->getNumber();

        sum += to_select->getNumber();
        return !to_select->isEquipped() && sum <= 13;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ZiyuanCard");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        int sum = 0;
        foreach(const Card *card, cards)
            sum += card->getNumber();

        if (sum != 13)
            return NULL;

        ZiyuanCard *rende_card = new ZiyuanCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class Jugu : public DrawCardsSkill
{
public:
    Jugu() : DrawCardsSkill("#jugu-draw", true)
    {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->sendCompulsoryTriggerLog(player, "jugu");
        player->broadcastSkillInvoke("jugu");
        return n + player->getMaxHp();
    }
};

class JuguKeep : public MaxCardsSkill
{
public:
    JuguKeep() : MaxCardsSkill("jugu")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill(this))
            return target->getMaxHp();
        else
            return 0;
    }
};

class Hongde : public TriggerSkill
{
public:
    Hongde() : TriggerSkill("hongde")
    {
        events << CardsMoveOneTime;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!room->getTag("FirstRound").toBool() && move.to == player && move.to_place == Player::PlaceHand) {
            if (move.card_ids.size() < 2) return false;
        } else if (move.from == player && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip))) {
            int lose_num = 0;
            for (int i = 0; i < move.card_ids.length(); i++) {
                if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip) {
                    lose_num ++;
                }
            }
            if (lose_num < 2) return false;
        } else return false;
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@hongde-invoke", true, true);
        if (target) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
            target->drawCards(1, objectName());
        }
        return false;
    }
};

DingpanCard::DingpanCard()
{
}

bool DingpanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->hasEquip();
}

void DingpanCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = source->getRoom();
    target->drawCards(1, "dingpan");
    if (!target->hasEquip()) return;
    if (room->askForChoice(target, "dingpan", "disequip+takeback", QVariant::fromValue(source), "@dingpan-choose:"+source->objectName()) == "disequip")
        room->throwCard(room->askForCardChosen(source, target, "e", "dingpan", false, Card::MethodDiscard), target, source);
    else {
        QList<const Card *> equips = target->getEquips();
        if (equips.isEmpty()) return;
        DummyCard *card = new DummyCard;
        foreach (const Card *equip, equips) {
            card->addSubcard(equip);
        }
        if (card->subcardsLength() > 0)
            target->obtainCard(card);
        room->damage(DamageStruct("dingpan", source, target));
    }
}

class DingpanViewAsSkill : public ZeroCardViewAsSkill
{
public:
    DingpanViewAsSkill() : ZeroCardViewAsSkill("dingpan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("#dingpan") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new DingpanCard;
    }
};

class Dingpan : public TriggerSkill
{
public:
    Dingpan() : TriggerSkill("dingpan")
    {
        events << PlayCard;
        view_as_skill = new DingpanViewAsSkill;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        int rebel_num = 0;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getRole() == "rebel")
                rebel_num++;
        }
        if (rebel_num > 0)
            room->setPlayerMark(player, "#dingpan", rebel_num - player->usedTimes("DingpanCard"));
        return false;
    }
};

OLPackage::OLPackage()
: Package("OL")
{
    General *chenlin = new General(this, "chenlin", "wei", 3); // SP 020
    chenlin->addSkill(new Bifa);
    chenlin->addSkill(new Songci);

    General *zhugeke = new General(this, "zhugeke", "wu", 3); // OL 002
    zhugeke->addSkill(new Aocai);
    zhugeke->addSkill(new Duwu);

    General *zhangbao = new General(this, "zhangbao", "qun", 3); // SP 025
    zhangbao->addSkill(new Zhoufu);
    zhangbao->addSkill(new Yingbing);

    General *chengyu = new General(this, "chengyu", "wei", 3);
    chengyu->addSkill(new Shefu);
    chengyu->addSkill(new ShefuCancel);
	chengyu->addSkill(new DetachEffectSkill("shefu", "ambush"));
	related_skills.insertMulti("shefu", "#shefu-clear");
    related_skills.insertMulti("shefu", "#shefu-cancel");
    chengyu->addSkill(new Benyu);

    General *sunhao = new General(this, "sunhao$", "wu", 5); // SP 041
    sunhao->addSkill(new Canshi);
    sunhao->addSkill(new Chouhai);
    sunhao->addSkill(new Guiming); // in Player::isWounded()

	General *mayunlu = new General(this, "mayunlu", "shu", 4, false);
	mayunlu->addSkill(new Fengpo);
	mayunlu->addSkill("mashu");

	General *wutugu = new General(this, "wutugu", "qun", 15);
	wutugu->addSkill(new Ranshang);
	wutugu->addSkill(new Hanyong);

	General *shixie = new General(this, "shixie", "qun", 3);
	shixie->addSkill(new Biluan);
    shixie->addSkill(new Lixia);

	General *tadun = new General(this, "tadun", "qun");
	tadun->addSkill(new Luanzhan);

	General *yanbaihu = new General(this, "yanbaihu", "qun");
	yanbaihu->addSkill(new Zhidao);
	yanbaihu->addSkill(new ZhidaoProhibit);
	yanbaihu->addSkill(new Jili);
	related_skills.insertMulti("zhidao", "#zhidao");

	General *zhanglu = new General(this, "zhanglu", "qun", 3);
	zhanglu->addSkill(new Yishe);
	zhanglu->addSkill(new DetachEffectSkill("yishe", "rice"));
	related_skills.insertMulti("yishe", "#yishe-clear");
	zhanglu->addSkill(new Bushi);
	zhanglu->addSkill(new Midao);

	General *guansuo = new General(this, "guansuo", "shu");
	guansuo->addSkill(new Zhengnan);
	guansuo->addSkill(new Xiefang);
	guansuo->addRelateSkill("wusheng");
	guansuo->addRelateSkill("dangxian");
	guansuo->addRelateSkill("zhiman");

	General *wanglang = new General(this, "wanglang", "wei", 3);
    wanglang->addSkill(new Gushe);
    wanglang->addSkill(new Jici);

	General *cuiyan = new General(this, "cuiyan", "wei", 3);
	cuiyan->addSkill(new Yawang);
	cuiyan->addSkill(new YawangLimitation);
	cuiyan->addSkill(new Xunzhi);
	cuiyan->addSkill(new XunzhiKeep);
	related_skills.insertMulti("yawang", "#yawang");
	related_skills.insertMulti("xunzhi", "#xunzhi");

    General *litong = new General(this, "litong", "wei");
    litong->addSkill(new Tuifeng);
    litong->addSkill(new TuifengTargetMod);
    litong->addSkill(new DetachEffectSkill("tuifeng", "tuifeng"));
    related_skills.insertMulti("tuifeng", "#tuifeng-target");
    related_skills.insertMulti("tuifeng", "#tuifeng-clear");

    General *mizhu = new General(this, "mizhu", "shu", 3);
    mizhu->addSkill(new Ziyuan);
    mizhu->addSkill(new Jugu);
    mizhu->addSkill(new JuguKeep);
    related_skills.insertMulti("jugu", "#jugu-draw");

    General *buzhi = new General(this, "buzhi", "wu", 3);
    buzhi->addSkill(new Hongde);
    buzhi->addSkill(new Dingpan);
/*
	General *taoqian = new General(this, "taoqian", "qun", 3);
	taoqian->addSkill(new Zhaohuo);
	taoqian->addSkill(new Yibing);
	taoqian->addSkill(new Qingshou);

	General *caojie = new General(this, "caojie", "qun", 3, false);
	caojie->addSkill(new Zuyin);
    caojie->addSkill(new Tianzuo);
*/
    addMetaObject<BifaCard>();
    addMetaObject<SongciCard>();
    addMetaObject<AocaiCard>();
    addMetaObject<DuwuCard>();
	addMetaObject<ZhoufuCard>();
    addMetaObject<ShefuCard>();
	addMetaObject<QingshouCard>();
	addMetaObject<LuanzhanCard>();
	addMetaObject<TianzuoDiscardCard>();
	addMetaObject<GusheCard>();
    addMetaObject<ZiyuanCard>();
    addMetaObject<DingpanCard>();
	
    skills << new AocaiVeiw << new ShixieDistance << new FengpoRecord << new LuanzhanColl << new TianzuoDiscard;
}

ADD_PACKAGE(OL)
