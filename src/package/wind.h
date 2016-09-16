#ifndef _WIND_H
#define _WIND_H

#include "package.h"
#include "card.h"
#include "skill.h"

class HuangtianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuangtianCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class ShensuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShensuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TianxiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TianxiangCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class GuhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GuhuoCard();
    bool guhuo(ServerPlayer *yuji) const;

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class Jushou : public PhaseChangeSkill
{
public:
    Jushou();
    virtual bool onPhaseChange(ServerPlayer *target) const;

protected:
    virtual int getJushouDrawNum(ServerPlayer *caoren) const;
};

class WindPackage : public Package
{
    Q_OBJECT

public:
    WindPackage();
};

#endif

