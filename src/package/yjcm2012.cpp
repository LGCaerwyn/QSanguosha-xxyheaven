#include "yjcm2012.h"
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

class Zhenlie : public TriggerSkill
{
public:
    Zhenlie() : TriggerSkill("zhenlie")
    {
        events << TargetConfirmed;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.contains(player) && use.from != player) {
                if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                    if (room->askForSkillInvoke(player, objectName(), data)) {
                        player->broadcastSkillInvoke(objectName());
                        player->setFlags("-ZhenlieTarget");
                        player->setFlags("ZhenlieTarget");
                        room->loseHp(player);
                        if (player->isAlive() && player->hasFlag("ZhenlieTarget")) {
                            player->setFlags("-ZhenlieTarget");
                            use.nullified_list << player->objectName();
                            data = QVariant::fromValue(use);
                            if (player->canDiscard(use.from, "he")) {
								int id = room->askForCardChosen(player, use.from, "he", objectName(), false, Card::MethodDiscard);
                                room->throwCard(id, use.from, player);
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
};

class Miji : public PhaseChangeSkill
{
public:
    Miji() : PhaseChangeSkill("miji")
    {

    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
		if (target->getPhase() == Player::Finish && target->isWounded() && target->askForSkillInvoke(this)) {
            target->broadcastSkillInvoke(objectName());
            QStringList draw_num;
            for (int i = 1; i <= target->getLostHp(); draw_num << QString::number(i++)) {}
            int num = room->askForChoice(target, "miji_draw", draw_num.join("+")).toInt();
            target->drawCards(num, objectName());
			QList<int> handcards = target->handCards();
			room->askForRende(target, handcards, objectName(), false, false, true, num, -1);
        }
        return false;
    }
};

QiceCard::QiceCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "qice";
}

bool QiceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *mutable_card = Sanguosha->cloneCard(user_string);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, mutable_card, targets);
}

bool QiceCard::targetFixed() const
{
    Card *mutable_card = Sanguosha->cloneCard(user_string);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFixed();
}

bool QiceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Card *mutable_card = Sanguosha->cloneCard(user_string);
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetsFeasible(targets, Self);
}

const Card *QiceCard::validate(CardUseStruct &) const
{
    Card *use_card = Sanguosha->cloneCard(user_string);
    use_card->setSkillName("qice");
    use_card->addSubcards(this->subcards);
    return use_card;
}

class Qice : public ZeroCardViewAsSkill
{
public:
    Qice() : ZeroCardViewAsSkill("qice")
    {
    }

    QString getSelectBox() const
    {
        return "guhuo_t";
    }

    virtual const Card *viewAs() const
    {
        QiceCard *card = new QiceCard;
        card->addSubcards(Self->getHandcards());
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("QiceCard");
    }
};

class Zhiyu : public MasochismSkill
{
public:
    Zhiyu() : MasochismSkill("zhiyu")
    {
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        if (target->askForSkillInvoke(this, QVariant::fromValue(damage))) {
			Room *room = target->getRoom();
            target->broadcastSkillInvoke(objectName());
            target->drawCards(1, objectName());

            

            if (target->isKongcheng())
                return;
            room->showAllCards(target);

            QList<const Card *> cards = target->getHandcards();
            Card::Color color = cards.first()->getColor();
            bool same_color = true;
            foreach (const Card *card, cards) {
                if (card->getColor() != color) {
                    same_color = false;
                    break;
                }
            }

            if (same_color && damage.from && damage.from->canDiscard(damage.from, "h"))
                room->askForDiscard(damage.from, objectName(), 1, 1);
        }
    }
};

class Jiangchi : public DrawCardsSkill
{
public:
    Jiangchi() : DrawCardsSkill("jiangchi")
    {
		view_as_skill = new dummyVS;
    }

    virtual int getDrawNum(ServerPlayer *caozhang, int n) const
    {
        Room *room = caozhang->getRoom();
		if (!room->askForSkillInvoke(caozhang, "skill_ask", "prompt:::" + objectName())) return false;
        QString choice = room->askForChoice(caozhang, objectName(), "jiang+chi+cancel");
        if (choice == "cancel")
            return n;

        room->notifySkillInvoked(caozhang, objectName());
        LogMessage log;
        log.from = caozhang;
        log.arg = objectName();
        if (choice == "jiang") {
            log.type = "#Jiangchi1";
            room->sendLog(log);
            caozhang->broadcastSkillInvoke(objectName(), 1);
            room->setPlayerCardLimitation(caozhang, "use,response", "Slash", true);
            return n + 1;
        } else {
            log.type = "#Jiangchi2";
            room->sendLog(log);
            caozhang->broadcastSkillInvoke(objectName(), 2);
            room->setPlayerFlag(caozhang, "JiangchiInvoke");
            return n - 1;
        }
    }
};

class JiangchiTargetMod : public TargetModSkill
{
public:
    JiangchiTargetMod() : TargetModSkill("#jiangchi-target")
    {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasFlag("JiangchiInvoke"))
            return 1;
        else
            return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasFlag("JiangchiInvoke"))
            return 1000;
        else
            return 0;
    }
};

class Qianxi : public PhaseChangeSkill
{
public:
    Qianxi() : PhaseChangeSkill("qianxi")
    {
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();

        if (target->getPhase() != Player::Start)
			return false;
        if (target->askForSkillInvoke(objectName())) {
            target->broadcastSkillInvoke(objectName());

            target->drawCards(1, objectName());

            if (target->isNude())
                return false;

            const Card *c = room->askForCard(target, "..!", "@qianxi-discard");
            if (c == NULL) {
                c = target->getCards("he").at(0);
                room->throwCard(c, target);
            }

            if (target->isDead())
                return false;

            QString color;
            if (c->isBlack())
                color = "black";
            else if (c->isRed())
                color = "red";
            else
                return false;
            QList<ServerPlayer *> to_choose;
            foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
                if (target->distanceTo(p) == 1)
                    to_choose << p;
            }
            if (to_choose.isEmpty())
                return false;

            ServerPlayer *victim = room->askForPlayerChosen(target, to_choose, objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), victim->objectName());
            QString pattern = QString(".|%1|.|hand$0").arg(color);
            target->tag[objectName()] = QVariant::fromValue(color);

            room->setPlayerFlag(victim, "QianxiTarget");
            room->addPlayerTip(victim, QString("#qianxi_%1").arg(color));
            room->setPlayerCardLimitation(victim, "use,response", pattern, false);

            LogMessage log;
            log.type = "#Qianxi";
            log.from = victim;
            log.arg = QString("no_suit_%1").arg(color);
            room->sendLog(log);
        }
        return false;
    }
};

class QianxiClear : public TriggerSkill
{
public:
    QianxiClear() : TriggerSkill("#qianxi-clear")
    {
        events << EventPhaseChanging << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return !target->tag["qianxi"].toString().isNull();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
        }

        QString color = player->tag["qianxi"].toString();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasFlag("QianxiTarget")) {
                room->removePlayerCardLimitation(p, "use,response", QString(".|%1|.|hand$0").arg(color));
                room->removePlayerTip(p, QString("#qianxi_%1").arg(color));
            }
        }
        return false;
    }
};

class Dangxian : public TriggerSkill
{
public:
    Dangxian() : TriggerSkill("dangxian")
    {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *liaohua, QVariant &) const
    {
        if (liaohua->getPhase() == Player::RoundStart) {
            liaohua->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(liaohua, objectName());

            liaohua->setPhase(Player::Play);
            room->broadcastProperty(liaohua, "phase");
            RoomThread *thread = room->getThread();
            if (!thread->trigger(EventPhaseStart, room, liaohua))
                thread->trigger(EventPhaseProceeding, room, liaohua);
            thread->trigger(EventPhaseEnd, room, liaohua);

            liaohua->setPhase(Player::RoundStart);
            room->broadcastProperty(liaohua, "phase");
        }
        return false;
    }
};

class Fuli : public TriggerSkill
{
public:
    Fuli() : TriggerSkill("fuli")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@laoji";
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@laoji") > 0;
    }

    int getKingdoms(Room *room) const
    {
        QSet<QString> kingdom_set;
        foreach(ServerPlayer *p, room->getAlivePlayers())
            kingdom_set << p->getKingdom();
        return kingdom_set.size();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *liaohua, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != liaohua)
            return false;
        if (liaohua->askForSkillInvoke(this, data)) {
            liaohua->broadcastSkillInvoke(objectName());
            //room->doLightbox("$FuliAnimate", 3000);

            room->removePlayerMark(liaohua, "@laoji");
            room->recover(liaohua, RecoverStruct(liaohua, NULL, getKingdoms(room) - liaohua->getHp()));

            liaohua->turnOver();
        }
        return false;
    }
};

class Zishou : public DrawCardsSkill
{
public:
    Zishou() : DrawCardsSkill("zishou")
    {

    }

    int getDrawNum(ServerPlayer *player, int n) const
    {
        if (player->askForSkillInvoke(this)) {
            Room *room = player->getRoom();
            player->broadcastSkillInvoke(objectName());

            room->setPlayerFlag(player, "zishou");

            QSet<QString> kingdomSet;
            foreach (ServerPlayer *p, room->getAlivePlayers())
                kingdomSet.insert(p->getKingdom());

            return n + kingdomSet.count();
        }

        return n;
    }
};

class ZishouProhibit : public ProhibitSkill
{
public:
    ZishouProhibit() : ProhibitSkill("#zishou")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        if (card->isKindOf("SkillCard"))
            return false;

        if (from->hasFlag("zishou"))
            return from != to;

        return false;
    }
};

class Zongshi : public MaxCardsSkill
{
public:
    Zongshi() : MaxCardsSkill("zongshi")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        int extra = 0;
        QSet<QString> kingdom_set;
        if (target->parent()) {
            foreach(const Player *player, target->parent()->findChildren<const Player *>())
            {
                if (player->isAlive())
                    kingdom_set << player->getKingdom();
            }
        }
        extra = kingdom_set.size();
        if (target->hasSkill(this))
            return extra;
        else
            return 0;
    }
};

class Shiyong : public TriggerSkill
{
public:
    Shiyong() : TriggerSkill("shiyong")
    {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash")
            && (damage.card->isRed() || damage.card->hasFlag("drank"))) {
            player->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());

            room->loseMaxHp(player);
        }
        return false;
    }
};

class FuhunViewAsSkill : public ViewAsSkill
{
public:
    FuhunViewAsSkill() : ViewAsSkill("fuhun")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getHandcardNum() >= 2 && Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return player->getHandcardNum() >= 2 && pattern == "slash";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        Slash *slash = new Slash(Card::SuitToBeDecided, 0);
        slash->setSkillName(objectName());
        slash->addSubcards(cards);

        return slash;
    }
};

class Fuhun : public TriggerSkill
{
public:
    Fuhun() : TriggerSkill("fuhun")
    {
        events << Damage << EventPhaseChanging;
        view_as_skill = new FuhunViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damage && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName()
                && player->getPhase() == Player::Play) {
                room->handleAcquireDetachSkills(player, "wusheng|paoxiao");
                player->broadcastSkillInvoke(objectName());
                player->setFlags(objectName());
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && player->hasFlag(objectName()))
                room->handleAcquireDetachSkills(player, "-wusheng|-paoxiao", true);
        }

        return false;
    }
};

GongqiCard::GongqiCard()
{
    target_fixed = true;
}

void GongqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->setPlayerFlag(source, "InfinityAttackRange");
    const Card *cd = Sanguosha->getCard(subcards.first());
    if (cd->isKindOf("EquipCard")) {
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(source))
            if (source->canDiscard(p, "he")) targets << p;
        if (!targets.isEmpty()) {
            ServerPlayer *to_discard = room->askForPlayerChosen(source, targets, "gongqi", "@gongqi-discard", true);
            if (to_discard)
                room->throwCard(room->askForCardChosen(source, to_discard, "he", "gongqi", false, Card::MethodDiscard), to_discard, source);
        }
    } 
}

class Gongqi : public OneCardViewAsSkill
{
public:
    Gongqi() : OneCardViewAsSkill("gongqi")
    {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GongqiCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const
    {
        GongqiCard *card = new GongqiCard;
        card->addSubcard(originalcard->getId());
        card->setSkillName(objectName());
        return card;
    }
};

JiefanCard::JiefanCard()
{
}

bool JiefanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void JiefanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(source, "@rescue");
    ServerPlayer *target = targets.first();
    source->tag["JiefanTarget"] = QVariant::fromValue(target);
    //room->doLightbox("$JiefanAnimate", 2500);

    foreach (ServerPlayer *player, room->getAllPlayers()) {
        if (player->isAlive() && player->inMyAttackRange(target))
            room->cardEffect(this, source, player);
    }
    source->tag.remove("JiefanTarget");
}

void JiefanCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    ServerPlayer *target = effect.from->tag["JiefanTarget"].value<ServerPlayer *>();
    QVariant data = effect.from->tag["JiefanTarget"];
    if (target && !room->askForCard(effect.to, ".Weapon", "@jiefan-discard::" + target->objectName(), data))
        target->drawCards(1, "jiefan");
}

class Jiefan : public ZeroCardViewAsSkill
{
public:
    Jiefan() : ZeroCardViewAsSkill("jiefan")
    {
        frequency = Limited;
        limit_mark = "@rescue";
    }

    virtual const Card *viewAs() const
    {
        return new JiefanCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@rescue") >= 1;
    }
};

AnxuCard::AnxuCard()
{
}

bool AnxuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self)
        return false;
    if (targets.isEmpty())
        return true;
    else if (targets.length() == 1)
        return to_select->getHandcardNum() != targets.first()->getHandcardNum();
    else
        return false;
}

bool AnxuCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void AnxuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QList<ServerPlayer *> selecteds = targets;
    ServerPlayer *from = selecteds.first()->getHandcardNum() < selecteds.last()->getHandcardNum() ? selecteds.takeFirst() : selecteds.takeLast();
    ServerPlayer *to = selecteds.takeFirst();
    if (!to->isKongcheng()){
		room->setPlayerFlag(from, "anxu_target"); // For AI
        const Card *card = room->askForExchange(to, "anxu", 1, 1, false, "@anxu-give:" + source->objectName() + ":" + from->objectName());
		room->setPlayerFlag(from, "-anxu_target"); // For AI
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, to->objectName(),
            from->objectName(), "anxu", QString());
        reason.m_playerId = source->objectName();
        room->moveCardTo(card, to, from, Player::PlaceHand, reason);
        delete card;
    } 
    if (from->getHandcardNum() == to->getHandcardNum()){
		if (source->isWounded() && room->askForChoice(source, "anxu", "recover+draw") == "recover")
                room->recover(source, RecoverStruct(source));
            else
                source->drawCards(1, "anxu");
	}
}

class Anxu : public ZeroCardViewAsSkill
{
public:
    Anxu() : ZeroCardViewAsSkill("anxu")
    {
    }

    virtual const Card *viewAs() const
    {
        return new AnxuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("AnxuCard");
    }
};

class Zhuiyi : public TriggerSkill
{
public:
    Zhuiyi() : TriggerSkill("zhuiyi")
    {
        events << Death;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill(this);
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        QList<ServerPlayer *> targets = (death.damage && death.damage->from) ? room->getOtherPlayers(death.damage->from) :
            room->getAlivePlayers();

        if (targets.isEmpty())
            return false;

        QString prompt = "zhuiyi-invoke";
        if (death.damage && death.damage->from && death.damage->from != player)
            prompt = QString("%1x:%2").arg(prompt).arg(death.damage->from->objectName());
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), prompt, true, true);
        if (!target) return false;

        player->broadcastSkillInvoke(objectName());

        target->drawCards(3, objectName());
        room->recover(target, RecoverStruct(player));
        return false;
    }
};

class Lihuo : public TriggerSkill
{
public:
    Lihuo() : TriggerSkill("lihuo")
    {
        events << CardUsed << PreDamageDone << CardFinished;
        view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (triggerEvent == CardUsed) {
			if (!TriggerSkill::triggerable(player))
				return false;
			CardUseStruct use = data.value<CardUseStruct>();
			bool changed = false;
            if (use.card->objectName() == "slash") {
				FireSlash *fire_slash = new FireSlash(use.card->getSuit(), use.card->getNumber());
			    fire_slash->copyFrom(use.card);
			    bool can_use = true;
                foreach (ServerPlayer *p, use.to) {
                    if (!player->canSlash(p, fire_slash, false)) {
                        can_use = false;
                        break;
                    }
                }
                if (can_use && room->askForSkillInvoke(player, objectName(), data)){
				    changed = true;
                    player->broadcastSkillInvoke("lihuo", 1);
					room->setCardFlag(fire_slash, "InvokedLihuo");
				    use.card = fire_slash;
			    }
			}
			if (use.card->isKindOf("FireSlash") && !use.card->hasFlag("slashDisableExtraTarget")) {
				QList<ServerPlayer *> available_targets;
				bool no_distance_limit = false;
				if (use.card->hasFlag("slashNoDistanceLimit")){
					no_distance_limit = true;
					room->setPlayerFlag(player, "slashNoDistanceLimit");
				}
				foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (use.to.contains(p) || room->isProhibited(player, p, use.card)) continue;
                    if (use.card->targetFilter(QList<const Player *>(), p, player))
                        available_targets << p;
                }
				if (no_distance_limit)
					room->setPlayerFlag(player, "-slashNoDistanceLimit");
				if (available_targets.isEmpty())
					return false;
				ServerPlayer *extra = room->askForPlayerChosen(player, available_targets, objectName(), "@lihuo-add", true);
				if (extra) {
					LogMessage log;
                    log.type = "#QiaoshuiAdd";
                    log.from = player;
                    log.to << extra;
                    log.card_str = use.card->toString();
                    log.arg = objectName();
                    room->sendLog(log);
					if (!changed)
                        player->broadcastSkillInvoke("lihuo", 1);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), extra->objectName());
					use.to.append(extra);
                    room->sortByActionOrder(use.to);
				}
			}
			data = QVariant::fromValue(use);
        } else if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->hasFlag("InvokedLihuo")) {
                QVariantList slash_list = damage.from->tag["InvokeLihuo"].toList();
                slash_list << QVariant::fromValue(damage.card);
                damage.from->tag["InvokeLihuo"] = QVariant::fromValue(slash_list);
            }
        } else if (TriggerSkill::triggerable(player) && !player->hasFlag("Global_ProcessBroken")) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;

            bool can_invoke = false;
            QVariantList slash_list = use.from->tag["InvokeLihuo"].toList();
            foreach (QVariant card, slash_list) {
                if (card.value<const Card *>() == use.card) {
                    can_invoke = true;
                    slash_list.removeOne(card);
                    use.from->tag["InvokeLihuo"] = QVariant::fromValue(slash_list);
                    break;
                }
            }
            if (!can_invoke) return false;

            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke("lihuo", 2);
            room->loseHp(player, 1);
        }
        return false;
    }
};

ChunlaoCard::ChunlaoCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ChunlaoCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->addToPile("wine", this);
}

ChunlaoWineCard::ChunlaoWineCard()
{
    m_skillName = "chunlao";
    target_fixed = true;
}

void ChunlaoWineCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
	ServerPlayer *who = room->getCurrentDyingPlayer();
	if (who)
		room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), who->objectName());

    if (!who || who->getHp() > 0) return;

    Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
    analeptic->setSkillName("_chunlao");
    analeptic->setFlags("UsedBySecondWay");
    room->useCard(CardUseStruct(analeptic, who, who, false));
}

class ChunlaoViewAsSkill : public ViewAsSkill
{
public:
    ChunlaoViewAsSkill() : ViewAsSkill("chunlao")
    {
        expand_pile = "wine";
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@chunlao";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (Self->getPile("wine").isEmpty())
            return to_select->isKindOf("Slash");
        else {
            ExpPattern pattern(".|.|.|wine");
            if (!pattern.match(Self, to_select)) return false;
            return selected.length() == 0;
        }
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (Self->getPile("wine").isEmpty()) {
            if (cards.length() == 0) return NULL;

            Card *acard = new ChunlaoCard;
            acard->addSubcards(cards);
            acard->setSkillName(objectName());
            return acard;
        } else {
            if (cards.length() != 1) return NULL;
            Card *wine = new ChunlaoWineCard;
            wine->addSubcards(cards);
            wine->setSkillName(objectName());
            return wine;
        }
    }
};

class Chunlao : public TriggerSkill
{
public:
    Chunlao() : TriggerSkill("chunlao")
    {
        events << EventPhaseStart << AskForPeaches;
        view_as_skill = new ChunlaoViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *chengpu, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && chengpu->getPhase() == Player::Finish
            && !chengpu->isKongcheng() && chengpu->getPile("wine").isEmpty()) {
            room->askForUseCard(chengpu, "@@chunlao", "@chunlao", 1, Card::MethodNone);
        }if (triggerEvent == AskForPeaches && !chengpu->getPile("wine").isEmpty()) {
			DyingStruct dying_data = data.value<DyingStruct>();
			room->askForUseCard(chengpu, "@@chunlao", "@chunlao-save:" + dying_data.who->objectName(), 2, Card::MethodNone);
		}
        return false;
    }

};

YJCM2012Package::YJCM2012Package()
    : Package("YJCM2012")
{
    General *bulianshi = new General(this, "bulianshi", "wu", 3, false); // YJ 101
    bulianshi->addSkill(new Anxu);
    bulianshi->addSkill(new Zhuiyi);

    General *caozhang = new General(this, "caozhang", "wei"); // YJ 102
    caozhang->addSkill(new Jiangchi);
    caozhang->addSkill(new JiangchiTargetMod);
    related_skills.insertMulti("jiangchi", "#jiangchi-target");

    General *chengpu = new General(this, "chengpu", "wu"); // YJ 103
    chengpu->addSkill(new Lihuo);
    chengpu->addSkill(new Chunlao);
	chengpu->addSkill(new DetachEffectSkill("chunlao", "wine"));
	related_skills.insertMulti("chunlao", "#chunlao-clear");

    General *guanxingzhangbao = new General(this, "guanxingzhangbao", "shu"); // YJ 104
    guanxingzhangbao->addSkill(new Fuhun);
	guanxingzhangbao->addRelateSkill("wusheng");
	guanxingzhangbao->addRelateSkill("paoxiao");

    General *handang = new General(this, "handang", "wu"); // YJ 105
    handang->addSkill(new Gongqi);
    handang->addSkill(new Jiefan);

    General *huaxiong = new General(this, "yj_huaxiong", "qun", 6); // YJ 106
    huaxiong->addSkill(new Shiyong);

    General *liaohua = new General(this, "liaohua", "shu"); // YJ 107
    liaohua->addSkill(new Dangxian);
    liaohua->addSkill(new Fuli);

    General *liubiao = new General(this, "liubiao", "qun", 3); // YJ 108
    liubiao->addSkill(new Zishou);
    liubiao->addSkill(new ZishouProhibit);
	related_skills.insertMulti("zishou", "#zishou");
    liubiao->addSkill(new Zongshi);

    General *madai = new General(this, "madai", "shu"); // YJ 109
    madai->addSkill("mashu");
    madai->addSkill(new Qianxi);
    madai->addSkill(new QianxiClear);
    related_skills.insertMulti("qianxi", "#qianxi-clear");

    General *wangyi = new General(this, "wangyi", "wei", 3, false); // YJ 110
    wangyi->addSkill(new Zhenlie);
    wangyi->addSkill(new Miji);

    General *xunyou = new General(this, "xunyou", "wei", 3); // YJ 111
    xunyou->addSkill(new Qice);
    xunyou->addSkill(new Zhiyu);

    addMetaObject<QiceCard>();
    addMetaObject<ChunlaoCard>();
    addMetaObject<ChunlaoWineCard>();
    addMetaObject<GongqiCard>();
    addMetaObject<JiefanCard>();
    addMetaObject<AnxuCard>();
	
}

ADD_PACKAGE(YJCM2012)

