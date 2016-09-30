#include "bestloyalist.h"
#include "engine.h"
#include "clientplayer.h"

AllArmy::AllArmy(Card::Suit suit, int number)
    : DelayedTrick(suit, number)
{
    setObjectName("all_army");

    judge.pattern = ".|club";
    judge.good = true;
    judge.reason = objectName();
}

bool AllArmy::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->containsTrick(objectName()) && to_select != Self;
}

void AllArmy::takeEffect(ServerPlayer *target) const
{
    target->broadcastSkillInvoke("@all_army");
    target->getRoom()->addPlayerMark(target, "all_army");
}

class AllArmyDraw : public TriggerSkill
{
public:
    AllArmyDraw() : TriggerSkill("all_army_others_draw")
    {
        events << DrawNCards << EventPhaseEnd;
        global = true;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getMark("all_army") == 0)
            return false;
        if (triggerEvent == DrawNCards) {
            int n = data.toInt();
            data = n - 1;
        } else if (player->getPhase() == Player::Draw) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->distanceTo(player) == 1)
                    p->drawCards(1, "all_army");
            }
            room->setPlayerMark(player, "all_army", 0);
        }
        return false;
    }

    int getPriority(TriggerEvent) const
    {
        return 2;
    }
};

IncreaseArmy::IncreaseArmy(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("increase_army");
}

bool IncreaseArmy::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num;
}

void IncreaseArmy::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    effect.to->drawCards(3);

    QList<const Card *> cards = effect.to->getCards("he");
    QList<const Card *> cardsCopy = cards;

    foreach(const Card *c, cardsCopy)
    {
        if (effect.to->isJilei(c))
            cards.removeOne(c);
    }

    if (cards.length() == 0)
        return;

    bool instanceDiscard = false;
    int instanceDiscardId = -1;

    if (cards.length() == 1)
        instanceDiscard = true;
    else if (cards.length() == 2) {
        bool bothBasic = true;
        int cardId = -1;

        foreach(const Card *c, cards)
        {
            if (c->getTypeId() != Card::TypeBasic)
                bothBasic = false;
            else
                cardId = c->getId();
        }

        instanceDiscard = bothBasic;
        instanceDiscardId = cardId;
    }

    if (instanceDiscard) {
        DummyCard d;
        if (instanceDiscardId == -1)
            d.addSubcards(cards);
        else
            d.addSubcard(instanceDiscardId);
        room->throwCard(&d, effect.to);
    } else if (!room->askForCard(effect.to, "@@reducecookdiscard!", "@reducecook-discard")) {
        DummyCard d;
        qShuffle(cards);
        int cardId = -1;
        foreach(const Card *c, cards)
        {
            if (c->getTypeId() != Card::TypeBasic) {
                cardId = c->getId();
                break;
            }
        }
        if (cardId != -1)
            d.addSubcard(cardId);
        else {
            d.addSubcard(cards.first());
            d.addSubcard(cards.last());
        }

        room->throwCard(&d, effect.to);
    }
}

class ReduceCookDiscard : public ViewAsSkill
{
public:
    ReduceCookDiscard() : ViewAsSkill("reducecookdiscard")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@reducecookdiscard!";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Self->isJilei(to_select))
            return false;

        if (selected.length() == 0)
            return true;
        else if (selected.length() == 1) {
            if (selected.first()->getTypeId() != Card::TypeBasic)
                return false;
            return true;
        } else
            return false;

        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        bool ok = false;
        if (cards.length() == 1)
            ok = cards.first()->getTypeId() != Card::TypeBasic;
        else if (cards.length() == 2)
            ok = true;

        if (!ok)
            return NULL;

        DummyCard *dummy = new DummyCard;
        dummy->addSubcards(cards);
        return dummy;
    }
};

BeatAnother::BeatAnother(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("beat_another");
}

bool BeatAnother::isAvailable(const Player *player) const
{
    return SingleTargetTrick::isAvailable(player);
}

bool BeatAnother::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

bool BeatAnother::targetFilter(const QList<const Player *> &targets,
    const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty()) {
        Q_ASSERT(targets.length() <= 2);
        if (targets.length() == 2) return false;
        return Self != to_select;
    } else
        return Self->distanceTo(to_select) == 1;
    return false;
}

void BeatAnother::onUse(Room *room, const CardUseStruct &card_use) const
{
    Q_ASSERT(card_use.to.length() == 2);
    ServerPlayer *from = card_use.to.at(0);
    ServerPlayer *to = card_use.to.at(1);
    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, from->objectName(), to->objectName());

    CardUseStruct new_use = card_use;
    new_use.to.removeAt(1);
    this->tag["beatAnotherTo"] = QVariant::fromValue(to);

    SingleTargetTrick::onUse(room, new_use);
}

void BeatAnother::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    ServerPlayer *killer = effect.to;
    ServerPlayer *victim = this->tag["beatAnotherTo"].value<ServerPlayer *>();
    this->tag.remove("beatAnotherTo");
    if (victim == NULL) return;

    if (source->isKongcheng()) return;
    const Card *card = room->askForCard(source, ".", "@beatanother-give:" + killer->objectName(), QVariant(), Card::MethodNone);
    if (card != NULL) {
        killer->obtainCard(card, false);
        if (killer->isKongcheng()) return;
        QList<const Card *> cards = killer->getCards("he");

        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, killer->objectName(), victim->objectName(), "beat_another", QString());
        if (cards.length() == 0)
            return;
        else if (cards.length() <= 2) {
            DummyCard d;
            d.addSubcards(cards);
            room->obtainCard(victim, &d, reason, false);
            d.deleteLater();
            return;
        }

        const Card *to_give = room->askForCard(killer, "@@beatanothergive!", "@beatanother-give2:" + victim->objectName(), QVariant(), Card::MethodNone);
        if (to_give == NULL) {
            DummyCard d;
            qShuffle(cards);
            d.addSubcard(cards.first());
            d.addSubcard(cards.last());
            room->obtainCard(victim, &d, reason, false);
            d.deleteLater();
        } else
            room->obtainCard(victim, to_give, reason, false);
    }
}

class BeatAnotherGive : public ViewAsSkill
{
public:
    BeatAnotherGive() : ViewAsSkill("beatanothergive")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@beatanothergive!";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        return selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        DummyCard *dummy = new DummyCard;
        dummy->addSubcards(cards);
        return dummy;
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
                foreach(ServerPlayer *p, room->getAllPlayers())
                {
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
            if (player->getMark("#yawang") < 1) {
                room->setPlayerCardLimitation(player, "use", ".|.|.|.", true);
                room->setPlayerFlag(player, "YawangLimitation");
            }
        } else if (triggerEvent == EventPhaseEnd) {
            if (player->hasFlag("YawangLimitation")) {
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
                if (player->getMark("#yawang") < 1) {
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

        foreach(ServerPlayer *p, room->getOtherPlayers(cuiyan))
        {
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

BestLoyalistCardPackage::BestLoyalistCardPackage()
    : Package("BestLoyalistCard", Package::CardPack)
{
    QList<Card *> cards;

    cards << new AllArmy(Card::Spade, 4)
        << new AllArmy(Card::Spade, 10)
        << new BeatAnother(Card::Spade, 3)
        << new BeatAnother(Card::Spade, 4)
        << new BeatAnother(Card::Spade, 11);

    cards << new IncreaseArmy(Card::Heart, 3)
        << new IncreaseArmy(Card::Heart, 4)
        << new IncreaseArmy(Card::Heart, 7)
        << new IncreaseArmy(Card::Heart, 8)
        << new IncreaseArmy(Card::Heart, 9)
        << new IncreaseArmy(Card::Heart, 11);

    cards << new BeatAnother(Card::Diamond, 3)
        << new BeatAnother(Card::Diamond, 4);

    foreach(Card *card, cards)
        card->setParent(this);

    skills << new AllArmyDraw << new ReduceCookDiscard << new BeatAnotherGive;
}

ADD_PACKAGE(BestLoyalistCard)

BestLoyalistPackage::BestLoyalistPackage()
    :Package("BestLoyalist")
{
    General *cuiyan = new General(this, "cuiyan", "wei", 3, true, true);
    cuiyan->addSkill(new Yawang);
    cuiyan->addSkill(new YawangLimitation);
    cuiyan->addSkill(new Xunzhi);
    cuiyan->addSkill(new XunzhiKeep);
    related_skills.insertMulti("yawang", "#yawang");
    related_skills.insertMulti("xunzhi", "#xunzhi");
}

ADD_PACKAGE(BestLoyalist)