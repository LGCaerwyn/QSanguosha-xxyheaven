#include "mol.h"
#include "client.h"
#include "general.h"
#include "skill.h"
#include "standard-skillcards.h"
#include "engine.h"
#include "maneuvering.h"
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

class Dujin : public DrawCardsSkill
{
public:
    Dujin() : DrawCardsSkill("dujin")
    {
        frequency = Frequent;
    }

    int getDrawNum(ServerPlayer *player, int n) const
    {
        if (player->askForSkillInvoke(this)) {
            player->broadcastSkillInvoke(objectName());
            return n + player->getEquips().length() / 2 + 1;
        } else
            return n;
    }
};

QingyiCard::QingyiCard()
{
}

bool QingyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("qingyi");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

void QingyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, NULL, false))
            targets.removeOne(target);
    }

    if (targets.length() > 0) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_qingyi");
        room->useCard(CardUseStruct(slash, source, targets));
    }
}

class QingyiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    QingyiViewAsSkill() : ZeroCardViewAsSkill("qingyi")
    {
        response_pattern = "@@qingyi";
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@qingyi";
    }

    const Card *viewAs() const
    {
        return new QingyiCard;
    }
};

class Qingyi : public TriggerSkill
{
public:
    Qingyi() : TriggerSkill("qingyi")
    {
        events << EventPhaseChanging;
        view_as_skill = new QingyiViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !player->isSkipped(Player::Judge)
            && !player->isSkipped(Player::Draw)) {
            if (Slash::IsAvailable(player) && room->askForUseCard(player, "@@qingyi", "@qingyi-slash")) {
                player->skip(Player::Judge, true);
                player->skip(Player::Draw, true);
            }
        }
        return false;
    }
};

class Shixin : public TriggerSkill
{
public:
    Shixin() : TriggerSkill("shixin")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Fire) {
            room->notifySkillInvoked(player, objectName());
            player->broadcastSkillInvoke(objectName());

            LogMessage log;
            log.type = "#ShixinProtect";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = "fire_nature";
            room->sendLog(log);
            return true;
        }
        return false;
    }
};

class Fenyin : public TriggerSkill
{
public:
    Fenyin() : TriggerSkill("fenyin")
    {
        events << EventPhaseStart << CardUsed << CardResponded;
        global = true;
        frequency = Frequent;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == EventPhaseStart)
            return 6;

        return TriggerSkill::getPriority(triggerEvent);
    }

    bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::RoundStart)
                player->setMark(objectName(), 0);

            return false;
        }

        const Card *c = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (player == use.from)
                c = use.card;
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse)
                c = resp.m_card;
        }

        if (c == NULL || c->isKindOf("SkillCard") || player->getPhase() == Player::NotActive)
            return false;

        if (player->getMark(objectName()) != 0) {
            Card::Color old_color = static_cast<Card::Color>(player->getMark(objectName()) - 1);
            if (old_color != c->getColor() && player->hasSkill(this) && player->askForSkillInvoke(this, QVariant::fromValue(c))) {
                player->broadcastSkillInvoke(objectName());
                player->drawCards(1);
            }
        }

        player->setMark(objectName(), static_cast<int>(c->getColor()) + 1);

        return false;
    }
};

class TunchuDraw : public DrawCardsSkill
{
public:
    TunchuDraw() : DrawCardsSkill("tunchu")
    {
        view_as_skill = new dummyVS;
    }

    int getDrawNum(ServerPlayer *player, int n) const
    {
        if (player->askForSkillInvoke("tunchu")) {
            player->setFlags("tunchu");
            player->broadcastSkillInvoke("tunchu");
            return n + 2;
        }

        return n;
    }
};

class TunchuEffect : public TriggerSkill
{
public:
    TunchuEffect() : TriggerSkill("#tunchu-effect")
    {
        events << AfterDrawNCards;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->hasFlag("tunchu") && !player->isKongcheng()) {
            const Card *c = room->askForExchange(player, "tunchu", 1, 1, false, "@tunchu-put");
            if (c != NULL)
                player->addToPile("food", c);
        }

        return false;
    }
};

class Tunchu : public TriggerSkill
{
public:
    Tunchu() : TriggerSkill("#tunchu-disable")
    {
        events << EventLoseSkill << EventAcquireSkill << CardsMoveOneTime;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill && data.toString() == "tunchu") {
            room->removePlayerCardLimitation(player, "use", "Slash,Duel$0");
        } else if (triggerEvent == EventAcquireSkill && data.toString() == "tunchu") {
            if (!player->getPile("food").isEmpty())
                room->setPlayerCardLimitation(player, "use", "Slash,Duel", false);
        } else if (triggerEvent == CardsMoveOneTime && player->isAlive() && player->hasSkill("tunchu", true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && move.to_pile_name == "food") {
                if (player->getPile("food").length() == 1)
                    room->setPlayerCardLimitation(player, "use", "Slash,Duel", false);
            } else if (move.from == player && move.from_places.contains(Player::PlaceSpecial)
                && move.from_pile_names.contains("food")) {
                if (player->getPile("food").isEmpty())
                    room->removePlayerCardLimitation(player, "use", "Slash,Duel$0");
            }
        }
        return false;
    }
};

class ShuliangVS : public OneCardViewAsSkill
{
public:
    ShuliangVS() : OneCardViewAsSkill("shuliang")
    {
        response_pattern = "@@shuliang";
        filter_pattern = ".|.|.|food";
        expand_pile = "food";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class Shuliang : public TriggerSkill
{
public:
    Shuliang() : TriggerSkill("shuliang")
    {
        events << EventPhaseStart;
        view_as_skill = new ShuliangVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->getPhase() == Player::Finish;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        foreach (ServerPlayer *const &p, room->getAllPlayers()) {
            if (!player->isKongcheng()) break;
			if (!TriggerSkill::triggerable(p) || p->getPile("food").isEmpty()) continue;

            const Card *card = room->askForCard(p, "@@shuliang", "@shuliang:" + player->objectName(), data, Card::MethodNone);
            if (card) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
				CardMoveReason r(CardMoveReason::S_REASON_REMOVE_FROM_PILE, p->objectName(), objectName(), QString());
				room->moveCardTo(card, NULL, Player::DiscardPile, r, true);
				player->drawCards(2, objectName());
			}
        }
		return false;
    }
};

ZhanyiViewAsBasicCard::ZhanyiViewAsBasicCard()
{
    m_skillName = "_zhanyi";
    will_throw = false;
}

bool ZhanyiViewAsBasicCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->addSubcard(this);
    card->setSkillName(getSkillName());
    return card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool ZhanyiViewAsBasicCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->addSubcard(this);
    card->setSkillName(getSkillName());
    return card->targetFixed();
}

bool ZhanyiViewAsBasicCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->addSubcard(this);
    card->setSkillName(getSkillName());
    return card->targetsFeasible(targets, Self);
}

const Card *ZhanyiViewAsBasicCard::validate(CardUseStruct &) const
{
    const Card *card = Sanguosha->getCard(subcards.first());
    QString user_str = user_string;
    Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
    use_card->setSkillName("_zhanyi");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    return use_card;
}

const Card *ZhanyiViewAsBasicCard::validateInResponse(ServerPlayer *) const
{
    const Card *card = Sanguosha->getCard(subcards.first());
    QString user_str = user_string;
    Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
    use_card->setSkillName("_zhanyi");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    return use_card;
}

ZhanyiCard::ZhanyiCard()
{
    target_fixed = true;
}

void ZhanyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->loseHp(source);
    if (source->isAlive()) {
        const Card *c = Sanguosha->getCard(subcards.first());
        if (c->getTypeId() == Card::TypeBasic) {
            room->setPlayerMark(source, "ViewAsSkill_zhanyiEffect", 1);
        } else if (c->getTypeId() == Card::TypeEquip)
            source->setFlags("zhanyiEquip");
        else if (c->getTypeId() == Card::TypeTrick) {
            source->drawCards(2, "zhanyi");
            room->setPlayerFlag(source, "zhanyiTrick");
        }
    }
}

class ZhanyiNoDistanceLimit : public TargetModSkill
{
public:
    ZhanyiNoDistanceLimit() : TargetModSkill("#zhanyi-trick")
    {
        pattern = "^SkillCard";
    }

    int getDistanceLimit(const Player *from, const Card *) const
    {
        return from->hasFlag("zhanyiTrick") ? 1000 : 0;
    }
};

class ZhanyiDiscard2 : public TriggerSkill
{
public:
    ZhanyiDiscard2() : TriggerSkill("#zhanyi-equip")
    {
        events << TargetSpecified;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->hasFlag("zhanyiEquip");
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == NULL || !use.card->isKindOf("Slash"))
            return false;


        foreach (ServerPlayer *p, use.to) {
            room->askForDiscard(p, "zhanyi", 2, 2, false, true, "@zhanyiequip_discard");
        }
        return false;
    }
};

class Zhanyi : public OneCardViewAsSkill
{
public:
    Zhanyi() : OneCardViewAsSkill("zhanyi")
    {

    }

    bool isResponseOrUse() const
    {
        return Self->getMark("ViewAsSkill_zhanyiEffect") > 0;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (!player->hasUsed("ZhanyiCard"))
            return true;

        if (player->getMark("ViewAsSkill_zhanyiEffect") > 0)
            return true;

        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getMark("ViewAsSkill_zhanyiEffect") == 0) return false;
        if (pattern.startsWith(".") || pattern.startsWith("@")) return false;
        if (pattern == "peach" && player->getMark("Global_PreventPeach") > 0) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false; // This is an extremely dirty hack!! For we need to prevent patterns like 'BasicCard'
        }
        return !(pattern == "nullification");
    }

    QString getSelectBox() const
    {
        return "guhuo_b";
    }

    bool buttonEnabled(const QString &button_name) const
    {
        if (Self->getMark("ViewAsSkill_zhanyiEffect") == 0)
            return false;
        return Skill::buttonEnabled(button_name);
    }

    bool viewFilter(const Card *to_select) const
    {
        if (Self->getMark("ViewAsSkill_zhanyiEffect") > 0)
            return to_select->isKindOf("BasicCard");
        else
            return true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        if (Self->getMark("ViewAsSkill_zhanyiEffect") == 0) {
            ZhanyiCard *zy = new ZhanyiCard;
            zy->addSubcard(originalCard);
            return zy;
        }

        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            ZhanyiViewAsBasicCard *card = new ZhanyiViewAsBasicCard;
            card->setUserString(Sanguosha->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        ZhanyiViewAsBasicCard *card = new ZhanyiViewAsBasicCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class ZhanyiRemove : public TriggerSkill
{
public:
    ZhanyiRemove() : TriggerSkill("#zhanyi-basic")
    {
        events << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->getMark("ViewAsSkill_zhanyiEffect") > 0;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive)
            room->setPlayerMark(player, "ViewAsSkill_zhanyiEffect", 0);

        return false;
    }
};

class Yuhua : public TriggerSkill
{
public:
    Yuhua() : TriggerSkill("yuhua")
    {
        events << EventPhaseProceeding;
		frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Discard){
			int num = 0;
			foreach (const Card *card, player->getHandcards()) {
                if (card->isKindOf("BasicCard"))
                    num++;
            }
			int discard_num = num - player->getMaxCards();
			if (player->getHandcardNum() > player->getMaxCards() && player->getHandcardNum() > num){
				room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
			}
			if (discard_num > 0)
				room->askForDiscard(player, "gamerule", discard_num, discard_num, false, false, "@yuhua-discard", "BasicCard");
			room->setTag("SkipGameRule", true);
		}
        return false;
    }
};

class Qirang : public TriggerSkill
{
public:
    Qirang() : TriggerSkill("qirang")
    {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *zhugeguo, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to == zhugeguo && move.to_place == Player::PlaceEquip) {
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (!zhugeguo->isAlive())
                    return false;
                if (zhugeguo->askForSkillInvoke(this)) {
                    zhugeguo->broadcastSkillInvoke(objectName());
					QList<int> tricks;
                    foreach (int card_id, room->getDrawPile())
						if (Sanguosha->getCard(card_id)->getTypeId() == Card::TypeTrick)
							tricks << card_id;
					if (tricks.isEmpty()){
                        LogMessage log;
                        log.type = "$SearchFailed";
                        log.from = zhugeguo;
						log.arg = "trick";
                        room->sendLog(log);
						break;
					}
					int index = qrand() % tricks.length();
					int id = tricks.at(index);
					zhugeguo->obtainCard(Sanguosha->getCard(id), false);
                } else {
                    break;
                }
            }
        }
        return false;
    }
};

class ShanjiaRecord : public TriggerSkill
{
public:
    ShanjiaRecord() : TriggerSkill("#shanjia-record")
    {
        events << PreCardUsed << EventAcquireSkill << EventLoseSkill;
        global = true;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (triggerEvent == PreCardUsed) {
			const Card *card = data.value<CardUseStruct>().card;
			if (card && card->getTypeId() == Card::TypeEquip){
			    room->addPlayerMark(player, "equipped_num");
				if (player->hasSkill("shanjia", true))
                    room->setPlayerMark(player, "#shanjia", qMin(player->getMark("equipped_num"), 7));
	        }
		} else {
			if (player->hasSkill("shanjia", true))
                room->setPlayerMark(player, "#shanjia", qMin(player->getMark("equipped_num"), 7));
			else
				room->setPlayerMark(player, "#shanjia", 0);
		}
        
        return false;
    }
};

class ShanjiaSlash : public ZeroCardViewAsSkill
{
public:
    ShanjiaSlash() : ZeroCardViewAsSkill("shanjia_slash")
    {
        response_pattern = "@@shanjia_slash!";
    }

    const Card *viewAs() const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_shanjia");
        return slash;
    }
};

class ShanjiaDiscard : public ViewAsSkill
{
public:
    ShanjiaDiscard() : ViewAsSkill("shanjia_discard")
    {
        response_pattern = "@@shanjia_discard!";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < Self->getMark("shanjia_disnum") && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == Self->getMark("shanjia_disnum")) {
            DummyCard *xt = new DummyCard;
            xt->addSubcards(cards);
            return xt;
        }
        return NULL;
    }
};

class Shanjia : public PhaseChangeSkill
{
public:
    Shanjia() : PhaseChangeSkill("shanjia")
    {
    }

    virtual bool onPhaseChange(ServerPlayer *caochun) const
    {
        Room *room = caochun->getRoom();
		if (caochun->getPhase() != Player::Play)
			return false;
        int n = qMin(caochun->getMark("equipped_num"), 7);
		if (n > 0 && caochun->askForSkillInvoke(this)){
            caochun->broadcastSkillInvoke(objectName());
			caochun->drawCards(n);
            QList<int> all_cards = caochun->forceToDiscard(10086, true);
            QList<int> to_discard = caochun->forceToDiscard(n, true);
			if (all_cards.length() > n){
                room->setPlayerMark(caochun, "shanjia_disnum", n);
				const Card *card = room->askForCard(caochun, "@@shanjia_discard!", "@shanjia-discard:::" + QString::number(n), QVariant(), Card::MethodNone);
                room->setPlayerMark(caochun, "shanjia_disnum", 0);
				if (card != NULL && card->subcardsLength() == n) {
					to_discard = card->getSubcards();
				}
			}
			bool selected_equipped = false;
			foreach (int card_id, to_discard) {
			    if (room->getCardPlace(card_id) == Player::PlaceEquip) {
					selected_equipped = true;
				}
			}
			DummyCard *dummy_card = new DummyCard(to_discard);
            CardMoveReason mreason(CardMoveReason::S_REASON_THROW, caochun->objectName(), QString(), objectName(), QString());
            room->throwCard(dummy_card, mreason, caochun);
			delete dummy_card;
			if (selected_equipped && Slash::IsAvailable(caochun)){
				ServerPlayer *slash_target = NULL;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (caochun->canSlash(p, NULL, true)) {
						slash_target = p;
						break;
					}
                }
                const Card *use = room->askForUseCard(caochun, "@@shanjia_slash!", "@shanjia-slash", -1, Card::MethodUse, false);
				if (!use){
					Slash *slash = new Slash(Card::NoSuit, 0);
					slash->setSkillName("_shanjia");
                    room->useCard(CardUseStruct(slash, caochun, slash_target), false);
				}
			}
		}
		return false;
    }
};

MOLPackage::MOLPackage()
: Package("MOL")
{
    General *sunru = new General(this, "sunru", "wu", 3, false);
    sunru->addSkill(new Qingyi);
    sunru->addSkill(new SlashNoDistanceLimitSkill("qingyi"));
    sunru->addSkill(new Shixin);
    related_skills.insertMulti("qingyi", "#qingyi-slash-ndl");

    General *lingcao = new General(this, "lingcao", "wu", 4);
    lingcao->addSkill(new Dujin);

    General *liuzan = new General(this, "liuzan", "wu");
    liuzan->addSkill(new Fenyin);

    General *lifeng = new General(this, "lifeng", "shu", 3);
    lifeng->addSkill(new TunchuDraw);
    lifeng->addSkill(new TunchuEffect);
    lifeng->addSkill(new Tunchu);
	lifeng->addSkill(new DetachEffectSkill("tunchu", "food"));
    related_skills.insertMulti("tunchu", "#tunchu-effect");
    related_skills.insertMulti("tunchu", "#tunchu-disable");
	related_skills.insertMulti("tunchu", "#tunchu-clear");
    lifeng->addSkill(new Shuliang);

    General *zhuling = new General(this, "zhuling", "wei");
    zhuling->addSkill(new Zhanyi);
    zhuling->addSkill(new ZhanyiDiscard2);
    zhuling->addSkill(new ZhanyiNoDistanceLimit);
    zhuling->addSkill(new ZhanyiRemove);
    related_skills.insertMulti("zhanyi", "#zhanyi-basic");
    related_skills.insertMulti("zhanyi", "#zhanyi-equip");
    related_skills.insertMulti("zhanyi", "#zhanyi-trick");

	General *zhugeguo = new General(this, "zhugeguo", "shu", 3, false);
	zhugeguo->addSkill(new Yuhua);
	zhugeguo->addSkill(new Qirang);

    General *caochun = new General(this, "caochun", "wei");
    caochun->addSkill(new Shanjia);

    addMetaObject<QingyiCard>();
    addMetaObject<ZhanyiCard>();
    addMetaObject<ZhanyiViewAsBasicCard>();
	
	skills << new ShanjiaDiscard << new ShanjiaSlash << new ShanjiaRecord;
}

ADD_PACKAGE(MOL)
